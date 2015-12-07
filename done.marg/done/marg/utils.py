'''
This module stores various utility functions used by done.marg
that are generic in nature
'''
import time
import datetime

from .config import DAY_IN_SECONDS, TIME_FORMAT


def time_to_seconds(t_time):
    '''
        Converts a datetime.time objects to seconds vs
        00:00:00
    '''
    if type(t_time) is datetime.time:
        return t_time.hour * 3600 + t_time.minute * 60 + t_time.second
    return t_time.tm_hour * 3600 + t_time.tm_min * 60 + t_time.tm_sec


def get_time_delta(start, end):
    '''
        Get difference in time between start and end
    '''
    start_as_seconds = time_to_seconds(start)
    end_as_seconds = time_to_seconds(end)
    if end_as_seconds >= start_as_seconds:
        return end_as_seconds - start_as_seconds
    delta_eod = DAY_IN_SECONDS - start_as_seconds
    return delta_eod + time_to_seconds(end)


def time_object(t_time):
    '''
    Converts time to a time object
    '''
    if isinstance(t_time, int):
        hour = int(t_time / 3600)
        minute = int((t_time - hour * 3600) / 60)
        second = t_time % 60
        return datetime.time(hour=hour, minute=minute, second=second)
    elif type(t_time) in [datetime.time, time.struct_time]:
        return t_time
    else:
        try:
            return time.strptime(t_time, TIME_FORMAT)
        except Exception:
            raise ValueError(
                'Unable to convert {} to a time object'.format(
                    time))


def find_path(reachability, source, target, cost_matrix):
    '''
    Extract the path from source to target given reachability matrix
    and cost matrix
    '''
    rows = []

    parent, edge = reachability[target]
    row = [parent, target, edge, cost_matrix[target]]

    if parent == source:
        rows.append(row)
    else:
        rows = rows + find_path(reachability, source, parent, cost_matrix)
    return rows
