#ifndef DIJKSTRA_HPP_INCLUDED
#define DIJKSTRA_HPP_INCLUDED

#include "graph.hpp"

bool smaller(Cost first, Cost second);

class SimpleEP : public EPGraph {
    private:
        template <typename Compare> void run_dijkstra(Vertex source, Vertex destination, DistanceMap& distances, PredecessorMap& predecessors, Compare& cmp, Cost inf, Cost zero, long t_max);
    public:
        ~SimpleEP();

        std::vector<Path> find_path(std::string src, std::string dst, long t_start, long t_max) override;
};

#endif
