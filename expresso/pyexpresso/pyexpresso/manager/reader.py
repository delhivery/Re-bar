'''
Exposes a reader for scans off kinesis stream
'''
from operator import itemgetter

from .parser import Parser
from .utils import (
    mod_path, load_from_local, prettify,
    load_from_s3, store_to_local, store_to_s3, validate)

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
        src_raw = scan_dict.get('cs', {}).get('sl', None).split(' (')[0]

        try:
            is_valid = validate(scan_dict)

            if not is_valid:
                return
        except ValueError as err:
            self.__parser.mark_termination('{}'.format(err))
            raise

        scan = {
            'src_raw': src_raw,
            'src': scan_dict['cs']['sl'],
            'dst': scan_dict['cn'],
            'sdt': scan_dict['cs']['sd'],
            'pdd': scan_dict['pdd'],
            'act': scan_dict['cs'].get('act', None),
            'cid': scan_dict['cs'].get('cid', None),
            'pri': scan_dict['cs'].get('pri', False),
        }
        self.load(scan['src'], scan['dst'], scan['sdt'], scan['pdd'])

        if scan['act'] == '<L' and scan['pri']:
            success = self.__parser.parse_inbound(scan['src'], scan['sdt'])

            if not success:
                self.create(
                    scan['src'], scan['dst'], (scan['sdt'], 0), scan['pdd'])

            if self.__store:
                self.__data.append({
                    'segments': prettify(self.__parser.value), 'scan': scan})

        elif scan['act'] in ['+L', '+C']:
            success = self.__parser.parse_outbound(
                scan['src'], scan['cid'], scan['sdt'])

            if not success:
                self.predict(**scan)

            if scan['act'] == '+C':
                self.__auto_custody_in(
                    scan['src'], scan['dst'], scan['cid'], scan['sdt'],
                    scan['pdd'])

            if self.__store:
                self.__data.append({
                    'segments': prettify(self.__parser.value),
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
            self.create(src, dst, (sdt, 0), pdd)

            if self.__store:
                self.__data.append({
                    'segments': prettify(self.__parser.value),
                    'scan': {},
                })

    def __auto_custody_in(self, src, dst, cid, sdt, pdd):
        c_data = self.__client.lookup(src, cid)

        if c_data.get('connection', None):
            src = c_data['connection']['dst']
            arr_dt = sdt + 1
            success = self.__parser.parse_inbound(src, arr_dt)

            if not success:
                self.create(src, dst, (arr_dt, 0), pdd)
        elif 'error' not in c_data:
            self.__auto_custody_in(src, dst, cid, sdt, pdd)

    def predict(self, **data):
        '''
        Make prediction from outbound connection to arrival at destination
        '''
        c_data = self.__client.lookup(data['src'], data['cid'])

        if c_data.get('connection', None):
            lat = data['sdt'] + c_data['connection']['dur']
            itd = c_data['connection']['dst']
            itd_cst = c_data['connection']['cost']

            self.__parser.make_new_blank(
                data['src'], itd, data['cid'], data['sdt'])
            self.create(itd, data['dst'], (lat, itd_cst), data['pdd'])
            self.__parser.arrival = None
        elif 'error' in c_data:
            print(
                'Looking up {}, {} failed. Reponse: {} Error: {}'.format(
                    data['src'], data['cid'], c_data, c_data['error']))
            self.__parser.make_new_blank(
                data['src'], None, data['cid'], data['sdt'])
            self.__parser.mark_termination(
                'MISSING_DATA: {}'.format(data['cid']))
        else:
            self.predict(**data)

    def solve(self, src, dst, sdt, pdd, **kwargs):
        '''
        Calls the underlying client to fletcher to populate graph data
        '''
        mode = kwargs.get('mode', 0)
        itd_cst = kwargs.pop('itd_cst', 0)

        sout = self.__client.get_path(src, dst, sdt, pdd, mode=mode)

        if sout.get('path', None):
            sout = mod_path(
                sout['path'], sdt, pdd=pdd, sol=MODES[mode],
                offset=self.__parser.lcost + itd_cst)
            self.__parser.add_segments(sout, novi=True, pdd=pdd)
            self.__parser.arrival = sdt
            return True
        return False

    def create(self, src, dst, offsets, pdd):
        '''
        Create (sub)graph. RCSP attempted with fallback to STSP
        '''

        if src and dst:
            sdt = offsets[0]
            itd_cst = offsets[1]

            if not self.solve(src, dst, sdt, pdd, itd_cst=itd_cst):
                self.solve(src, dst, sdt, pdd, mode=1, itd_cst=itd_cst)
