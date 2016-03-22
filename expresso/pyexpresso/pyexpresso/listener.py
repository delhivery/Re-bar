'''
Apollo MQ listener
'''
import boto3
import json
import stomp

from .manager import ScanReader, Client

S3CLIENT = boto3.client('s3')
S3BUCKET = 'ExPath20160321'

EXPATH_HOST = 'Expath-Fletcher-ELB-544799728.us-east-1.elb.amazonaws.com'
EXPATH_PORT = 80

EPCLIENT = Client(host=EXPATH_HOST, port=EXPATH_PORT)


class Listener(stomp.ConnectionListener):

    def on_error(self, headers, message):
        print('Recieved an error: {}'.format(message))

    def on_message(self, headers, message):
        # Process Message
        record = json.loads(message)
        reader = ScanReader(EPCLIENT, S3CLIENT, S3BUCKET)
        reader.read(record)

STOMP_HOST = '10.0.30.215'
STOMP_PORT = '61613'
STOMP_USER = 'admin'
STOMP_PASS = 'password'
EP_QUEUE = '/queue/expath'

STOMP = stomp.Connection()
STOMP.set_listener('expath', Listener)
STOMP.start()
STOMP.connect(STOMP_USER, STOMP_PASS, wait=True)
STOMP.subscribe(EP_QUEUE, id=1, ack='auto')
