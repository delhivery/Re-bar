#!/usr/bin/env python
'''
Load connection data into fletcher
'''
from __future__ import print_function

import json
import threading

from pyexpresso.manager import Client

FLETCHER_HOST = '127.0.0.1'
FLETCHER_PORT = 9000

HANDLE = open('fixtures/vertices.json', 'r')
VERTICES = json.load(HANDLE)
HANDLE.close()

HANDLE = open('fixtures/edges.json', 'r')
EDGES = json.load(HANDLE)
HANDLE.close()


def add_vertex_chunk(vertices):
    '''
    Add a bunch of vertices to fletcher
    '''
    client = Client(host=FLETCHER_HOST, port=FLETCHER_PORT)
    client.add_vertices(vertices)
    client.close()


def add_edge_chunk(edges):
    '''
    Add a bunch of edges to fletcher
    '''
    client = Client(host=FLETCHER_HOST, port=FLETCHER_PORT)
    client.add_edges(edges)
    client.close()


def chunks(lst, chunk_size):
    '''
    Split a list into equal chunks of chunk_size
    '''

    for idx in range(0, len(lst), chunk_size):
        yield lst[idx: idx + chunk_size]


def prepare():
    '''
    Dump vertex/edge data from fixtures to Fletcher
    '''

    threads = []

    for vertices in chunks(VERTICES, len(VERTICES) / 8):
        thread = threading.Thread(target=add_vertex_chunk, args=[vertices])
        thread.start()
        threads.append(thread)
    print ('Adding vertices over {} threads.'.format(len(threads)))

    for thread in threads:
        thread.join()
    print ('Added vertices')

    for edges in chunks(EDGES, len(EDGES) / 16):
        thread = threading.Thread(target=add_edge_chunk, args=[edges])
        thread.start()
        threads.append(thread)
    print ('Adding edges over {} threads'.format(len(threads)))

    for thread in threads:
        thread.join()
    print ('Added edges')


if __name__ == '__main__':
    prepare()
