#include <boost/date_time.hpp>
#include <limits>

double get_time(double datetime_as_double) {
    if (datetime_as_double == std::numeric_limits<double>::infinity()) {
        return datetime_as_double;
    }

    if (datetime_as_double == std::numeric_limits<double>::infinity() * -1) {
        return datetime_as_double;
    }

    auto datetime = boost::posix_time::from_time_t(static_cast<double>(datetime_as_double));
    return static_cast<double>(datetime.time_of_day().seconds());
}
