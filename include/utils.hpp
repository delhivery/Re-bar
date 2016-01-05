#include <boost/date_time.hpp>
#include <limits>
#include <constants.hpp>

double get_time(double datetime_as_double); 

double abs_durinal(double time_as_double);

double wait_time(double t_init, double t_depart);

double time_from_string(std::string datestring);
