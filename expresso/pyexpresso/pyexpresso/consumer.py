'''
AWS Lambda Handler
'''
import json

from base64 import b64decode

import boto3

from .manager import ScanReader, Client

EXPATH_HOST = 'Expath-Fletcher-ELB-544799728.us-east-1.elb.amazonaws.com'
EXPATH_PORT = 80

S3CLIENT = boto3.client('s3')
S3BUCKET = 'ExPath20160321'

DDCLIENT = boto3.resource('dynamodb', region_name='us-east-1')
DDTABLE = DDCLIENT.Table('expath')


def lambda_handler(event, context):
    '''
    Invokation handler for records fetched via kinesis on lambda
    '''
    client = Client(host=EXPATH_HOST, port=EXPATH_PORT)

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
        reader = ScanReader(client, S3CLIENT, S3BUCKET)

        if (data.get('cs', {}).get('st', None) != 'UD'):
            # Ignore non-forward flow
            continue

        try:
            reader.read(data)
        except ValueError:
            continue
    client.close()
