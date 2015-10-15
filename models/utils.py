'''
Utilities used by models exposed to store the graphs generated via EP
'''

def recurse_set_attribute(obj, attributes, value):
    '''
    Recursively iterate through the object to set the value against it
    Used when trying to set values in nested dictionaries via dot notation
    '''

    if len(attributes) == 1:
        obj[attributes[0]] = value
    else:
        recurse_set_attribute(
            obj[attributes[0]], attributes[1:], value
        )


def recurse_get_attribute(obj, attributes):
    '''
    Recursively iterate through the object to get the value against it
    Used when trying to get values in nested dictionaries via dot notation
    '''
    if not isinstance(attributes, list):
        raise ValueError()

    if not attributes:
        if isinstance(obj, dict) and not obj:
            return None
        return obj

    return recurse_get_attribute(
        obj.get(attributes[0], {}), attributes[1:]
    )
