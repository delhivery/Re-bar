#ifndef OPTIMAL_HPP_INCLUDED
#define OPTIMAL_HPP_INCLUDED

#include <marge/graph.hpp>

typedef vector<Cost> DistanceMap;
typedef vector<Edge> PredecessorMap;

typedef typename boost::graph_traits<Graph>::out_edge_iterator out_edge_iter;

struct Compare {
    public:
        bool operator () (const pair<Vertex, Cost>& first, const pair<Vertex, Cost>& second) const;
};

class Optimal : public BaseGraph {
    private:
        bool ignore_cost;
        void run_dijkstra(Vertex src, Vertex dst, DistanceMap& dmap, PredecessorMap& pmap, Cost inf, Cost zero, long t_max);
    public:
        Optimal() {}
        Optimal(bool _ignore_cost = false);
        void add_edge(string_view src, string_view dst, string_view code, const long dep, const long dur, const long tip, const long top, const long tap, const double cost);
        vector<Path> find_path(string_view src, string_view dst, const long t_start, const long t_max);
};

#endif
