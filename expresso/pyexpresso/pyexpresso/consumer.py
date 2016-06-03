'''
AWS Lambda Handler
'''
import json

from base64 import b64decode

import boto3
import redis

from .manager import ScanReader, Client

EXPATH_HOST = (
    'internal-Expath-Fletcher-20160519-1116480816.'
    'us-east-1.elb.amazonaws.com')
EXPATH_PORT = 80


REDIS_HOST = 'expath-redis.toju9q.ng.0001.use1.cache.amazonaws.com'

S3CLIENT = boto3.client('s3')
S3BUCKET = 'tardigrade'

RDCLIENT = redis.StrictRedis(host=REDIS_HOST, port=6379, db=0)


def lambda_handler(event, context):
    '''
    Invokation handler for records fetched via kinesis on lambda
    '''
    records = []
    waybills = []

    for record in event['Records']:
        data = json.loads(b64decode(record['kinesis']['data']))

        if (
                (data.get('cs', {}).get('st', None) != 'UD') or
                not data.get('wbn', None)):
            continue
        else:
            records.append(data)
            waybills.append(data.get('wbn'))

    for record in event['Records']:
        data = json.loads(b64decode(record['kinesis']['data']))

        if (data.get('cs', {}).get('st', None) != 'UD'):
            # Ignore non-forward flow
            continue

        # Add lock for scan
        scan = data.get('cs', {})
        key = '{}{}{}'.format(
            scan.get('act'),
            scan.get('sl'),
            scan.get('sd')
        )
        if not (RDCLIENT.set(key, 1, ex=24 * 3600, nx=True)):
            continue

        try:
            client = Client(host=EXPATH_HOST, port=EXPATH_PORT)
            reader = ScanReader(client, S3CLIENT, S3BUCKET)
            reader.read(data)
            client.close()
        except ValueError:
            continue
