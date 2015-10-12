import datetime
import json
import os
import socket
import sys

import threading

from done.marg import Marg

from models.base import Connection, DeliveryCenter

from rebar.manager import GraphManager

SOCKET_FILE = '/var/run/fap/socket'


def manage_wrapper(solver, solver_lock, **kwargs):
    waybill = kwargs.pop('waybill')
    g = GraphManager(waybill, solver, solver_lock)
    try:
        g.parse_path(**kwargs)
    except KeyError as err:
        print('Missing key in kwargs: {} Error: {}'.format(kwargs, err))


class DeckardCain:

    def __init__(self):
        super(DeckardCain, self).__init__()

        connections = []

        for connection in Connection.find({'active': True}):
            connections.append({
                'id': connection._id,
                'cutoff_departure': connection.departure,
                'duration': connection.duration,
                'origin': connection.origin.code,
                'destination': connection.destination.code
            })
        self.solver = Marg(connections, json=True)
        self.dc_map = {}
        self.solver_lock = threading.Lock()
        self._target = manage_wrapper

        try:
            os.unlink(SOCKET_FILE)
        except OSError:
            if os.path.exists(SOCKET_FILE):
                raise

        self.socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.socket.bind(SOCKET_FILE)

        for delivery_center in DeliveryCenter.all():
            self.dc_map[delivery_center.name] = delivery_center.code

        self.socket.listen(1)
        print('Started listening')

    def serve(self):
        while True:
            connection, client_addr = self.socket.accept()

            try:
                dstream = b''
                while True:
                    data = connection.recv(16)

                    if data:
                        dstream += data
                    else:
                        break
            finally:
                dstream = dstream.decode('utf-8')

                if dstream:
                    try:
                        payload = json.loads(dstream)
                        payload['location'] = self.dc_map[
                            payload['location'].split(' (')[0]
                        ]

                        if payload['destination'] is None:
                            raise ValueError(
                                'Skipping package. Missing destination'
                            )
                        payload['destination'] = self.dc_map[
                            payload['destination'].split(' (')[0]
                        ]

                        try:
                            payload[
                                'scan_datetime'
                            ] = datetime.datetime.strptime(
                                payload['scan_datetime'],
                                '%Y-%m-%dT%H:%M:%S.%f'
                            )
                        except ValueError:
                            payload[
                                'scan_datetime'
                            ] = datetime.datetime.strptime(
                                payload['scan_datetime'],
                                '%Y-%m-%dT%H:%M:%S'
                            )

                        try:
                            payload[
                                'pickup_date'
                            ] = datetime.datetime.strptime(
                                payload['pickup_date'],
                                '%Y-%m-%dT%H:%M:%S.%f'
                            )
                        except ValueError:
                            payload[
                                'pickup_date'
                            ] = datetime.datetime.strptime(
                                payload['pickup_date'],
                                '%Y-%m-%dT%H:%M:%S'
                            )

                        call = threading.Thread(
                            target=manage_wrapper,
                            args=(
                                self.solver, self.solver_lock
                            ),
                            kwargs=payload,
                            name='ep_{}'.format(payload.get('waybill', None))
                        )
                        call.start()
                    except (TypeError, KeyError, ValueError) as err:
                        print(
                            'Unable to parse payload: {}. Error : {} '
                            'Skipping'.format(
                                dstream, err
                            ),
                            file=sys.stderr
                        )
                else:
                    print('Empty dstream')
                connection.close()

if __name__ == '__main__':
    listener = DeckardCain()
    listener.serve()
