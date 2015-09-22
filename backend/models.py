import datetime
# from .utils.graph import create_graph
from fields.relational import ForeignKey
from fields.base import ChoiceField, TimeField
from mongo.utils import DBConnection, serialize


class BaseModel(dict):
    collection = ''
    database = ''
    structure = ''
    required_keys = []
    unique_keys = []

    @classmethod
    def get_connection(cls, connection=None):
        if not connection:
            connection = DBConnection(cls.database, cls.collection)
        return connection

    @classmethod
    def get(cls, value, connection=None):
        connection = cls.get_connection(connection)
        return cls(**connection.find_one({'_id': value}))

    @property
    def pkey(self):
        return self.data.get('_id')

    def __init__(self, **kwargs):
        self.data = {}

        if '_id' in kwargs:
            self.data['_id'] = kwargs['_id']

        for key, key_type in self.structure.items():
            value = kwargs.get(key, None)

            if isinstance(key_type, type):

                if not isinstance(value, key_type):
                    value = key_type(value)
            else:
                value = key_type.valueOf(value)

            self.data[key] = value

    def save(self, connection=None):
        connection = self.__class__.get_connection(connection)
        save_data = serialize(self)

        if '_id' in save_data:
            connection.update_one(
                {'_id': save_data['_id']},
                {'$set': save_data}, upsert=True)
        else:
            result = connection.insert_one(save_data)
            self.data['_id'] = result.inserted_id


class DeliveryCenter(BaseModel):

    collection = 'nodes'
    database = 'easypeasy'

    structure = {
        'code': str,
        'active': bool,
    }

    required_keys = ['code']
    unique_keys = [('code', ), ]


class Connection(BaseModel):
    collection = 'edges'
    database = 'easypeasy'

    structure = {
        'name': str,
        'origin': ForeignKey(DeliveryCenter),
        'destination': ForeignKey(DeliveryCenter),
        'departure': TimeField(),
        'duration': TimeField(),
        'type': ChoiceField(type=str, choices=[
            'Local', 'Surface', 'Railroad', 'Air'
        ])
    }

    required_keys = [
        'name', 'origin', 'destination', 'departure', 'duration',
        'type'
    ]
    unique_keys = []


class GraphNode(BaseModel):
    '''
        Represents a single leg of a path for a waybill
    '''
    collection = 'paths'
    database = 'easypeasy'

    structure = {
        'wbn': str,
        'vertex': ForeignKey(DeliveryCenter),
        'parent': ForeignKey('backend.models.GraphNode', required=False),
        'edge': ForeignKey(Connection, required=False),
        'arrival': datetime.datetime,
        'departure': datetime.datetime,
        'state': ChoiceField(type=str, choices=[
            'active', 'reached', 'future'
        ]),
    }

    required_keys = [
        'wbn', 'vertex', 'state'
    ]
    unique_keys = [('wbn', 'order')]

# g = create_graph(GraphNode.find({'wbn': '12192078102'}))
# g.get_active()
# if g.active.connects_with(13):
#     g.active.destination.activate()
