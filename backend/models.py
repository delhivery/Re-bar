import datetime
import uuid

# from .utils.graph import create_graph
from fields.relational import ForeignKey
from fields.base import ChoiceField, TimeField
from mongo.utils import DBConnection


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

        for key, key_type in self.structure.items():
            value = kwargs.get(key, None)

            if isinstance(key_type, type):

                if not isinstance(value, key_type):
                    value = key_type(value)
            else:
                value = key_type.valueOf(value)

            self.data[key] = value

        if '_id' not in self.data:
            self.data['_id'] = uuid.uuid4()

    def save(self, connection=None):
        connection = self.__class__.get_connection(connection)
        save_data = {}

        for key, value in self.data.items():
            if key == '_id':
                save_data[key] = value
                continue

            if isinstance(self.structure[key], ForeignKey):
                value = value.pkey

            save_data[key] = value
        connection.update_one(
            {'_id': self.data['_id']},
            {'$set': self.data}, upsert=True)


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
        'order': int,
        'vertex': ForeignKey(DeliveryCenter),
        'parent': ForeignKey('backend.models.GraphNode'),
        'edge': ForeignKey(Connection),
        'arrival': datetime.datetime,
        'departure': datetime.datetime,
        'state': ChoiceField(type=str, choices=[
            'active', 'reached', 'future'
        ]),
    }

    required_keys = [
        'wbn', 'order', 'vertex', 'parent', 'edge', 'arrival',
        'departure', 'state'
    ]
    unique_keys = [('wbn', 'order')]

# g = create_graph(GraphNode.find({'wbn': '12192078102'}))
# g.get_active()
# if g.active.connects_with(13):
#     g.active.destination.activate()
