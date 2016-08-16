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
    chunk_size = int(chunk_size)
    for idx in range(0, len(lst), chunk_size):
        yield lst[idx: idx + chunk_size]


def prepare(edge_file):
    '''
    Dump vertex/edge data from fixtures to Fletcher
    '''
    handle = open(edge_file, 'r')
    edges = json.load(handle)
    handle.close()
    vertices = set()

    for edge in edges:
        vertices.add(edge['src'])
        vertices.add(edge['dst'])
    vertices = list(vertices)
    threads = []

    for vertices_chunk in chunks(vertices, int(len(vertices) / 8)):
        thread = threading.Thread(target=add_vertex_chunk, args=[vertices_chunk])
        thread.start()
        threads.append(thread)
    print ('Adding vertices over {} threads.'.format(len(threads)))

    for thread in threads:
        thread.join()
    print ('Added vertices')

    for edges_chunk in chunks(edges, len(edges) / 16):
        thread = threading.Thread(target=add_edge_chunk, args=[edges_chunk])
        thread.start()
        threads.append(thread)
    print ('Adding edges over {} threads'.format(len(threads)))

    for thread in threads:
        thread.join()
    print ('Added edges')


if __name__ == '__main__':
    prepare()
