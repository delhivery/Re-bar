import time
import datetime

from orm.base import BaseField


class GenericField(BaseField):
    '''
    For base types such as int, str, float etc
    '''
    pass


class ChoiceField(BaseField):
    def __init__(self, *args, **kwargs):
        self.choices = kwargs.pop('choices', [])
        super(ChoiceField, self).__init__(*args, **kwargs)

        for choice in self.choices:
            if not isinstance(choice, self.f_type):
                raise TypeError()

        if not isinstance(self.default, (type(None), self.f_type)):
            raise TypeError()

    def validate(self):
        super(ChoiceField, self).validate()

        if self.value and self.value not in self.choices:
            raise ValueError()


class TimeField(BaseField):
    def __init__(self, *args, **kwargs):
        self.formats = kwargs.pop('formats', ['%H:%M:%S'])

        kwargs['type'] = datetime.time
        super(TimeField, self).__init__(*args, **kwargs)

    def __default__(self):

        if not isinstance(self.default, (type(None), self.f_type)):
            raise ValueError()

    def save_as(self):
        value = self.value_of()

        if value is not None:
            return (
                value.hour * 3600 + value.minute * 60 +
                value.second
            )

        return value

    def setval(self, value):

        if not isinstance(value, (type(None), self.f_type)):

            if isinstance(value, str):

                for time_format in self.formats:
                    try:
                        value = time.strptime(value, time_format)
                        self.value = datetime.time(
                            hour=value.tm_hour, minute=value.tm_min,
                            second=value.tm_sec
                        )
                        break
                    except ValueError:
                        continue

            elif isinstance(value, (int, float)):
                hours = int(value / 3600)
                minutes = int((value - 3600 * hours) / 60)
                seconds = int(value % 60)
                self.value = datetime.time(
                    hour=hours, minute=minutes, second=seconds
                )
        else:
            self.value = value
        self.validate()


class DateTimeField(BaseField):

    def __init__(self, *args, **kwargs):
        self.formats = kwargs.pop('formats', ['%Y-%m-%dT%H:%M:%S'])
        kwargs['type'] = datetime.datetime

        self.auto_add_now = kwargs.pop('auto_add_now', False)

        super(DateTimeField, self).__init__(*args, **kwargs)

    def __default__(self):

        if not isinstance(self.default, (type(None), self.f_type)):
            raise ValueError()

    def setval(self, value):

        if value is None and self.required is False:
            self.value = None
            return

        if not isinstance(value, self.f_type):

            if isinstance(value, str):
                for time_format in self.formats:
                    try:
                        value = time.strptime(value, time_format)
                        self.value = datetime.time(
                            hour=value.tm_hour, minute=value.tm_min,
                            second=value.tm_sec
                        )
                        self.validate()
                        return
                    except ValueError:
                        continue

            if value is None and self.auto_add_now:
                value = datetime.datetime.now()

        self.value = value
        self.validate()
