from pymongo import MongoClient

HOST = '127.0.0.1'
PORT = '27017'


def DBConnection(self, database, collection):
    client = MongoClient(HOST, PORT)
    database = client[database]
    collection = database[collection]
    return collection
