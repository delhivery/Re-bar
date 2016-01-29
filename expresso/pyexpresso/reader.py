'''
Exposes a reader for scans off kinesis stream
'''
import datetime
import json
import uuid
import boto3

from operator import itemgetter

from tabulate import tabulate

from .parser import Parser
from .mapper import VERTEX_NAME_CODE_MAPPING

S3CLIENT = boto3.client('s3')
S3BUCKET = 'expath'


def pretty(records):

    if records:
        keys = ['idx', 'par', 'sol', 'st', 'cst', 'conn', 'src', 'dst', 'p_arr', 'a_arr', 'p_dep', 'a_dep', 'pdd'] 
        # keys = records[0].keys()
        table = []

        if 'rmk' in keys:
            keys.remove('rmk')

        for data in records:
            if data['st']: # in ['ACTIVE', 'FAIL', 'REACHED']:
                output = []

                for key in keys:
                    output.append(data[key])
                table.append(output)
        print tabulate(table, headers=keys)
        print ('\n\n\n')


def mod_path(path, start_time, **kwargs):
    '''
    Convert a path to a segment
    '''
    segments = []

    for path_segment in path:
        departure = path_segment['departure_from_source']
        departure = departure + start_time if departure < 1000000 else departure

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

def load_json_from_s3(waybill):
    '''
    Load graph data from s3
    '''
    path = '/tmp/{}{}'.format(uuid.uuid4(), waybill)
    try:
        S3CLIENT.download_file(S3BUCKET, waybill, path)
        handler = open(path, 'r')
        data = json.load(handler)
        handler.close()
        return data
    except Exception as e:
        return []
    return []


def load_from_local(waybill):
    '''
    Load graph from local
    '''
    path = '/home/amitprakash/tests/{}'.format(waybill)
    data = []

    try:
        handler = open(path, 'r')
        data = json.load(handler)
        handler.close()
    except Exception as e:
        return []
    return data


def store_to_local(waybill, data):
    '''
    Write graph to local
    '''
    path = '/home/amitprakash/tests/{}'.format(waybill)
    handler = open(path, 'w')
    handler.write(json.dumps(data))
    handler.close()

def store_to_s3(waybill, data):
    '''
    Store graph data to s3
    '''
    path = '/tmp/{}{}'.format(uuid.uuid4(), waybill)
    handler = open(path, 'w')
    handler.write(json.dumps(data))
    handler.close()
    S3CLIENT.upload_file(path, S3BUCKET, waybill)
    return json.dumps(data)


class ScanReader:
    '''
    Class to read a package scan and load the appropriate graph
    '''

    def __init__(self, client, scan_dict, host='Expath-Fletcher-ELB-544799728.us-east-1.elb.amazonaws.com', port=80):
        '''
        Initialize a scan reader off a package scan
        '''
        # client = Client(host=self.__host, port=self.__port)
        self.__client = client
        self.parser = Parser()
        self.__waybill = scan_dict['wbn']
        self.__host = host
        self.__port = port

        if scan_dict.get('ivd', None) is None:
            return

        current_scan = scan_dict['cs']
        raw_data = {'cs.sl': current_scan['sl'], 'cs.sd': current_scan['sd'], 'cn': scan_dict['cn'], 'pdd': scan_dict['pdd']}
        epoch = datetime.datetime(1970, 1, 1)
        current_scan['sd'] = int((datetime.datetime.strptime(
            current_scan['sd'][:19], '%Y-%m-%dT%H:%M:%S') - epoch).total_seconds())
        scan_dict['pdd'] = int((datetime.datetime.strptime(
            scan_dict['pdd'][:19], '%Y-%m-%dT%H:%M:%S') - epoch).total_seconds())

        scan_dict['cs']['sl'] = VERTEX_NAME_CODE_MAPPING.get(scan_dict['cs']['sl'].split(' (')[0], None)
        scan_dict['cn'] = VERTEX_NAME_CODE_MAPPING.get(scan_dict['cn'].split(' (')[0], None)

        self.load_data(
            current_scan['sl'], scan_dict['cn'],
            current_scan['sd'], scan_dict['pdd'])

        if current_scan.get('act', None) in ['<L', '<C']:
            print('Raw: {}'.format(raw_data))

            if scan_dict.get('ps', None) == scan_dict.get('pid', None):
                success = self.parser.parse_inbound(
                    current_scan['sl'], current_scan['sd'])

                if not success:
                    print('Raw: {}'.format(raw_data))
                    self.create_subgraph(
                        current_scan['sl'], scan_dict['cn'],
                        current_scan['sd'], scan_dict['pdd'])
            pretty(self.parser.value())

        elif current_scan.get('act', None) in ['+L', '+C']:
            print('Raw: {}'.format(raw_data))
            success = self.parser.parse_outbound(
                current_scan['sl'], current_scan['cid'], current_scan['sd'])

            if not success:
                print('Calculating from: {}'.format(current_scan['sd']))
                self.create_outbound(current_scan['sl'], scan_dict['cn'], current_scan['cid'], current_scan['sd'], scan_dict['pdd'])
                '''self.create_subgraph(
                    current_scan['sl'], scan_dict['cn'],
                    current_scan['sd'], scan_dict['pdd'])'''
            pretty(self.parser.value())
        # store_to_s3(self.__waybill, self.parser.value())
        store_to_local(self.__waybill, self.parser.value())

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
        # data = load_json_from_s3(self.__waybill)
        data = load_from_local(self.__waybill)
        data = sorted(data, key=itemgetter('idx'), reverse=False)

        if data:
            self.parser.add_segments(data)
        else:
            self.create_subgraph(
                location, destination, scan_datetime, promise_datetime)
            pretty(self.parser.value())

    def create_outbound(self, location, destination, connection, scan_datetime, promise_datetime):
        response = self.__client.lookup(location, connection)

        if response.get('connection', None):
            max_arrival = scan_datetime + response['connection']['dur']
            next_center = response['connection']['dst']
            self.parser.make_new(connection, next_center)
            self.create_subgraph(next_center, destination, max_arrival, promise_datetime)

    def create_subgraph(
            self, location, destination, scan_datetime, promise_datetime):
        '''
        Create a subgraph via a TCP call to fletcher
        '''

        if location and destination:
            solver_out = self.__client.get_path(location, destination, scan_datetime, promise_datetime)

            if solver_out.get('path', None):
                solver_out = mod_path(solver_out['path'], scan_datetime, sol='RCSP', pdd=promise_datetime)
                self.parser.add_segments(solver_out, novi=True)
            else:
                solver_out = self.__client.get_path(location, destination, scan_datetime, promise_datetime, mode=1)

                if solver_out.get('path', None):
                    solver_out = mod_path(solver_out['path'], scan_datetime, sol='STSP', pdd=promise_datetime)
                    self.parser.add_segments(solver_out, novi=True)
