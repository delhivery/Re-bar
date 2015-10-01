from boto import kinesis
from manager.graph import Graph
import datetime
import json
import time




kinesis = kinesis.connect_to_region(
    "us-east-1", aws_access_key_id="AKIAIR6JG4JKJU6LUXLQ", 
    aws_secret_access_key="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")

shard_id = 'shardId-000000000000'
shard_it = kinesis.get_shard_iterator(
    "Expected_Path_Data", shard_id, "LATEST")["ShardIterator"]
while 1==1:
    out = kinesis.get_records(shard_it, limit=10)
    shard_it = out["NextShardIterator"]
    for line in out['Records']:
        print (line)
        payload = json.loads(line['Data'])
        g = Graph(payload['wbn'])
        if 'connection' in payload['instance'].keys() and \
        payload['instance']['connection']:
            g.update_path(
                payload['instance']['location'], 
                payload['instance']['destination'], 
                datetime.datetime.fromtimestamp(
                    (payload['instance']['scan_datetime']['$date'])/1000.0), 
                payload['instance']['action'], 
                payload['instance']['connection']['index'])
        else:
            g.update_path(
                payload['instance']['location'], 
                payload['instance']['destination'], 
                datetime.datetime.fromtimestamp(
                    (payload['instance']['scan_datetime']['$date'])/1000.0), 
                payload['instance']['action'], None)
        
    time.sleep(0.2)
