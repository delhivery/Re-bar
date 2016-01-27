'''
AWS Lambda Handler
'''
from pyexpresso import ScanReader


def lambda_handler(event, context):
    '''
    Invokation handler for records fetched via kinesis on lambda
    '''
    responses = []

    for record in event['Records']:
        scanner = ScanReader(record)
        responses.append(scanner.get())
    return responses

