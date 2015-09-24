import datetime
# from .utils.graph import create_graph

from bson import ObjectId
from fields.relational import ForeignKey
from fields.base import ChoiceField, TimeField
from mongo.utils import DBConnection, serialize
from manager.utils import recurse_get_attr, recurse_set_attr


class BaseModel(dict):
    collection = None
    database = 'easypeasy'
    required_keys = []
    unique_keys = []

    @classmethod
    def get_connection(cls, connection=None):
        if not connection:
            connection = DBConnection(cls.database, cls.collection)
        return connection

    @classmethod
    def find_one(cls, value, connection=None):
        connection = cls.get_connection(connection)

        if isinstance(value, str):
            value = ObjectId(value)

        if isinstance(value, ObjectId):
            return cls(**connection.find_one({'_id': value}))
        elif isinstance(value, dict):
            cursor = connection.find(value)
            if cursor.count() == 1:
                return cls(**cursor[0])
        raise ValueError('Multiple or no results found')

    @property
    def pkey(self):
        return self.get('_id')

    def __setattr__(self, attribute, value):
        attributes = attribute.split('.')
        recurse_set_attr(attributes, self, value)

    def __getattr__(self, attribute):
        attributes = attribute.split('.')
        return recurse_get_attr(attributes, self)

    def validate(self):
        for key, value in self.items():
            if key in self.required_keys:
                if not value:
                    raise ValueError('Value for {} is required'.format(key))

    def __init__(self, **kwargs):
        if '_id' in kwargs:
            self['_id'] = kwargs['_id']

        for key, key_type in self.structure.items():
            value = kwargs.get(key, None)

            if isinstance(key_type, type):

                if not isinstance(value, key_type):
                    value = key_type(value)
            else:
                value = key_type.valueOf(value)

            self[key] = value

        self.validate()
        super(BaseModel, self).__init__()

    def save(self, connection=None):
        connection = self.__class__.get_connection(connection)
        save_data = serialize(self)

        if '_id' in save_data:
            connection.update_one(
                {'_id': save_data['_id']},
                {'$set': save_data}, upsert=True)
        else:
            result = connection.insert_one(save_data)
            self['_id'] = result.inserted_id


class DeliveryCenter(BaseModel):

    collection = 'nodes'

    required_keys = ['code']
    unique_keys = [('code', ), ]

    def __init__(self, *args, **kwargs):
        self.structure = {
            'code': str,
            'active': bool,
        }
        super(DeliveryCenter, self).__init__(*args, **kwargs)


class Connection(BaseModel):
    collection = 'edges'

    required_keys = [
        'name', 'origin', 'destination', 'departure', 'duration',
        'type'
    ]
    unique_keys = []

    def __init__(self, *args, **kwargs):
        self.structure = {
            'name': str,
            'origin': ForeignKey(DeliveryCenter),
            'destination': ForeignKey(DeliveryCenter),
            'departure': TimeField(),
            'duration': TimeField(),
            'active': bool,
            'type': ChoiceField(type=str, choices=[
                'Local', 'Surface', 'Railroad', 'Air'
            ])
        }
        super(Connection, self).__init__(*args, **kwargs)


class GraphNode(BaseModel):
    '''
        Represents a single leg of a path for a waybill
    '''
    collection = 'paths'

    required_keys = [
        'wbn', 'vertex', 'state'
    ]

    unique_keys = [('wbn', 'order')]

    def __init__(self, *args, **kwargs):
        self.structure = {
            'wbn': str,
            'vertex': ForeignKey(DeliveryCenter, required=False),
            'parent': ForeignKey('backend.models.GraphNode', required=False),
            'edge': ForeignKey(Connection, required=False),
            'arrival': datetime.datetime,
            'departure': datetime.datetime,
            'state': ChoiceField(type=str, choices=[
                'active', 'reached', 'future', 'failed',
            ]),
            'destination': bool,
        }

        super(GraphNode, self).__init__(*args, **kwargs)


# g = create_graph(GraphNode.find({'wbn': '12192078102'}))
# g.get_active()
# if g.active.connects_with(13):
#     g.active.destination.activate()
