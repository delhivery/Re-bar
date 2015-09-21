from pymongo import MongoClient

HOST = '127.0.0.1'
PORT = 27017


def DBConnection(database, collection):
    client = MongoClient(HOST, PORT)
    database = client[database]
    collection = database[collection]
    return collection


def serialize(obj):
    data = {}

    for key, value in obj.data.items():

        if hasattr(value, 'serialize'):
            data[key] = value.serialize()
        else:
            data[key] = value

    return data
