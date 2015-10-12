import sys

from .utils import recurse_get_attribute, recurse_set_attribute

from bson import ObjectId

from mongo import DBConnection

from orm.fields import GenericField, TimeField, ChoiceField, DateTimeField
from orm.relational import ForeignKey, ForeignOidKey


class BaseModel(dict):
    __collection__ = None
    __database__ = 'rebar'
    __auth__ = None

    __unique_keys__ = []

    structure = {}

    @classmethod
    def get_connection(cls):

        connect = DBConnection(cls.__database__)

        if isinstance(cls.__auth__, tuple):
            connect.authenticate(cls.__auth__[0], cls.__auth__[1])

        if cls.__collection__:
            connect = connect[cls.__collection__]

        return connect

    @classmethod
    def find_one(cls, filter_dict):

        if isinstance(filter_dict, (str, ObjectId)):
            if isinstance(filter_dict, str):
                filter_dict = {'_id': ObjectId(filter_dict)}
            else:
                filter_dict = {'_id': filter_dict}

        if not isinstance(filter_dict, dict):
            raise ValueError()

        connection = cls.get_connection()
        try:
            return cls(**connection.find_one(filter_dict))
        except TypeError:
            return None

    @classmethod
    def find(cls, filter_dict):

        if not isinstance(filter_dict, dict):
            raise ValueError()

        connection = cls.get_connection()
        results = connection.find(filter_dict)

        for result in results:
            yield cls(**result)

    @classmethod
    def count(cls, filter_dict):
        if not isinstance(filter_dict, dict):
            raise ValueError()

        connection = cls.get_connection()
        return connection.find(filter_dict).count()

    @classmethod
    def all(cls):
        return cls.find({})

    @classmethod
    def update(cls, filter_dict, update_dict, multi=True):

        if not isinstance(filter_dict, dict):
            raise ValueError()

        if not isinstance(update_dict, dict):
            raise ValueError()

        connection = cls.get_connection()
        connection.update(filter_dict, update_dict, multi=multi)

    def __init__(self, *args, **kwargs):

        for key, key_type in self.structure.items():
            try:
                key_type.reset()
                value = None

                if key in kwargs:
                    value = kwargs.pop(key)
                    key_type.setval(value)

                self[key] = key_type.value_of()
            except (TypeError, ValueError) as err:
                print('Error in setting value {} for key {}: '.format(
                    value, key, err
                ), file=sys.stderr)
                raise

        for key in kwargs.keys():
            self[key] = kwargs[key]

        super(BaseModel, self).__init__(*args, **kwargs)

    def __setattr__(self, attribute, value):
        attributes = attribute.split('.')
        recurse_set_attribute(self, attributes, value)

    def __getattr__(self, attribute):
        attributes = attribute.split('.')
        return recurse_get_attribute(self, attributes)

    def save(self, save=True):
        connection = self.get_connection()

        data = {}

        for key, key_type in self.structure.items():

            if key in self:
                key_type.setval(self[key])

            data[key] = key_type.save_as()

        for key, value in self.items():
            if key not in data:
                data[key] = value

        if save:

            if '_id' in self:
                connection.update_one(
                    {'_id': self['_id']},
                    {'$set': data}, upsert=True
                )
            else:
                result = connection.insert_one(data)
                self['_id'] = result.inserted_id
        else:
            return data


class DeliveryCenter(BaseModel):
    __collection__ = 'nodes'
    __unique_keys__ = [('code', )]

    structure = {
        'code': GenericField(type=str),
        'name': GenericField(type=str, required=False),
        'active': GenericField(type=bool)
    }


class Connection(BaseModel):

    __collection__ = 'edges'
    __unique_keys__ = [('index', )]

    structure = {
        'name': GenericField(type=str),
        'origin': ForeignKey(type=DeliveryCenter),
        'destination': ForeignKey(type=DeliveryCenter),
        'departure': TimeField(),
        'duration': GenericField(type=int),
        'active': GenericField(type=bool),
        'mode': ChoiceField(type=str, choices=[
            'Local', 'Surface', 'Railroad', 'Air'
        ]),
        'index': GenericField(type=int),
    }


class GraphNode(BaseModel):
    __collection__ = 'paths'
    __unique_keys__ = []

    structure = {
        'wbn': GenericField(type=str),
        'pd': DateTimeField(required=False),
        'vertex': ForeignKey(type=DeliveryCenter, required=False),
        'edge': ForeignKey(type=Connection, required=False),
        'parent': ForeignOidKey(type='models.base.GraphNode', required=False),
        'e_arr': DateTimeField(required=False),
        'e_dep': DateTimeField(required=False),
        'a_arr': DateTimeField(required=False),
        'a_dep': DateTimeField(required=False),
        'dst': GenericField(type=bool),
        'st': ChoiceField(type=str, choices=[
            'reached', 'active', 'failed', 'future', 'inactive'
        ]),  # Statuses
        'f_at': ChoiceField(type=str, choices=[
            'center', 'cin', 'cout'
        ]),  # Failure At: Center, Connection In/Out
        'stc': ChoiceField(type=str, choices=[
            'regen', 'dmod', 'hard', 'soft', 'mroute'
        ]),  # Status Cause: Regen, Destination Mod, Hard/Soft Fail
        'cr_at': DateTimeField(auto_add_now=True, required=True),
    }

    @classmethod
    def find_by_parent(cls, parent):
        return cls.find_one({'parent._id': parent._id, 'st': 'future'})

    def deactivate(self, stc=None, f_at=None):
        self['st'] = 'failed'

        if stc:
            self['stc'] = stc

        if f_at:
            self['f_at'] = f_at

        self.save()
        self.update(
            {'_id': {'$gt': self['_id']}},
            {'$set': {'st': 'inactive', 'dst': False}}
        )

    def reached(self, stc=None, f_at=None):
        self['st'] = 'reached'

        if stc:
            self['stc'] = stc

        if f_at:
            self['f_at'] = f_at
        self.save()

    def activate(self):
        self['st'] = 'active'
        self.save()
