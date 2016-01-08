#ifndef CONSTRAINED_HPP_INCLUDED
#define CONSTRAINED_HPP_INCLUDED

#include "graph.hpp"

struct Traversal {
    double cost;
    long time;

    Traversal(double cost = 0, long time = 0);

    Traversal& operator = (const Traversal& other) {
        if (this == &other) {
            return *this;
        }

        this->~Traversal();
        new (this) Traversal(other);
        return *this;
    }
};

class TATExtension {
    private:
        long t_max;

    public:
        TATExtension();

        TATExtension(long t_max=0) : t_max(t_max) {};

        inline bool operator() (
            const Graph& g, Traversal& fresh, const Traversal& old,
            boost::graph_traits<Graph>::edge_descriptor ed
        ) const {
            const Connection& conn = boost::get(boost::edge_bundle, g)[ed];
            fresh.cost = old.cost + conn.cost;
            fresh.time = old.time + conn.duration + conn.wait_time(old.time);
            return fresh.time <= t_max ? true : false;
        }
};

class DominanceTraversal {
    public:
        inline bool operator()(const Traversal& first, const Traversal& second) const {
            return first.cost <= second.cost && first.time <= second.time;
        }
};

class ConstrainedEP : public EPGraph {
    public:
        ~ConstrainedEP();

        std::vector<Path> find_path(std::string src, std::string dst, long t_start, long t_max) override;
};

#endif
