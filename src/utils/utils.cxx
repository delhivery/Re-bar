#include <cmath>
#include <iostream>
#include <utils.hpp>

double get_time(double datetime_as_double) {
    if (datetime_as_double == P_INF) {
        return datetime_as_double;
    }

    if (datetime_as_double == N_INF) {
        return datetime_as_double;
    }

    auto datetime = boost::posix_time::from_time_t(static_cast<double>(datetime_as_double));
    return static_cast<double>(datetime.time_of_day().total_seconds());
}

double abs_durinal(double time_as_double) {
    return time_as_double >= 0 ? fmod(time_as_double, HOURS_IN_DAY): time_as_double + HOURS_IN_DAY;
}

double wait_time(double t_init, double t_depart) {
    double t_durinal = fmod(t_init, HOURS_IN_DAY);
    return t_durinal <= t_depart ? t_depart - t_durinal: t_depart + HOURS_IN_DAY - t_durinal;
}

double time_from_string(std::string datestring) {
    std::istringstream iss{datestring};
    iss.imbue(std::locale(std::locale::classic(), new boost::posix_time::time_input_facet(1)));
    boost::posix_time::ptime datetime;
    iss >> datetime;

    boost::posix_time::ptime epoch{boost::gregorian::date(1970, 1, 1)};
    auto diff = datetime - epoch;
    return (diff.ticks() / boost::posix_time::time_duration::rep_type::ticks_per_second);
}
