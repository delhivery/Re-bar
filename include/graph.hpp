#ifndef EXPATH_DEFINED
#define EXPATH_DEFINED

#include <iostream>
#include <utility>

#include "epgraph.hpp"

#ifdef BOOST_NO_EXCEPTIONS
    void boost::throw_exception(std::exception const& exc) {
            std::cout << exc.what() << std::endl;
    }
#endif

namespace expath {
    bool smaller(Cost first, Cost second);

    class Expath : public EPGraph {
        public:

            std::pair<DistanceMap, PredecessorMap> tdsp_wrapper(
                std::string src, std::string dst, double t_start, double t_max);

            std::vector<std::pair<Cost, std::pair<std::string, std::string> > > get_path(
                std::string src, std::string dst, double t_start, double t_max);
    };
}

#endif
