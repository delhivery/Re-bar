'''
Exposes a connection for MongoDB
'''

from pymongo import MongoClient

from ...config import MONGO_URI


def db_connection(database, mongo_uri=MONGO_URI):
    '''
    Fetches a connection to MongoDB for a given database.
    Takes an optional parameter mongo_uri in case connection to a
    non-configured database is required
    '''
    client = MongoClient(mongo_uri)
    return client[database]
