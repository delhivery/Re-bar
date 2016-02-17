'''
Sample parser for ExPath Client
'''
from __future__ import print_function

import base64
import datetime
import json
import os
import threading

from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler
from urlparse import parse_qs

import ijson

import requests

from bson import ObjectId, json_util

from pyexpresso.manager import Client
from pyexpresso.manager import ScanReader

PACKAGE_URI = 'https://hq.delhivery.com/api/p/info/{}/.json'
HOST_NAME = '172.16.3.130'
PORT_NUMBER = 8000
FLETCHER_HOST = '127.0.0.1'
FLETCHER_PORT = 9000
CLIENT = Client(host=FLETCHER_HOST, port=FLETCHER_PORT)

class DTEncoder(json.JSONEncoder):
    '''
    Encode datetime object as ISO String
    '''

    def default(self, obj):
        if isinstance(obj, datetime.datetime):
            return obj.isoformat()
        elif isinstance(obj, ObjectId):
            return json.dumps(obj, default=json_util.default)
        return super(DTEncoder, self).default(obj)


def push_to_stream(package, stream):
    '''
    Push package information off HQ to a stream which tests are going to
    execute. Stream simulates Kinesis behavior
    '''
    npack = {}

    for key, value in package.items():
        npack[key] = value

    npack['s'] = []

    if "ivd" not in npack:
        import ipdb
        ipdb.set_trace()

    for status in package['s']:
        npack['s'].append(status)
        npack['cs'] = status
        stream.append(base64.b64encode(json.dumps(npack, cls=DTEncoder)))


def test_handler(stream):
    '''
    Run tests on streamed data.
    Stream simulates Kinesis behavior
    '''
    records = []

    for data in stream:
        value = json.loads(base64.b64decode(data))
        reader = ScanReader(CLIENT, store=True)
        try:
            reader.read(value)
        except ValueError:
            continue
        if reader.data:
            records.extend(reader.data)
    return records


def run(packages):
    '''
    Run actual tests
    '''
    wbn = packages[0]['wbn']
    try:
        os.remove('tests/{}'.format(wbn))
    except OSError:
        pass

    stream = []
    thread = threading.Thread(
        target=push_to_stream, args=(packages[0], stream))
    thread.start()
    thread.join()

    return test_handler(stream)


def ltester():
    '''
    Reads data from a local package fixture and populates output
    '''
    handle = open('packages.json', 'r')
    packages = ijson.parse(handle)
    count = 0

    for package in packages:
        count += 1
        print('Processed packages: {}'.format(count))
        run(package)