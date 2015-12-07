from boto import kinesis
from rebar.config import AWS_KINESIS_STREAM

kcl = kinesis.layer1.KinesisConnection()
stream = kcl.describe_stream(AWS_KINESIS_STREAM)
shards = stream['StreamDescription']['Shards']

