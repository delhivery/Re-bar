from pymongo import MongoClient

HOST = '127.0.0.1'
PORT = 27017


def DBConnection(database, collection):
    client = MongoClient(HOST, PORT)
    database = client[database]

    if collection:
        return database[collection]
    return database


def serialize(obj):
    data = {}

    for key, value in obj.items():

        if key != 'structure':

            if hasattr(value, 'serialize'):
                if key == 'parent':
                    data[key] = value.serialize(recurse=False)
                else:
                    data[key] = value.serialize()
            else:
                data[key] = value

    return data
