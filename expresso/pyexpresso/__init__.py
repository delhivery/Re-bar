import json
import socket
import struct


def number_to_bytes(number):
    '''
    Convert a number to bytes to send across a socket as a single byte
    '''
    if (number > 255):
        raise Exception(
            'Supported range is 0-255. Unsupported value provided: {}'.format(i
                number
            )
        )
    return bytes([number])


def keyword_to_bytes(keyword):
    '''
    Converts an argument to bytes to send across a socket as string
    '''
    return bytes(keyword, "UTF-8")


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

        if type(value) is int:
            data += keyword_to_bytes("INT")
        elif type(value) is str:
            data += keyword_to_bytes("STR")
        else:
            data += keyword_to_bytes("DBL")
        data += param_to_bytes(key) + param_to_bytes(value)
    return data


def command_to_bytes(mode, command, **kwargs):
    '''
    Converts a mode, command and named argument combination to re-bar supported protocol
    '''
    return number_to_bytes(mode) + keyword_to_bytes(command) + number_to_bytes(len(kwargs)) + kwargs_to_bytes(kwargs)


class Client:
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
        self.__handler.connect(host, port)

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
        body_length = struct.unpack('I', handler.recv(4))[0]
        return json.loads(this.__handler.recv(body_length).decode('utf-8'))
