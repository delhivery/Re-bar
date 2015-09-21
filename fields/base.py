import uuid


class BaseField:

    def __init__(self, *args, **kwargs):
        try:
            self.f_type = kwargs.get('type')
        except KeyError:
            raise TypeError('Field type is required')
        self.value = None

    def validate(self):
        if not isinstance(self.value, self.f_type):
            raise TypeError(
                'Expected {}, {}:{} specified'.format(
                    self.f_type, type(self.value), self.value
                )
            )


class ChoiceField(BaseField):

    def __init__(self, *args, **kwargs):

        super(ChoiceField, self).__init__(*args, **kwargs)
        self.choices = kwargs.get('choices', [])

        for choice in self.choices:
            if not isinstance(choice, self.f_type):
                raise TypeError(
                    'Choices should be of type {}: '
                    'Choice {}:{} specified'.format(
                        self.f_type, type(choice), choice
                    )
                )

    def valueOf(self, value=None):

        if value:

            if value not in self.choices:
                raise ValueError(
                    'Field accepts only {} as choices, {} specified'.format(
                        self.choices, value
                    )
                )
            self.value = value

        if value is None and self.value is None:
            self.value = self.f_type()
        self.validate()
        return self.value


class AutoField(BaseField):
    def __init__(self):
        super(AutoField, self).__init__(type=uuid.UUID)

    def valueOf(self, value=None):
        if value:
            if isinstance(value, str):
                value = uuid.UUID(value)
            self.value = value

        if value is None and self.value is None:
            self.value = uuid.uuid4()

        self.validate()
        return self.value
