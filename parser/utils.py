def recurse_set_attr(attributes, obj, value):
    if len(attributes) == 1:
        attributes = attributes[0]

    if isinstance(attributes, list):
        attr = attributes[0]

        if not isinstance(obj.get(attr, None), dict):
            obj[attr] = dict()

        recurse_set_attr(attributes[1:], obj[attr])
    else:
        obj[attributes] = value


def recurse_get_attr(attributes, obj):
    if len(attributes) == 1:
        attributes = attributes[0]

    if isinstance(attributes, list):
        attr = attributes[0]
        return recurse_get_attr(attributes[1:], obj.get(attr, {}))
    else:
        return obj.get(attributes, None)
