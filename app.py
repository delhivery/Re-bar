import datetime
import json
import os
import socket
import sys

import threading

from done.marg import Marg

from models.base import Connection, DeliveryCenter

from .rebar.manager import GraphManager

SOCKET_FILE = '/var/run/fap/socket'


def manage_wrapper(solver, solver_lock, **kwargs):
    print('Got kwargs: {}'.format(kwargs))
    waybill = kwargs.pop('waybill')
    g = GraphManager(waybill, solver, solver_lock)
    g.parse_path(kwargs)


class DeckardCain:

    def __init__(self):
        super(DeckardCain, self).__init__()
        self.solver = Marg(Connection.find({'active': True}))
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
                        payload['location'] = self.dc_map[payload['location']]
                        payload['destination'] = self.dc_map[
                            payload['destination']
                        ]
                        payload['scan_datetime'] = datetime.datetime.strptime(
                            payload['scan_datetime'], '%Y-%m-%dT%H:%M:%S.%f')
                        call = threading.Thread(
                            target=manage_wrapper,
                            args=(
                                self.solver, self.solver_lock
                            ),
                            kwargs=payload,
                            name='ep_{}'.format(payload.get('waybill', None))
                        )
                        call.start()
                    except (TypeError, KeyError) as e:
                        print(
                            'Unable to parse payload: {}. Error : {} '
                            'Skipping'.format(
                                dstream, e
                            ),
                            file=sys.stderr
                        )
                else:
                    print('Empty dstream')
                connection.close()

if __name__ == '__main__':
    listener = DeckardCain()
    listener.serve()
