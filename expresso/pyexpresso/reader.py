'''
Exposes a reader for scans off kinesis stream
'''
import datetime
import json
import uuid
import boto3

from .client import Client
from .parser import Parser

S3CLIENT = boto3.client('s3')
S3BUCKET = 'expath'


def load_json_from_s3(waybill):
    '''
    Load graph data from s3
    '''
    # path = '/tmp/{}{}'.format(uuid.uuid4(), waybill)
    # try:
    #     S3CLIENT.download_file(S3BUCKET, waybill, path)
    #     handler = open(path, 'r')
    #     data = json.load(handler)
    #     handler.close()
    #     return data
    # except Exception as e:
    #     return []
    return []


def store_to_s3(waybill, data):
    '''
    Store graph data to s3
    '''
    # path = '/tmp/{}{}'.format(uuid.uuid4(), waybill)
    # handler = open(path, 'w')
    # handler.write(json.dumps(data))
    # handler.close()
    # S3CLIENT.upload_file(path, S3BUCKET, waybill)
    return json.dumps(data)


class ScanReader:
    '''
    Class to read a package scan and load the appropriate graph
    '''

    def __init__(self, scan_dict):
        '''
        Initialize a scan reader off a package scan
        '''
        self.parser = Parser()
        self.__waybill = scan_dict['wbn']

        if scan_dict.get('ivd', None) is None:
            return

        current_scan = scan_dict['cs']
        epoch = datetime.datetime(1970, 1, 1)
        current_scan['sd'] = int((datetime.datetime.strptime(
            current_scan['sd'][:19], '%Y-%m-%dT%H:%M:%S') - epoch).total_seconds())
        scan_dict['pdd'] = int((datetime.datetime.strptime(
            scan_dict['pdd'][:19], '%Y-%m-%dT%H:%M:%S') - epoch).total_seconds())

        self.load_data(
            current_scan['sl'], scan_dict['cn'],
            current_scan['sd'], scan_dict['pdd'])

        if scan_dict.get('act', None) in ['<L', '<C']:

            if scan_dict.get('ps', None) == scan_dict.get('pid', None):
                success = self.parser.parse_inbound(
                    current_scan['sl'], current_scan['sd'])

                if not success:
                    self.create_subgraph(
                        current_scan['sl'], scan_dict['cn'],
                        current_scan['sd'], scan_dict['pdd'])
                store_to_s3(self.__waybill, self.parser.value())

        elif scan_dict.get('act', None) in ['+L', '+C']:
            success = self.parser.parse_outbound(
                current_scan['sl'], current_scan['cid'], current_scan['sd'])

            if not success:
                self.create_subgraph(
                    current_scan['sl'], scan_dict['cn'],
                    current_scan['sd'], scan_dict['pdd'])
            store_to_s3(self.__waybill, self.parser.value())

    def get(self):
        '''
        Return the parsed output
        '''

        if self.parser.value():
            return self.parser.value()

    def load_data(
            self, location, destination, scan_datetime, promise_datetime):
        '''
        Load existing graph data from s3 for waybill
        '''
        data = load_json_from_s3(self.__waybill)

        if data:
            self.parser.add_segments(data)
        else:
            self.create_subgraph(
                location, destination, scan_datetime, promise_datetime)

    def create_subgraph(
            self, location, destination, scan_datetime, promise_datetime):
        '''
        Create a subgraph via a TCP call to fletcher
        '''
        client = Client(host='Expath-Fletcher-ELB-544799728.us-east-1.elb.amazonaws.com', port=80)
        response = self.parser.add_segments(client.get_path(
            location, destination, scan_datetime, promise_datetime))
        self.parser.add_segments(response, subgraph=True, **response)
