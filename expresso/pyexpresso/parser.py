'''
This class exposes parser and its utilities to update a recommended path
'''


def match(obj, secondary):
    '''
    Returns True if all secondary conditions are met by obj else False
    '''
    for key, value in secondary.items():
        if obj.get(key, None) != value:
            return False
    return True


class Parser:
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

    def deactivate(self):
        '''
        Deactivate all future edges
        '''
        for idx, segment in enumerate(self.__segments):
            if segment['st'] == 'FUTURE':
                self.__segments[idx]['st'] = 'INACTIVE'

    def add_segment(self, subgraph=False, **kwargs):
        '''
        Add a segment to parser
        '''
        segment = {
            'src': kwargs['src'],
            'dst': kwargs['dst'],
            'conn': kwargs['conn'],
            'p_arr': kwargs['p_arr'],
            'p_dep': kwargs['p_dep'],
            'sol': kwargs['sol'],
            'cst': kwargs['cst'],
            'a_arr': kwargs.get('a_arr', None),
            'a_dep': kwargs.get('a_dep', None),
            'st':  kwargs.get('st', 'FUTURE'),
            'rmk': kwargs.get('rmk', []),
        }

        segment['idx'] = kwargs.get('idx', len(self.__segments))

        if not subgraph:
            segment['par'] = kwargs.get(
                'par',
                len(self.__segments) - 1 if len(self.__segments) > 0 else None)
        else:
            segment['par'] = self.active
            segment['st'] = 'ACTIVE'

        if segment['st'] == 'ACTIVE':
            self.active = segment['idx']

        self.__segments.append(segment)
        return len(self.__segments)

    def add_segments(self, segments, new=False):
        '''
        Add multiple segments to parser
        '''
        if not isinstance(segments, list):
            raise TypeError('List expected. Got {}'.format(type(segments)))

        for segment in segments:
            self.add_segment(subgraph=new, **segment)
            new = False

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
        else:
            self.active = self.lookup(**{
                'par': self.__segments[self.active]['idx'],
                'st': 'FUTURE'
            })

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
                self.mark_inbound(
                    scan_datetime, rmk='FAIL_LATE_ARRIVAL', fail=True)
        else:
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
