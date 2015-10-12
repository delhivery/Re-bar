from pydisque import Client

from config import DISQUE_HOSTS


def DBConnection(DISQUE_HOSTS=DISQUE_HOSTS):
    client = Client(DISQUE_HOSTS)
    client.connect()
    return client
