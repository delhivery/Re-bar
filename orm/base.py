class BaseField:

    def __init__(self, *args, **kwargs):
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
        if not isinstance(self.value, self.f_type):
            if (
                self.value is None and self.required) or (
                self.value is not None
            ):
                raise TypeError()

    def reset(self):
        if self.default is not None:
            self.value = self.default
        else:
            self.value = None

    def value_of(self):
        self.validate()
        return self.value

    def save_as(self):
        return self.value_of()

    def setval(self, value):
        self.value = value
        self.validate()
