from bson import ObjectId
from fields.relational import ForeignKey
from fields.base import ChoiceField, TimeField, DateTimeField
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
            value = connection.find_one({'_id': value})

            if value is not None:
                return cls(**value)
        elif isinstance(value, dict):
            cursor = connection.find(value)

            if cursor.count() == 1:
                return cls(**cursor[0])
        raise ValueError('Multiple or no results found')

    @classmethod
    def find(cls, filters, fields=[], connection=None):
        connection = cls.get_connection(connection)

        if not isinstance(filters, dict):
            raise TypeError(
                'Expected query string, found {}: {} instead'.format(
                    type(filters), filters
                )
            )

        f_dict = {}

        for field in fields:
            f_dict[field] = 1

        if f_dict:
            return connection.find(filters, f_dict)

        return connection.find(filters)

    def update(cls, filters, update, connection=None):
        connection = cls.get_connection(connection)

        if not isinstance(filters, dict):
            raise TypeError(
                'Expected query dict, found {}: {} instead'.format(
                    type(filters), filters
                )
            )

        if not isinstance(update, dict):
            raise TypeError(
                'Expected update dict, found {}: {} instead'.format(
                    type(update), update
                )
            )

        connection.update(filters, update)

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
    unique_keys = [('index',)]

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
            ]),
            'index': int,
        }
        super(Connection, self).__init__(*args, **kwargs)


class GraphNode(BaseModel):
    '''
        Represents a single leg of a path for a waybill
    '''
    collection = 'paths'

    required_keys = [
        'wbn', 'state'
    ]

    unique_keys = [('wbn', 'order')]

    def __init__(self, *args, **kwargs):
        self.structure = {
            'wbn': str,
            'vertex': ForeignKey(DeliveryCenter, required=False),
            'parent': ForeignKey('backend.models.GraphNode', required=False),
            'edge': ForeignKey(Connection, required=False),
            'e_arr': DateTimeField(required=False),
            'e_dep':  DateTimeField(required=False),
            'state': ChoiceField(type=str, choices=[
                'active', 'reached', 'future', 'failed',
            ]),
            'destination': bool,
            'a_arr':  DateTimeField(required=False),
            'a_dep':  DateTimeField(required=False),
        }

        super(GraphNode, self).__init__(*args, **kwargs)

    @classmethod
    def find_by_parent(cls, parent):
        return cls.find_one({'parent._id': parent._id, 'state': 'future'})

    def deactivate(self):
        # Update all children in future
        # Uses id to ensure children only given ordered nature of node
        # creation
        self.update(
            {'wbn': self.wbn, 'state': 'future', '_id': {'$gt': self._id}},
            {'$set': {'state': 'inactive'}}
        )
        self.state = 'failed'
        self.save()

    def reached(self):
        self.state = 'reached'
        self.save()

    def record_soft_failure_outscan(self, scan_datetime):
        self.a_dep = scan_datetime
        c = ConnectionFailure(connection=self.edge, fail_out=True)
        c.save()

    def update_parent(self, parent):
        self.parent = parent
        self.save()


class ConnectionFailure(BaseModel):
    '''
    Failures when connections connection late or arrived late

    The failure is considered hard only if the connection arrived late
    enough to impact connectivity with future connections.
    '''

    collection = 'failcon'

    def __init__(self, *args, **kwargs):
        self.structure = {
            'connection': ForeignKey(Connection),
            'fail_out': bool,
            'fail_in': bool,
            'hard': bool
        }

        super(ConnectionFailure, self).__init__(*args, **kwargs)


class CenterFailure:
    '''
    Records failures for a center when it is unable to connect to the right
    connection in time despite having the shipment arrived in a safe time for
    a future connection
    '''

    collection = 'epic_fail'

    def __init__(self, *args, **kwargs):
        self.structure = {
            'center': ForeignKey(DeliveryCenter),
            # Center failed to connect
            'cfail': bool,
            # Center misrouted
            'mroute': bool,
        }
        super(CenterFailure, self).__init__(*args, **kwargs)
