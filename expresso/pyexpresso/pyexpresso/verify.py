'''
Local Data Source Handler
'''
import json

from pyexpresso.manager import ScanReader, Client

EXPATH_HOST = '127.0.0.1'
EXPATH_PORT = 9000


def lambda_handler(records):
    '''
    Invokation handler for records fetched via kinesis on lambda
    '''
    client = Client(host=EXPATH_HOST, port=EXPATH_PORT)

    for data in records:

        if (
                (data.get('cs', {}).get('st', None) != 'UD') or
                not data.get('wbn', None)):
            continue

        try:
            reader = ScanReader(client, store=True)
            reader.read(data)
        except:
            print('Exception in reading scan for {}'.format(data))
            raise
    client.close()

if __name__ == '__main__':
    fhandle = open('/home/amitprakash/out.json', 'r')
    records = json.load(fhandle)
    fhandle.close()
    lambda_handler(records)
