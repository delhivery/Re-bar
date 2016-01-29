'''
AWS Lambda Handler
'''
import json

from base64 import b64decode
from pyexpresso import ScanReader


def lambda_handler(event, context):
    '''
    Invokation handler for records fetched via kinesis on lambda
    '''
    # responses = []

    for record in event['Records']:
        data = json.loads(b64decode(record['kinesis']['data']))
        scanner = ScanReader(data)
