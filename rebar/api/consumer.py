#!/usr/bin/env python
'''
This module is invoked by the multilang utility provided by amazon_kclpy
to process records for package scans
'''


import base64
import json
import sys
import time

from amazon_kclpy import kcl

from rebar.config import FAP_QUEUE
from rebar.database.disque import db_connection


def process_record(data):
    '''
    Process a single record retrived from a kinesis stream
    '''
    data = json.loads(data)

    if not data['ivd']:
        return

    destination = data['cn']
    status = data['cs']
    location = status['sl']
    action = status.get('act', None)
    connection = status.get('cid', None)
    pid = status.get('pid', None)

    if status['st'] == 'RT':
        return

    if location == 'NSZ' or destination == 'NSZ':
        return

    if action not in ['<L', '+L', None]:
        return

    if action == '<L' and status['ps'] != status['pid']:
        return

    waybill = data['wbn']
    scan_datetime = status['sd']
    pickup_date = data['pd']

    payload = json.dumps({
        'waybill': waybill,
        'location': location,
        'destination': destination,
        'scan_datetime': scan_datetime,
        'action': action,
        'connection': connection,
        'pickup_date': pickup_date,
        'pid': pid
    }).encode('utf-8')

    client = db_connection()
    client.add_job(FAP_QUEUE, payload, timeout=100)


class PackageStatusProcessor(kcl.RecordProcessorBase):
    '''
    Record processor for package statuses
    '''
    def __init__(self):
        self.initialized = False
        self.sleep_seconds = 5
        self.checkpoint_retries = 5
        self.checkpoint_freq_seconds = 60
        self.shard_id = None
        self.largest_seq = None
        self.last_checkpoint_time = None
        self.initialized = None

    def initialize(self, shard_id):
        '''
        Initialize the status processor
        '''
        self.shard_id = shard_id
        self.largest_seq = None
        self.last_checkpoint_time = time.time()
        self.initialized = True

    def checkpoint(self, checkpointer, sequence_number=None):
        '''
        Records a checkpoint indicating the extent of data read from kinesis
        '''
        for attempt in range(0, self.checkpoint_retries):

            try:
                checkpointer.checkpoint(sequence_number)
                return
            except kcl.CheckpointError as err:

                if err.value == 'ShutdownException':
                    print(
                        'Konsumer: Encountered ShutdownException, skipping '
                        'checkpoint'
                    )
                    return

                elif err.value == 'ThrottlingException':
                    if self.checkpoint_retries - 1 == attempt:
                        print(
                            'Konsumer: Failed to checkpoint after {attempt} '
                            'attempts, giving up.'.format(attempt=attempt),
                            file=sys.stderr
                        )
                        return
                    else:
                        print(
                            'Konsumer: Throttled while checkpointing. Next '
                            'attempt in {s} seconds'.format(
                                s=self.sleep_seconds
                            )
                        )
                elif err.value == 'InvalidStateException':
                    print(
                        'Konsumer: MultiLangDaemon reported an invalid state '
                        'while checkpointing.',
                        file=sys.stderr
                    )
                else:
                    print(
                        'Konsumer: Unknown error while checkpointing: '
                        '{error}'.format(error=err)
                    )
            time.sleep(self.sleep_seconds)

    def process_records(self, records, checkpointer):
        '''
        Process a batch of records retrieved from the kinesis stream
        '''
        for record in records:
            data = base64.b64decode(record.get('data')).decode('utf-8')
            seq = int(record.get('sequenceNumber'))
            process_record(data)

            if self.largest_seq is None or seq > self.largest_seq:
                self.largest_seq = seq

        if (
                time.time() - self.last_checkpoint_time >
                self.checkpoint_freq_seconds
        ):
            self.checkpoint(checkpointer, str(self.largest_seq))
            self.last_checkpoint_time = time.time()

    def shutdown(self, checkpointer, reason):
        '''
        Shut down the reader
        '''
        if reason == 'TERMINATE':
            print('Konsumer: Shutdown requested. Attempting checkpoint.')
            if self.initialized:
                self.checkpoint(checkpointer, None)
        else:
            print('Konsumer: Error occurred. Shutting down')


if __name__ == '__main__':
    KCLPROCESS = kcl.KCLProcess(PackageStatusProcessor())
    KCLPROCESS.run()
