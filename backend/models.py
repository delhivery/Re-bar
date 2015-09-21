import datetime

# from .utils.graph import create_graph
from fields.relational import ForeignKey
from fields.base import ChoiceField, AutoField
from mongo.utils import DBConnection


class BaseModel(dict):
    collection = ''
    database = ''
    structure = ''
    required_keys = []
    unique_keys = []
    primary_key = None

    @classmethod
    def get_connection(cls, connection=None):
        if not connection:
            connection = DBConnection(cls.database, cls.collection)
        return connection

    @classmethod
    def get(cls, value, connection=None):
        connection = cls.get_connection(connection)
        return connection.find_one({cls.primary_key: value})

    @property
    def pkey(self):
        return self.data.get(self.primary_key)

    def __init__(self, **kwargs):
        self.data = {}

        for key, key_type in self.structure:
            default = key_type

            if key_type is datetime.datetime:
                default = datetime.datetime.now

            if hasattr(key_type, 'valueOf'):
                default = key_type.valueOf

            value = kwargs.get(key, default())

            if not isinstance(value, key_type):
                try:
                    value = key_type(value)
                except TypeError:
                    value = key_type.valueOf(value)
                except:
                    raise TypeError(
                        'Invalid value {} specified for key {}. '
                        'Accepts only {}'.format(
                            value, key, key_type
                        )
                    )
            self.data[key] = value

    def save(self, connection=None):
        connection = self.__class__.get_connection(connection)
        save_data = {}

        for key, value in self.data.items():

            if isinstance(self.structure[key], ForeignKey):
                value = value.pkey

            save_data[key] = value
        connection.insert(self.data)


class DeliveryCenter(BaseModel):

    collection = 'nodes'
    database = 'easypeasy'

    structure = {
        'code': str,
        'active': bool,
    }

    required_keys = ['code']
    primary_key = 'code'
    unique_keys = [('code', ), ]


class Connection(BaseModel):
    collection = 'edges'
    database = 'easypeasy'

    structure = {
        'index': AutoField(),
        'name': str,
        'origin': ForeignKey(DeliveryCenter),
        'destination': ForeignKey(DeliveryCenter),
        'departure': datetime.time(),
        'duration': datetime.time(),
        'type': ChoiceField(type=str, choices=[
            'Local', 'Surface', 'Railroad', 'Air'
        ])
    }

    required_keys = [
        'index', 'name', 'origin', 'destination', 'departure', 'duration',
        'type'
    ]
    primary_key = 'index'
    unique_keys = [('index', ), ]


class GraphNode(BaseModel):
    '''
        Represents a single leg of a path for a waybill
    '''
    collection = 'paths'
    database = 'easypeasy'

    structure = {
        'index': AutoField(),
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
        'index', 'wbn', 'order', 'vertex', 'parent', 'edge', 'arrival',
        'departure', 'state'
    ]
    primary_key = 'index'
    unique_keys = [('index',), ('wbn', 'order')]

# g = create_graph(GraphNode.find({'wbn': '12192078102'}))
# g.get_active()
# if g.active.connects_with(13):
#     g.active.destination.activate()
