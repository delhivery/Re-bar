import importlib

from orm.base import BaseField


class ForeignKey(BaseField):

    def lazy_load(self):
        module_elements = self.f_type.split('.')
        module = '.'.join(module_elements[:-1])
        module = importlib.import_module(module)
        model = getattr(module, module_elements[-1])
        self.f_type = model

    def setval(self, value):

        if isinstance(self.f_type, str):
            self.lazy_load()

        if not isinstance(value, self.f_type):
            if value is not None:
                if isinstance(value, dict):
                    if '_id' in value:
                        value = {'_id': value['_id']}
                self.value = self.f_type.find_one(value)
            else:
                self.value = value
        else:
            self.value = value
        self.validate()

    def value_of(self):
        if isinstance(self.f_type, str):
            self.lazy_load()
        return super(ForeignKey, self).value_of()

    def save_as(self):
        return self.value.save(save=False)


class ForeignOidKey(ForeignKey):

    def value_of(self):
        ret = super(ForeignOidKey, self).value_of()

        if ret is not None:
            ret = {'_id': ret['_id']}

        return ret

    def save_as(self):
        return self.value_of()