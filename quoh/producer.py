import json
import stomp

from base64 import b64decode

STOMP_HOST = '10.0.30.215'
STOMP_PORT = '61613'
STOMP_USER = 'admin'
STOMP_PASS = 'password'


def lambda_handler(event, ctx):
    '''
    Handler to handle kinesis events
    '''
    STOMP = stomp.Connection([(STOMP_HOST, STOMP_PORT)])
    STOMP.start()
    STOMP.connect(STOMP_USER, STOMP_PASS, wait=True)

    for record in event['Records']:
        data = json.loads(b64decode(record['kinesis']['data']))

        if (
                (data.get('cs', {}).get('st', None) != 'UD') or
                not data.get('wbn', None)):
            continue
        STOMP.send('/queue/expath', json.dumps(data), JMSXGroupID=data['wbn'])
    STOMP.disconnect()
