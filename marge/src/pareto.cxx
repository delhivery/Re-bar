// Include list - missing in BGL-rcsp
#include <list>
#include <boost/graph/r_c_shortest_paths.hpp>

#include "pareto.hpp"

template <typename T> struct reversion_wrapper { T& iterable; };
template <typename T> auto begin (reversion_wrapper<T> w) { return std::rbegin(w.iterable); }
template <typename T> auto end (reversion_wrapper<T> w) { return std::rend(w.iterable); }
template <typename T> reversion_wrapper<T> reverse (T&& iterable) { return { iterable }; }

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

vector<Path> Pareto::find_path(string_view src, string_view dst, long t_start, long t_max) {
    vector<Path> path;
    if (vertex_map.find(src) == vertex_map.end()) {
        throw invalid_argument("Invalid source");
    }

    if (vertex_map.find(dst) == vertex_map.end()) {
        throw invalid_argument("Invalid destination");
    }

    Vertex source = vertex_map.at(src.to_string());
    Vertex destination = vertex_map.at(dst.to_string());

    vector<vector<Edge> > optimal_solutions;
    vector<Traversal> pareto_optimal_paths;

    shared_lock<shared_timed_mutex> graph_read_lock(graph_mutex, defer_lock);
    graph_read_lock.lock();

    boost::r_c_shortest_paths(
        g, get(&VertexProperty::index, g), get(&EdgeProperty::index, g),
        source, destination, optimal_solutions, pareto_optimal_paths,
        Traversal(0, t_start), TimeConstraint(t_max), TraversalDominance(),
        allocator<boost::r_c_shortest_paths_label<Graph, Traversal> >(),
        boost::default_r_c_shortest_paths_visitor()
    );

    cout << "Source, Connection, Destination, Arrival, Departure, Cost" << endl;
    for (auto const& solution: optimal_solutions) {
        Cost current{0, 0};
        Vertex source, target;
        EdgeProperty eprop;

        for (auto const& edge: reverse(solution)) {
            source = boost::source(edge, g);
            target = boost::target(edge, g);
            eprop = g[edge];
            cout << "Pushing segment: " << g[source].code << ", " << eprop.code << ", " << g[target].code << ", " << current.second  << ", "<< eprop.wait_time(current.second) + current.second  << ", "<< current.first << endl;
            path.push_back(Path{g[source].code, g[edge].code, g[target].code, current.second, eprop.wait_time(current.second) + current.second, current.first});
            current = eprop.weight(current, t_max);
        }
        cout << "Pushing segment: " << g[target].code << ", , , " << current.second  << ", "<< P_L_INF  << ", "<< current.first << endl;
        path.push_back(Path{g[target].code, "", "", current.second, P_L_INF, current.first});
        break;
    }

    return path;
}
