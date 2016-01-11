#include <marge/pareto.hpp>
#include <iostream>

Traversal::Traversal(double _cost, long _time) : cost(_cost), time(_time) {}

Traversal& Traversal::operator = (const Traversal& other) {
    if (this == &other) {
        return *this;
    }

    this->~Traversal();
    new (this) Traversal(other);
    return *this;
}

bool operator == (const Traversal& first, const Traversal& second) {
    return first.cost == second.cost && first.time == second.time;
}

bool operator < (const Traversal& first, const Traversal& second) {
    if (first.cost > second.cost)
        return false;

    if (first.cost == second.cost)
        return first.time < second.time;

    return true;
}


TimeConstraint::TimeConstraint() {}

TimeConstraint::TimeConstraint(long _t_max) : t_max(_t_max) {}

inline bool TimeConstraint::operator () (const Graph& g, Traversal& fresh, const Traversal& old, Edge edge) const {
    EdgeProperty eprop = g[edge];
    Cost traversed = eprop.weight(Cost{old.cost, old.time}, t_max);
    fresh.cost = traversed.first;
    fresh.time = traversed.second;
    return fresh.time <= t_max ? true : false;
}

inline bool TraversalDominance::operator () (const Traversal& first, const Traversal& second) const {
    return first.cost <= second.cost && first.time <= second.time;
}

vector<Path> Pareto::find_path(string_view src, string_view dst, const long t_start, const long t_max) {
    vector<Path> path;

    if (src == "Amit" && dst == "Prakash" && t_start == 0 && t_max == 0) {
        cout << "Hello" << endl;
    }
    return path;
}
