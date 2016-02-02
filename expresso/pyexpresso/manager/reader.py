'''
Exposes a reader for scans off kinesis stream
'''
from operator import itemgetter

from .parser import Parser
from .utils import (
    iso_to_seconds, center_name_to_code, mod_path, load_from_local,
    load_from_s3, store_to_local, store_to_s3)

MODES = ['RCSP', 'STSP']


class ScanReader(object):
    '''
    Class to read a package scan and load the appropriate graph
    '''

    def __init__(self, client, s3client=None, s3bucket=None, store=False):
        '''
        Initialize a scan reader off a package scan
        '''
        self.__client = client
        self.__s3client = s3client
        self.__s3bucket = s3bucket
        self.__parser = Parser()
        self.__waybill = None
        self.__store = store
        self.__data = []

    @property
    def data(self):
        '''
        Returns status of graph on parsing edge
        '''
        return self.__data

    def read(self, scan_dict):
        '''
        Read a scan and update the graph as needed
        '''
        self.__waybill = scan_dict['wbn']

        if scan_dict.get('ivd', None) is None:
            return
        scan_dict['cs']['sd'] = iso_to_seconds(scan_dict['cs']['sd'])
        scan_dict['pdd'] = iso_to_seconds(scan_dict['pdd'])

        try:
            scan_dict['cs']['sl'] = center_name_to_code(scan_dict['cs']['sl'])
        except KeyError:
            self.__parser.mark_termination(
                'MISSING DATA. BAD CENTER: {}'.format(
                    scan_dict['cs']['sl']))
            raise ValueError

        try:
            scan_dict['cn'] = center_name_to_code(scan_dict['cn'])
        except KeyError:
            self.__parser.mark_termination(
                'MISSING DATA. BAD CENTER: {}'.format(scan_dict['cn'])
            )
            raise ValueError

        scan = {
            'src': scan_dict['cs']['sl'],
            'dst': scan_dict['cn'],
            'sdt': scan_dict['cs']['sd'],
            'pdd': scan_dict['pdd'],
            'act': scan_dict['cs'].get('act', None),
            'cid': scan_dict['cs'].get('cid', None),
            'pri': True if (scan_dict['cs'].get(
                'ps', None) == scan_dict['cs'].get(
                    'pid', None)) else False,
        }

        if scan['act'] in ['+C', '<C']:
            scan['pri'] = True

            if scan.get('cid', None) is None:
                scan['cid'] = scan.get('pid', None)

        self.load(scan['src'], scan['dst'], scan['sdt'], scan['pdd'])

        if scan['act'] in ['<L', '<C']:

            if scan['pri']:
                success = self.__parser.parse_inbound(scan['src'], scan['sdt'])

                if not success:
                    self.create(
                        scan['src'], scan['dst'], scan['sdt'], scan['pdd'])

                if self.__store:
                    self.__data.append({
                        'segments': self.__parser.value,
                        'scan': scan})

        elif scan['act'] in ['+L', '+C']:
            success = self.__parser.parse_outbound(
                scan['src'], scan['cid'], scan['sdt'])

            if not success:
                self.predict(**scan)

            if self.__store:
                self.__data.append({
                    'segments': self.__parser.value,
                    'scan': scan})

        if self.__store:
            store_to_local(self.__waybill, self.__parser.value)
        else:
            store_to_s3(
                self.__s3client, self.__s3bucket, self.__waybill,
                self.__parser.value)

    def load(self, src, dst, sdt, pdd):
        '''
        Get or create graph data
        '''
        if self.__store:
            segments = load_from_local(self.__waybill)
        else:
            segments = load_from_s3(
                self.__s3client, self.__s3bucket, self.__waybill)

        if segments:
            segments = sorted(segments, key=itemgetter('idx'), reverse=False)
            self.__parser.add_segments(segments)
        else:
            self.create(src, dst, sdt, pdd)

            if self.__store:
                self.__data.append({
                    'segments': self.__parser.value,
                    'scan': {},
                })

    def predict(self, **data):
        '''
        Make prediction from outbound connection to arrival at destination
        '''
        c_data = self.__client.lookup(data['src'], data['cid'])

        if c_data.get('connection', None):
            lat = data['sdt'] + c_data['connection']['dur']
            itd = c_data['connection']['dst']
            self.__parser.make_new_blank(
                data['src'], itd, data['cid'], data['sdt'])
            self.create(itd, data['dst'], lat, data['pdd'])
            self.__parser.arrival = None
        else:
            self.__parser.mark_termination(
                'MISSING DATA: {}'.format(data['cid']))

    def solve(self, src, dst, sdt, pdd, **kwargs):
        '''
        Calls the underlying client to fletcher to populate graph data
        '''
        mode = kwargs.get('mode', 0)
        sout = self.__client.get_path(src, dst, sdt, pdd, mode=mode)

        if sout.get('path', None):
            sout = mod_path(sout['path'], sdt, pdd=pdd, sol=MODES[mode])
            self.__parser.add_segments(sout, novi=True)
            self.__parser.arrival = sdt
            return True
        return False

    def create(self, src, dst, sdt, pdd):
        '''
        Create (sub)graph. RCSP attempted with fallback to STSP
        '''

        if src and dst:

            if not self.solve(src, dst, sdt, pdd):
                self.solve(src, dst, sdt, pdd, mode=1)
