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

INV_MAPPING = {VALUE: KEY for KEY, VALUE in VERTEX_NAME_CODE_MAPPING.items()}

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


def prettify(segments):
    '''
    Convert center codes to user readeable format
    '''
    tsegments = []

    for segment in segments:
        tsegment = {}

        for key, value in segment.items():
            tsegment[key] = value

        tsegment['src'] = INV_MAPPING.get(tsegment['src'], None)
        tsegment['dst'] = INV_MAPPING.get(tsegment['dst'], None)
        tsegments.append(tsegment)
    return tsegments


def validate(scan_dict):
    '''
    Perform validations and transformations on scan dictionary
    '''

    if scan_dict.get('ivd', None) is None:
        return False

    try:
        scan_dict['cs']['sd'] = iso_to_seconds(scan_dict['cs']['sd'])
        scan_dict['pdd'] = iso_to_seconds(scan_dict['pdd'])

    except (ValueError, TypeError, KeyError):
        return False
    center = None

    try:
        center = scan_dict['cs']['sl']
        scan_dict['cs']['sl'] = center_name_to_code(center)

        center = scan_dict['cn']
        scan_dict['cn'] = center_name_to_code(center)

    except KeyError:
        raise ValueError('BAD CENTER: {}'.format(center))

    except (ValueError, TypeError):
        return False

    if scan_dict['cs'].get('act', None) in ['+C', '<C']:

        if (
                scan_dict['cs'].get('cid', None) is None and
                scan_dict['cs'].get('pid', None) is not None):
            scan_dict['cs']['cid'] = scan_dict['cs']['pid']
            scan_dict['cs']['pri'] = True

    else:
        scan_dict['cs']['pri'] = (
            scan_dict['cs'].get('pid', None) ==
            scan_dict['cs'].get('ps', None))
    return True


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
