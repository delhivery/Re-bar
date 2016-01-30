'''
This class exposes parser and its utilities to update a recommended path
'''
from .utils import match


class Parser(object):
    '''
    Class representing a path with utilities to parse through it
    '''
    __segments = []
    active = None

    # __map_state_segments = {}

    def __init__(self):
        '''
        Initlialize Parser.
        Optionally add segments from existing data
        '''
        self.active = None
        self.__segments = []
        self.active = None

    def deactivate(self):
        '''
        Deactivate all future edges
        '''
        for idx, segment in enumerate(self.__segments):

            if segment['st'] == 'FUTURE':
                self.__segments[idx]['st'] = 'INACT'

    @property
    def arrival(self):
        '''
        Get the actual arrival time at active vertex
        '''
        return self.__segments[self.active]['a_arr']

    @arrival.setter
    def arrival(self, sdt):
        '''
        Set actual arrival at active vertex
        '''
        self.__segments[self.active]['a_arr'] = sdt

    def add_nullseg(self):
        '''
        Add a null segment
        '''
        segment = {
            'src': None,
            'dst': None,
            'conn': None,
            'p_arr': None,
            'a_arr': None,
            'p_dep': None,
            'a_dep': None,
            'cst': None,
            'st': 'REACHED',
            'rmk': [],
            'idx': 0,
            'par': None,
            'sol': None,
            'pdd': None,
        }
        self.__segments.append(segment)
        self.active = 0

    def add_segment(self, subgraph=False, **kwargs):
        '''
        Add a segment to parser
        '''
        if subgraph:

            if len(self.__segments) == 0:
                self.add_nullseg()

        segment = {
            'src': kwargs['src'],
            'dst': kwargs['dst'],
            'conn': kwargs['conn'],
            'p_arr': kwargs['p_arr'],
            'p_dep': kwargs['p_dep'],
            'cst': kwargs['cst'],
            'a_arr': kwargs.get('a_arr', None),
            'a_dep': kwargs.get('a_dep', None),
            'st':  kwargs.get('st', 'FUTURE'),
            'rmk': kwargs.get('rmk', []),
        }

        for key, value in kwargs.items():
            if key not in [
                    'src', 'dst', 'conn', 'p_arr', 'p_dep', 'cst',
                    'a_arr', 'a_dep', 'st', 'rmk', 'idx', 'par']:
                segment[key] = value

        segment['idx'] = kwargs.get('idx', len(self.__segments))

        if not subgraph:
            segment['par'] = kwargs.get('par', len(self.__segments) - 1)
        else:
            segment['par'] = self.active
            segment['st'] = 'ACTIVE'

        if segment['st'] == 'ACTIVE':
            self.active = segment['idx']

        self.__segments.append(segment)
        return len(self.__segments)

    def add_segments(self, segments, novi=False):
        '''
        Add multiple segments to parser
        '''
        if not isinstance(segments, list):
            raise TypeError('List expected. Got {}'.format(type(segments)))

        for segment in segments:
            self.add_segment(subgraph=novi, **segment)
            novi = False

    def mark_inbound(self, scan_datetime, rmk=None, fail=False):
        '''
        Mark inbound against path
        '''
        self.__segments[self.active]['a_arr'] = scan_datetime

        if rmk:
            self.__segments[self.active]['rmk'].append(rmk)

        if fail:
            self.__segments[self.active]['st'] = 'FAIL'
            self.deactivate()
            self.active = self.__segments[self.active]['par']

    def mark_outbound(self, scan_datetime, rmk=None, fail=False):
        '''
        Mark outbound against path
        '''
        self.__segments[self.active]['a_dep'] = scan_datetime

        if rmk:
            self.__segments[self.active]['rmk'].append(rmk)

        if fail:
            self.__segments[self.active]['st'] = 'FAIL'
            self.deactivate()
            self.active = self.__segments[self.active]['par']
        else:
            self.__segments[self.active]['st'] = 'REACHED'
            self.active = self.lookup(**{
                'par': self.__segments[self.active]['idx'],
                'st': 'FUTURE'
            })
            self.__segments[self.active]['st'] = 'ACTIVE'

    def make_new(self, connection, intermediary):
        '''
        Make a duplicate node from old active to new intermediary
        and mark it reached
        '''
        segment = {}

        for key, value in self.__segments[self.active].items():
            segment[key] = value
        segment['conn'] = connection
        segment['dst'] = intermediary
        segment['st'] = 'REACHED'
        segment['idx'] = len(self.__segments)
        segment['par'] = self.__segments[self.active]['par']
        self.__segments.append(segment)
        self.active = segment['idx']

    def parse_inbound(self, location, scan_datetime):
        '''
        Parse inbound against graph
        [in] location: Location at which inbound has been performed
        [in] scan_datetime: Date time of inscan
        [out] boolean: True on successful inbound, False on failed inbound
        '''
        if self.active is None:
            raise ValueError('No active nodes found')
        a_seg = self.__segments[self.active]

        if a_seg['src'] == location:
            if a_seg['p_arr'] >= scan_datetime:
                self.mark_inbound(scan_datetime)
                return True
            elif scan_datetime < a_seg['p_dep']:
                self.mark_inbound(scan_datetime, rmk='WARN_LATE_ARRIVAL')
                return True
            else:
                # print('Late arrival. Got {}'.format(scan_datetime))
                self.mark_inbound(
                    scan_datetime, rmk='FAIL_LATE_ARRIVAL', fail=True)
        else:
            # print('Location mismatch. Got {}'.format(location))
            self.mark_inbound(
                scan_datetime, rmk='LOCATION_MISMATCH', fail=True)
        return False

    def parse_outbound(self, location, connection, scan_datetime):
        '''
        Parse outbound against graph
        '''

        if self.active is None:
            raise ValueError('No active nodes found')
        a_seg = self.__segments[self.active]

        if a_seg['src'] == location:

            if a_seg['conn'] == connection:

                if a_seg['p_dep'] >= scan_datetime:
                    self.mark_outbound(scan_datetime)
                    return True
                elif a_seg['p_dep'] - scan_datetime < 24 * 3600:
                    self.mark_outbound(
                        scan_datetime, rmk='WARN_LATE_DEPARTURE')
                    return True
                else:
                    self.mark_outbound(
                        scan_datetime, rmk='CONNECTION_MISMATCH', fail=True)
            else:
                self.mark_outbound(
                    scan_datetime, rmk='CONNECTION_MISMATCH', fail=True)
        else:
            self.mark_outbound(
                scan_datetime, rmk='LOCATION_MISMATCH', fail=True)
        return False

    def lookup(self, **kwargs):
        '''
        Return the index for segment matching kwargs
        '''
        for index, segment in enumerate(self.__segments):

            if match(segment, kwargs):
                return index
        return None

    @property
    def value(self):
        '''
        Return the entire graph as a list
        '''
        return self.__segments
