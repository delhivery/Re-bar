'''
Utitlity functions for various components of PyExpresso
'''
from __future__ import print_function

import datetime
import json
import uuid

import boto3
import botocore

VNCMAP_HANDLE = open('fixtures/vertex_name_code_mapping.json', 'r')
VERTEX_NAME_CODE_MAPPING = json.load(VNCMAP_HANDLE)
VNCMAP_HANDLE.close()

EPOCH = datetime.datetime(1970, 1, 1)

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


def load_from_s3(client, bucket, waybill):
    '''
    Load graph data from s3
    '''
    path = '/tmp/{}{}'.format(uuid.uuid4(), waybill)
    try:
        client.download_file(bucket, waybill, path)
        handler = open(path, 'r')
        data = json.load(handler)
        handler.close()
        return data
    except (
            boto3.exceptions.S3TransferFailedError,
            botocore.exceptions.ClientError):
        return []
    return []


def load_from_local(waybill):
    '''
    Load graph from local
    '''
    path = 'tests/{}'.format(waybill)
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
    path = 'tests/{}'.format(waybill)
    handler = open(path, 'w')
    handler.write(json.dumps(data))
    handler.close()


def store_to_s3(client, bucket, waybill, data):
    '''
    Store graph data to s3
    '''
    path = '/tmp/{}{}'.format(uuid.uuid4(), waybill)
    handler = open(path, 'w')
    handler.write(json.dumps(data))
    handler.close()
    client.upload_file(path, bucket, waybill)
