'''
Sample parser for ExPath Client
'''
#!/usr/bin/env python
import base64
import datetime
import json
import threading

from bson import ObjectId, json_util

from pyexpresso.client import Client
from pyexpresso.reader import ScanReader

HANDLE = open('fixtures/packages.json', 'r')
PACKAGES = json.load(HANDLE)
HANDLE.close()

HANDLE = open('fixtures/vertices.json', 'r')
VERTICES = json.load(HANDLE)
HANDLE.close()

HANDLE = open('fixtures/edges.json', 'r')
EDGES = json.load(HANDLE)
HANDLE.close()


CLIENT = Client(host='127.0.0.1', port=9000)


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
    Push package information off HQ to a stream which tests are going to execute
    Stream simulates Kinesis behavior
    '''
    npack = {}

    for key, value in package.items():
        npack[key] = value

    npack['s'] = []

    for status in package['s']:
        npack['s'].append(status)
        npack['cs'] = status
        stream.append(base64.b64encode(json.dumps(npack, cls=DTEncoder)))


def test_handler(stream):
    '''
    Run tests on streamed data.
    Stream simulates Kinesis behavior
    '''
    for data in stream:
        value = json.loads(base64.b64decode(data))
        scanner = ScanReader(CLIENT, host='127.0.0.1', port=9000)
        scanner.read(value)
    CLIENT.close()

def run(load=False):
    '''
    Run actual tests
    '''
    if load:
        prepare()
    threads = []
    stream = []

    for package in [PACKAGES[0]]:
        thread = threading.Thread(target=push_to_stream, args=(package, stream))
        thread.start()
        threads.append(thread)

    for thread in threads:
        thread.join()

    test_handler(stream)

def prepare():
    '''
    Dump vertex/edge data from fixtures to Fletcher
    '''
    CLIENT.add_vertices(VERTICES)
    CLIENT.add_edges(EDGES)
