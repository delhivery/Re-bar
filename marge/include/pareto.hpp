#ifndef PARETO_HPP_DEFINED
#define PARETO_HPP_DEFINED

#include "graph.hpp"

struct Traversal {
    double cost;
    long time;

    Traversal(double _cost=0, long _time = 0);
    Traversal& operator = (const Traversal& other);
};

bool operator == (const Traversal& first, const Traversal& second);

bool operator < (const Traversal& first, const Traversal& second);

class TimeConstraint {
    private:
        long t_max;

    public:
        TimeConstraint();
        TimeConstraint(long _t_max=0);

        inline bool operator () (const Graph& g, Traversal& fresh, const Traversal& old, Edge edge) const;
};

class TraversalDominance {
    public:
        inline bool operator () (const Traversal& first, const Traversal& second) const;
};

class Pareto : public BaseGraph {
    public:
        vector<Path> find_path(string_view src, string_view dst, long t_start, long t_max);
};

#endif
