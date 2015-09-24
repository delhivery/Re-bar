import datetime
import uuid


class BaseField:

    def __init__(self, *args, **kwargs):
        try:
            self.f_type = kwargs.get('type')
            self.required = kwargs.get('required', True)
            if not isinstance(self.required, bool):
                try:
                    self.required = bool(self.required)
                except TypeError:
                    self.required = True
        except KeyError:
            raise TypeError('Field type is required')
        self.value = None

    def validate(self):
        if self.required:

            if not isinstance(self.value, self.f_type):
                raise TypeError(
                    'Expected {}, {}:{} specified'.format(
                        self.f_type, type(self.value), self.value
                    )
                )
        elif self.value is not None:
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
        return self

    def serialize(self, recurse=True):
        return self.value


class TimeField(BaseField):
    def __init__(self, *args, **kwargs):
        super(TimeField, self).__init__(type=datetime.time, *args, **kwargs)

    def valueOf(self, value=None):
        if value is not None:
            if isinstance(value, int):
                hours = int(value / 3600)
                minutes = int((value - hours * 3600) / 60)
                seconds = value % 60
                value = datetime.time(hours, minutes, seconds)
            self.value = value

        if value is None and self.value is None:
            self.value = uuid.uuid4()

        self.validate()
        return self

    def serialize(self, recurse=True):
        return (
            self.value.hour * 3600 + self.value.minute * 60 +
            self.value.second
        )
