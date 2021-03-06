'''
Implements a client to fletcher
'''

import json
import socket
import struct


def number_to_bytes(number):
    '''
    Convert a number to bytes to send across a socket as a single byte
    '''
    if number > 255:
        raise Exception(
            'Supported range is 0-255. Unsupported value provided: {}'.format(
                number
            )
        )
    return chr(number)


def keyword_to_bytes(keyword):
    '''
    Converts an argument to bytes to send across a socket as string
    '''
    return str(keyword).encode('UTF-8')
    # return bytes(keyword, "UTF-8")


def param_to_bytes(param):
    '''
    Converts an arguments to bytes with a header prefix specifying its length
    '''
    param = '{}'.format(param)
    return number_to_bytes(len(param)) + keyword_to_bytes(param)


def kwargs_to_bytes(kwargs):
    '''
    Convert a map of named arguments to re-bar supported protocol
    '''
    data = b''

    for key, value in kwargs.items():

        if isinstance(value, int):
            data += keyword_to_bytes("INT")
        elif isinstance(value, str) or isinstance(value, unicode):
            data += keyword_to_bytes("STR")
        else:
            data += keyword_to_bytes("DBL")
        data += param_to_bytes(key) + param_to_bytes(value)
    return data


def command_to_bytes(mode, command, **kwargs):
    '''
    Converts a mode, command and named argument combination to re-bar
    supported protocol
    '''
    return (
        number_to_bytes(mode) + keyword_to_bytes(command) +
        number_to_bytes(len(kwargs)) + kwargs_to_bytes(kwargs))


