'''
Exposes a reader for scans off kinesis stream
'''
import datetime
import json
import uuid
import boto3

from .client import Client
from .parser import Parser
from .mapper import VERTEX_NAME_CODE_MAPPING

S3CLIENT = boto3.client('s3')
S3BUCKET = 'expath'


def mod_path(path, start_time, sol='RCSP'):
    '''
    Convert a path to a segment
    '''
    segments = []

    for path_segment in path:
        departure = path_segment['departure_from_source']
        departure = departure + start_time if departure < 9000000000000000000 else departure

        arrival = path_segment['arrival_at_source']
        arrival = arrival + start_time if arrival < 9000000000000000000 else arrival

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
            'cst': cost
            'sol': sol,
        }

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

    def __init__(self, scan_dict, host='Expath-Fletcher-ELB-544799728.us-east-1.elb.amazonaws.com', port=80):
        '''
        Initialize a scan reader off a package scan
        '''
        self.parser = Parser()
        self.__waybill = scan_dict['wbn']
        self.__host = host
        self.__port = port

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
        client = Client(host=self.__host, port=self.__port)
        location = VERTEX_NAME_CODE_MAPPING.get(location.split(' (')[0], None)
        destination = VERTEX_NAME_CODE_MAPPING.get(destination.split(' (')[0], None)

        if location and destination:
            solver_out = client.get_path(location, destination, scan_datetime, promise_datetime)

            if solver_out.get('path', None):
                solver_out = mod_path(solver_out, scan_datetime)
                self.parser.add_segments(solver_out, True)
