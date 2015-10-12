from config import MONGO_URI
from pymongo import MongoClient


def DBConnection(database, MONGO_URI=MONGO_URI):
    client = MongoClient(MONGO_URI)
    return client[database]