class Client(object):
    '''
    Client to re-bar.

    Sample Usage:
    >> from pyexpresso import Client
    >> client = Client([host], [port])
    >> print(client.execute(command, [mode], **kwargs))
    '''

    __handler = None

    def __init__(self, host="127.0.0.1", port=9000):
        '''
        Initializes a connection to re-bar
        '''
        self.__handler = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.__handler.connect((host, port))

    def close(self):
        '''
        Terminates the connection against server
        '''
        self.__handler.close()

    def execute(self, command, mode=0, **kwargs):
        '''
        Executes a command against server and returns the json response
        '''
        self.__handler.sendall(command_to_bytes(mode, command, **kwargs))
        headers = self.__handler.recv(4)
        body_length = struct.unpack('I', headers)[0]
        return json.loads(self.__handler.recv(body_length).decode('utf-8'))

    def add_vertex(self, vertex):
        '''
        Add a vertex to solver
        '''
        if isinstance(vertex, str) or isinstance(vertex, unicode):
            return self.execute("ADDV", code=vertex)
        raise TypeError(
            'Vertices should be a code or a list of codes. Got {}'.format(
                type(vertex)
            )
        )

    def add_vertices(self, vertices):
        '''
        Add vertices to solver
        '''
        response = {}

        if isinstance(vertices, list):
            for vertex in vertices:
                response[vertex] = self.add_vertex(vertex)
        else:
            raise TypeError(
                'Vertices should be a list of codes. Got {}'.format(
                    type(vertices)
                )
            )
        return response

    def mod_edge(self, code, state):
        '''
        Enable/disable an edge
            [in]code: code for connection
            [in]state: boolean representing state of connection
        '''
        if not isinstance(code, str) and not isinstance(code, unicode):
            raise TypeError('Connection should be a code. Got {}'.format(
                type(code)))

        if not isinstance(state, bool):
            raise TypeError('State should be a boolean. Got {}'.format(
                type(state)))

        if state:
            state = 1
        else:
            state = 0

        kwargs = {
            'code': code,
            'state': state
        }

        return self.execute("MODC", **kwargs)

    def add_edge(self, **kwargs):
        '''
        Add an edge to solver. Parameters
            [in]source: code for source vertex
            [in]destination: code for destination vertex
            [in]code: code for connection
            [in]tip: time for inbound processing
            [in]tap: time for aggregation processing
            [in]top: time for outbound processing
            [in]departure: time of departure
            [in]duration: duration of connection traversal
            [in]cost: cost of connection traversal
        '''
        source = kwargs.get('src', None)
        destination = kwargs.get('dst', None)
        code = kwargs.get('conn', None)
        tip = kwargs.get('tip', None)
        tap = kwargs.get('tap', None)
        top = kwargs.get('top', None)
        departure = kwargs.get('dep', None)
        duration = kwargs.get('dur', None)
        cost = kwargs.get('cost', None)

        if not isinstance(source, str) and not isinstance(source, unicode):
            raise TypeError('Source should be a code. Got {}: {}'.format(
                type(source), kwargs
            ))

        if (
                not isinstance(destination, str) and
                not isinstance(destination, unicode)):
            raise TypeError('Destination should be a code. Got {}: {}'.format(
                type(destination), kwargs
            ))

        if not isinstance(code, str) and not isinstance(code, unicode):
            raise TypeError('Connection should be a code. Got {}'.format(
                type(code)
            ))

        if not isinstance(tip, int):
            raise TypeError(
                'Inbound processing time should be an integer. Got {}'.format(
                    type(tip)
                )
            )

        if not isinstance(tap, int):
            raise TypeError(
                'Aggregation processing time should be an '
                'integer. Got {}'.format(type(tap)))

        if not isinstance(top, int):
            raise TypeError(
                'Outbound processing time should be an integer. Got {}'.format(
                    type(top)
                )
            )

        if not isinstance(departure, int) and departure is not None:
            raise TypeError(
                'Departure time should be an integer. Got {}'.format(
                    type(tip)
                )
            )

        if not isinstance(duration, int) and duration is not None:
            raise TypeError(
                'Duration should be an integer. Got {}'.format(
                    type(tip)
                )
            )

        if not(
                isinstance(cost, int) or isinstance(cost, float)) and (
                    cost is not None):
            raise TypeError(
                'Cost should be a numeric type. Got {}'.format(
                    type(cost)
                )
            )

        kwargs = {
            'src': source,
            'dst': destination,
            'conn': code,
            'tip': tip,
            'tap': tap,
            'top': top
        }

        if (duration is None or duration == 0):
            return self.execute("ADDC", **kwargs)
        elif (
                departure is not None and
                duration is not None and
                cost is not None):
            kwargs['dep'] = departure
            kwargs['dur'] = duration
            kwargs['cost'] = float(cost)
            return self.execute("ADDE", **kwargs)
        raise ValueError(
            'Mismatched values. '
            'All of departure, duration and cost should have integeral values '
            'or be None. Got deparutre<{}>, duration<{}>, cost<{}>'.format(
                departure, duration, cost
            )
        )

    def add_edges(self, edges):
        '''
        Add edges to solver
        '''
        response = {}

        if isinstance(edges, list):
            for edge in edges:
                response[edge['conn']] = self.add_edge(**edge)
        else:
            raise TypeError('Required a list of edges. Got {}'.format(
                type(edges)))
        return response

    def lookup(self, source, edge):
        '''
        Fetch attributes of edge in solver
        '''
        if not isinstance(source, str) and not isinstance(source, unicode):
            raise TypeError('Source should be a code. Got {}'.format(
                type(source)))

        if not isinstance(edge, str) and not isinstance(edge, unicode):
            raise TypeError('Edge should be a code. Got {}'.format(type(edge)))
        return self.execute("LOOK", src=source, conn=edge)

    def get_path(self, source, destination, t_start, t_max, **kwargs):
        '''
        Find a path using solver
        '''
        mode = kwargs.get('mode', 0)

        if not isinstance(source, str) and not isinstance(source, unicode):
            raise TypeError('Source should be a code. Got {}'.format(
                type(source)))

        if (
                not isinstance(destination, str) and
                not isinstance(destination, unicode)):
            raise TypeError('Destination should be a code. Got {}'.format(
                type(destination)))

        if not isinstance(t_start, int):
            raise TypeError(
                'Arrival time at source should be an integer. Got {}'.format(
                    type(t_start)
                )
            )

        if not isinstance(t_max, int):
            raise TypeError('Promise date should be an integer. Got {}'.format(
                type(t_max)))
        return self.execute(
            "FIND", mode=mode, src=source, dst=destination,
            beg=t_start, tmax=t_max)
