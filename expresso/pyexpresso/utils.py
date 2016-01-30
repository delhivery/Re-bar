'''
Utitlity functions for various components of PyExpresso
'''
from __future__ import print_function

import datetime
import json
import uuid

import boto3

from tabulate import tabulate

VNCMAP_HANDLE = open('fixtures/vertex_name_code_mapping.json', 'r')
VERTEX_NAME_CODE_MAPPING = json.load(VNCMAP_HANDLE)
VNCMAP_HANDLE.close()

EPOCH = datetime.datetime(1970, 1, 1)
S3CLIENT = boto3.client('s3')
S3BUCKET = 'expath'


def match(obj, secondary):
    '''
    Returns True if all secondary conditions are met by obj else False
    '''
    for key, value in secondary.items():
        if obj.get(key, None) != value:
            return False
    return True


def iso_to_seconds(iso_string):
    '''
    Convert an ISO datetime string to seconds since epoch
    '''
    iso_string_dt = datetime.datetime.strptime(
        iso_string[:19], '%Y-%m-%dT%H:%M:%S')
    delta = iso_string_dt - EPOCH
    return int(delta.total_seconds())


def center_name_to_code(name):
    '''
    Convert a center name to center code
    '''
    name = name.split(' (')[0]
    return VERTEX_NAME_CODE_MAPPING.get(name, None)


def pretty(records):
    '''
    Print path data in a pretty way
    '''

    if records:
        keys = records[0].keys()
        table = []

        for data in records:
            if data['st']:
                output = []

                for key in keys:
                    output.append(data[key])
                table.append(output)
        print(tabulate(table, headers=keys))
        print('\n\n\n')


def mod_path(path, start_time, **kwargs):
    '''
    Convert a path to a segment
    '''
    segments = []

    for path_segment in path:
        departure = path_segment['departure_from_source']
        departure = (
            departure + start_time) if departure < 1000000 else departure

        arrival = path_segment['arrival_at_source']
        arrival = arrival + start_time if arrival < 1000000 else arrival

        source = path_segment['source']
        destination = path_segment['destination']
        connection = path_segment['connection']

        cost = path_segment['cost_reaching_source']
        segment = {
            'src': source,
            'dst': destination,
            'conn': connection,
            'p_arr': arrival,
            'p_dep': departure,
            'cst': cost,
        }

        for key, value in kwargs.items():
            segment[key] = value
        segments.append(segment)
    return segments


def load_from_s3(waybill):
    '''
    Load graph data from s3
    '''
    path = '/tmp/{}{}'.format(uuid.uuid4(), waybill)
    try:
        S3CLIENT.download_file(S3BUCKET, waybill, path)
        handler = open(path, 'r')
        data = json.load(handler)
        handler.close()
        return data
    except boto3.exceptions.S3TransferFailedError:
        return []
    return []


def load_from_local(waybill):
    '''
    Load graph from local
    '''
    path = '/home/amitprakash/tests/{}'.format(waybill)
    data = []

    try:
        handler = open(path, 'r')
        data = json.load(handler)
        handler.close()
    except (OSError, IOError, ValueError):
        pass
    return data


def store_to_local(waybill, data):
    '''
    Write graph to local
    '''
    path = '/home/amitprakash/tests/{}'.format(waybill)
    handler = open(path, 'w')
    handler.write(json.dumps(data))
    handler.close()


def store_to_s3(waybill, data):
    '''
    Store graph data to s3
    '''
    path = '/tmp/{}{}'.format(uuid.uuid4(), waybill)
    handler = open(path, 'w')
    handler.write(json.dumps(data))
    handler.close()
    S3CLIENT.upload_file(path, S3BUCKET, waybill)
    return json.dumps(data)
