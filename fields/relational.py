import importlib

from .base import BaseField


class ForeignKey(BaseField):

    def __init__(self, fkey_to):

        if isinstance(fkey_to, str):
            self.fkey_to = fkey_to
            self.lazy_load = True
        else:
            fkey_to = fkey_to
            super(ForeignKey, self).__init__(type=fkey_to)

    def _lazy_load(self):
        self.lazy_load = False
        fullpath = self.fkey_to.split('.')
        module_path = '.'.join(fullpath[:-1])
        attr_path = fullpath[-1]
        module = importlib.import_module(module_path)
        f_type = getattr(module, attr_path)
        del(self.fkey_to)
        super(ForeignKey, self).__init__(type=f_type)

    def valueOf(self, value=None):
        if self.lazy_load:
            self._lazy_load()

        if value:
            self.value = self.f_type.get(value)

        if value is None and self.value is None:
            raise TypeError(
                'Missing value for ForeignKey: {}'.format(self.fkey_to)
            )

        self.validate()
        return self.value
