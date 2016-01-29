#!/usr/bin/env python
import base64
import datetime
import json
import threading

from bson import ObjectId, json_util
from pyexpresso.case import PACKAGES
from pyexpresso.reader import ScanReader
from pyexpresso.client import Client
from pyexpresso.globber import VERTICES, EDGES

CLIENT = Client(host='127.0.0.1', port=9000)


class DTEncoder(json.JSONEncoder):

    def default(self, obj):
        if isinstance(obj, datetime.datetime):
            return obj.isoformat()
        elif isinstance(obj, ObjectId):
            return json.dumps(obj, default=json_util.default)
        return super(DTEncoder, self).default(obj)

def push_to_stream(package, stream):
    npack = {}

    for key, value in package.items():
        npack[key] = value

    npack['s'] = []

    for status in package['s']:
        npack['s'].append(status)
        npack['cs'] = status
        stream.append(base64.b64encode(json.dumps(npack, cls=DTEncoder)))


def test_handler(stream):
    for data in stream:
        value = json.loads(base64.b64decode(data))
        scanner = ScanReader(CLIENT, value, host='127.0.0.1', port=9000)
    CLIENT.close()

def run(load=False):

    if load:
        prepare()
    threads = []
    stream = []

    for package in [PACKAGES[0]]:
        t = threading.Thread(target=push_to_stream, args=(package, stream))
        t.start()
        threads.append(t)

    for thread in threads:
        thread.join()
    test_handler(stream)

def prepare():
    CLIENT.add_vertices(VERTICES)
    CLIENT.add_edges(EDGES)
