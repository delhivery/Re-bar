'''
This module exposes the base field for base data types and its acceessors
'''


class BaseField:
    '''
    This class represents the base field for in-built data types in python
    '''

    def __init__(self, **kwargs):
        self.f_type = kwargs.get('type', type(None))
        self.required = kwargs.get('required', True)
        self.default = kwargs.get('default', None)
        self.__default__()
        self.value = self.default

    def __default__(self):
        if isinstance(self.default, (type(None), self.f_type)):
            if self.default is None and self.required:
                try:
                    self.default = self.f_type()
                except (ValueError, TypeError):
                    self.default = None
        else:
            raise ValueError

    def validate(self):
        '''
        Validate the value set against the field
        '''
        if not isinstance(self.value, self.f_type):
            if (self.value is None and self.required) or (
                    self.value is not None):
                raise TypeError()

    def reset(self):
        '''
        Reset the value against the field
        '''
        if self.default is not None:
            self.value = self.default
        else:
            self.value = None

    def value_of(self):
        '''
        Retrieve the value against the field
        '''
        self.validate()
        return self.value

    def save_as(self):
        '''
        Converts python data type to db supported data type
        '''
        return self.value_of()

    def setval(self, value):
        '''
        Sets the value for the field
        '''
        self.value = value
        self.validate()
