'''
Exposes a connection for Disque
'''

from pydisque.client import Client

from ...config import DISQUE_HOSTS


def db_connection(disque_hosts=None):
    '''
    Fetches a connection to Disque
    Takes an optional parameter disque_hosts in case connection to a
    non-configured disque is required
    '''

    if disque_hosts is None:
        disque_hosts = DISQUE_HOSTS
    client = Client(disque_hosts)
    client.connect()
    return client
