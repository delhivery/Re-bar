'''
Utitlity functions for various components of PyExpresso
'''
from __future__ import print_function

import datetime
import json
import tempfile
import uuid

import boto3
import botocore

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


def prettify(segments):
    '''
    Convert center codes to user readeable format
    '''
    tsegments = []

    for segment in segments:
        tsegment = {}

        for key, value in segment.items():
            tsegment[key] = value
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

    scan_dict['cn'] = scan_dict['cn'].split(' (')[0].strip()
    scan_dict['cs']['sl'] = scan_dict['cs']['sl'].split(' (')[0].strip()

    if scan_dict['cs'].get('act', None) in ['+C', '<C']:

        if (
                scan_dict['cs'].get('cid', None) is None and
                scan_dict['cs'].get('pid', None) is not None):
            scan_dict['cs']['cid'] = scan_dict['cs']['pid']

        if (scan_dict['cs']['cid'] is not None):
            scan_dict['cs']['pri'] = True

    else:
        scan_dict['cs']['pri'] = (
            scan_dict['cs'].get('pid', None) ==
            scan_dict['cs'].get('ps', None))
    return True


def mod_path(path, start_time, offset=0, **kwargs):
    '''
    Convert a path to a segment
    '''
    segments = []

    for path_segment in path:
        departure = path_segment['departure_from_source']
        arrival = path_segment['arrival_at_source']
        latest_arrival = path_segment['arrival_max_by']

        source = path_segment['source']
        destination = path_segment['destination']
        connection = path_segment['connection']

        cost = path_segment['cost_reaching_source']
        segment = {
            'src': source,
            'dst': destination,
            'conn': connection,
            'p_arr': arrival,
            'm_arr': latest_arrival,
            'p_dep': departure,
            'cst': cost + offset,
        }

        for key, value in kwargs.items():
            segment[key] = value
        segments.append(segment)
    return segments


def load_from_s3(client, bucket, waybill):
    '''
    Load graph data from s3
    '''
    data = []
    try:
        data = json.load(
            client.get_object(
                Bucket=bucket, Key=waybill)['Body'].read().decode('UTF-8'))
        return data
    except (
            boto3.exceptions.S3TransferFailedError,
            botocore.exceptions.ClientError):
        pass
    return data


def load_from_local(waybill):
    '''
    Load graph from local
    '''
    path = 'tests/{}.json'.format(waybill)
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
    for record in data:
        if record.get('par', None) is not None:
            record['pcon'] = data[record['par']]['conn']

    if data:
        path = 'tests/{}.json'.format(waybill)
        handler = open(path, 'w')
        handler.write(json.dumps(data))
        handler.close()


def store_to_s3(client, bucket, waybill, data):
    '''
    Store graph data to s3
    '''

    for record in data:
        if record.get('par', None) is not None:
            record['pcon'] = data[record['par']]['conn']

    if data:

        with tempfile.NamedTemporaryFile(mode='w+') as handler:
            json.dump(data, handler)
            handler.flush()
            client.upload_file(handler.name, bucket, waybill)
