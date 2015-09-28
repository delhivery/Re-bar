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
            if value is None:
                data[key] = value
            elif hasattr(value, 'serialize'):
                if key == 'parent':
                    if value.serialize is None:
                        data[key] = value._id
                    else:
                        data[key] = value.serialize(recurse=False)
                else:
                    data[key] = value.serialize()
            else:
                data[key] = value

    return data
