import datetime
import json
import sys

from done.marg import Marg

from config import FAP_QUEUE, JOBS_TO_FETCH
from database.disque import DBConnection
from models.base import Connection, DeliveryCenter
from rebar.manager import GraphManager


def manage_wrapper(solver, **kwargs):
    try:
        waybill = kwargs.pop('waybill')
        g = GraphManager(waybill, solver)
        g.parse_path(**kwargs)
    except Exception as err:
        print(
            'Error {} occurred during execution of EP for '
            'payload {}'.format(err, kwargs),
            file=sys.stderr
        )


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
        self._target = manage_wrapper

        for delivery_center in DeliveryCenter.all():
            self.dc_map[delivery_center.name] = delivery_center.code

    def process(self):
        client = DBConnection()

        while True:
            jobs = client.get_job(
                [FAP_QUEUE], count=JOBS_TO_FETCH
            )

            for queue_name, job_id, job in jobs:
                payload = json.loads(job)
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
                manage_wrapper(self.solver, **payload)
                client.ack_job(job_id)

if __name__ == '__main__':
    listener = DeckardCain()
    listener.process()
