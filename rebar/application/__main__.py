'''
This module is responsible for reading data from disque and processing
the requests as scans from HQ to convert it to graph changes
'''

import datetime
import json

from done.marg import Marg

from ..config import FAP_QUEUE, JOBS_TO_FETCH
from ..database.disque import db_connection
from ..models.base import Connection, DeliveryCenter
from ..utils.manager import GraphManager


def manage_wrapper(solver, **kwargs):
    '''
    Wrapper to process extracted payload from queue
    '''
    scan_record = {}

    for key, value in kwargs.items():
        scan_record[key] = value

    waybill = kwargs.pop('waybill')
    manager = GraphManager(waybill, solver)
    manager.analyze_scan(**kwargs)


def process(dc_map, solver):
    '''
    Listen to the queue for jobs, extract the payload and invoke
    the processor
    '''
    client = db_connection()

    while True:
        jobs = client.get_job(
            [FAP_QUEUE], count=JOBS_TO_FETCH
        )

        for _, job_id, job in jobs:
            payload = json.loads(job.decode('utf-8'))

            if payload['destination'] == 'NSZ':
                client.ack_job(job_id)
                continue
            try:
                payload['location'] = dc_map[
                    payload['location'].split(' (')[0]
                ]
            except KeyError:
                print('Skippping package. Unidentified location: {}'.format(
                    payload))
                continue

            if payload['destination'] is None:
                print(
                    'Skipping package. Missing destination: {}'.format(
                        payload
                    )
                )
                continue

            try:
                payload['destination'] = dc_map[
                    payload['destination'].split(' (')[0]
                ]
            except KeyError:
                print('Skippping package. Unidentified destination: {}'.format(
                    payload))
                continue

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
            manage_wrapper(solver, **payload)
            client.ack_job(job_id)

if __name__ == '__main__':
    CONNECTIONS = []

    for connection in Connection.find({'active': True}):
        CONNECTIONS.append({
            'id': connection.get('_id'),
            'cutoff_departure': connection['departure'],
            'duration': connection['duration'],
            'origin': connection['origin']['code'],
            'destination': connection['destination']['code']
        })
    SOLVER = Marg(CONNECTIONS, json=True)
    DC_MAP = {}

    for delivery_center in DeliveryCenter.all():
        DC_MAP[delivery_center['name']] = delivery_center['code']

    process(DC_MAP, SOLVER)
