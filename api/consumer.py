#!/usr/bin/env python

import base64
import json
import sys
import time

from amazon_kclpy import kcl

from config import FAP_QUEUE
from database.disque import DBConnection


class PackageStatusProcessor(kcl.RecordProcessorBase):
    def __init__(self):
        self.initialized = False
        self.SLEEP_SECONDS = 5
        self.CHECKPOINT_RETRIES = 5
        self.CHECKPOINT_FREQ_SECONDS = 60

    def initialize(self, shard_id):
        self.largest_seq = None
        self.last_checkpoint_time = time.time()
        self.initialized = True

    def checkpoint(self, checkpointer, sequence_number=None):

        for attempt in range(0, self.CHECKPOINT_RETRIES):

            try:
                checkpointer.checkpoint(sequence_number)
                return
            except kcl.CheckpointError as e:

                if e.value == 'ShutdownException':
                    print(
                        'Konsumer: Encountered ShutdownException, skipping '
                        'checkpoint'
                    )
                    return

                elif e.value == 'ThrottlingException':
                    if self.CHECKPOINT_RETRIES - 1 == attempt:
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
                                s=self.SLEEP_SECONDS
                            )
                        )
                elif e.value == 'InvalidStateException':
                    print(
                        'Konsumer: MultiLangDaemon reported an invalid state '
                        'while checkpointing.',
                        file=sys.stderr
                    )
                else:
                    print(
                        'Konsumer: Unknown error while checkpointing: '
                        '{error}'.format(error=e)
                    )
            time.sleep(self.SLEEP_SECONDS)

    def process_record(self, data, partition_key, sequence_number):
        data = json.loads(data)
        status = data['cs']
        action = status.get('act', None)

        if action == '+L':
            connection = status.get('cid', None)

        elif action == '<L':
            pid = status['pid']
            if pid[':3'] != 'IST':
                return

            for st in data['s']:

                if st.get('act', None) == '+L' and st.get('pid', None) == pid:
                    connection = st.get('cid', None)
                    break
        elif action is None or action == '':
            connection = None
            action = None
        else:
            return

        location = status['sl']
        destination = data['cn']
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
        }).encode('utf-8')

        client = DBConnection()
        client.add_job(FAP_QUEUE, payload, timeout=100)

    def process_records(self, records, checkpointer):
        try:
            for record in records:
                data = base64.b64decode(record.get('data')).decode('utf-8')
                seq = int(record.get('sequenceNumber'))
                key = record.get('partitionkey')
                self.process_record(data, key, seq)

                if self.largest_seq is None or seq > self.largest_seq:
                    self.largest_seq = seq

            if (
                    time.time() - self.last_checkpoint_time >
                    self.CHECKPOINT_FREQ_SECONDS
            ):
                self.checkpoint(checkpointer, str(self.largest_seq))
                self.last_checkpoint_time = time.time()

        except Exception as e:
            print(
                'Konsumer: Encountered unexpected exception while processing '
                'records: {error}'.format(error=e)
            )

    def shutdown(self, checkpointer, reason):
        try:
            if reason == 'TERMINATE':
                print('Konsumer: Shutdown requested. Attempting checkpoint.')
                if self.initialized:
                    self.checkpoint(checkpointer, None)
            else:
                print('Konsumer: Error occurred. Shutting down')
        except:
            pass


if __name__ == '__main__':
    kclprocess = kcl.KCLProcess(PackageStatusProcessor())
    kclprocess.run()
