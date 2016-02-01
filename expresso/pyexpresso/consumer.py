'''
AWS Lambda Handler
'''
import json

import boto3

from base64 import b64decode
from manager import ScanReader, Client

CLIENT = Client(
    host='Expath-Fletcher-ELB-544799728.us-east-1.elb.amazonaws.com', port=80)
S3CLIENT = boto3.client('s3')
S3BUCKET = 'expath'


def lambda_handler(event, context):
    '''
    Invokation handler for records fetched via kinesis on lambda
    '''

    for record in event['Records']:
        data = json.loads(b64decode(record['kinesis']['data']))
        reader = ScanReader(CLIENT, S3CLIENT, S3BUCKET)
        try:
            reader.read(data)
        except ValueError:
            continue
    CLIENT.close()
