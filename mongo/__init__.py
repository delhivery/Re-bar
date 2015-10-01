from pymongo import MongoClient

HOST = '127.0.0.1'
PORT = 27017


def DBConnection(database, host=HOST, port=PORT):
    client = MongoClient(host, port)
    return client[database]
