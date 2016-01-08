#include <cmath>
#include <iostream>
#include <utils.hpp>
#include <algorithm>

long abs_durinal(long time) {
    return time >= 0 ? fmod(time, HOURS_IN_DAY) : time + HOURS_IN_DAY;
}

long wait_time(long t_init, long t_depart) {
    long t_durinal = fmod(t_init, HOURS_IN_DAY);
    return t_durinal <= t_depart ? t_depart - t_durinal: t_depart + HOURS_IN_DAY - t_durinal;
}

long time_from_string(std::string datestring) {
    std::replace(datestring.begin(), datestring.end(), 'T', ' ');
    boost::posix_time::ptime datetime = boost::posix_time::time_from_string(datestring);
    boost::posix_time::ptime epoch{boost::gregorian::date(1970, 1, 1)};
    auto diff = datetime - epoch;
    return diff.total_seconds() - 19800;
    // return (diff.ticks() / boost::posix_time::time_duration::rep_type::ticks_per_second);
}
