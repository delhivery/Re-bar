#ifndef DIJKSTRA_HPP_INCLUDED
#define DIJKSTRA_HPP_INCLUDED

#include "graph.hpp"

bool smaller(Cost first, Cost second);

class SimpleEP : public EPGraph {
    public:

        std::pair<DistanceMap, PredecessorMap> tdsp_wrapper(
            std::string src, std::string dst, double t_start, double t_max);

        std::vector<std::pair<Cost, std::pair<std::string, std::string> > > get_path(
            std::string src, std::string dst, double t_start, double t_max);
};

#endif
