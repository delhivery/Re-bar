// Include list - missing in BGL-rcsp
#include <list>
#include <boost/graph/r_c_shortest_paths.hpp>

#include "pareto.hpp"

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

std::vector<Path> Pareto::find_path(std::experimental::string_view src, std::experimental::string_view dst, const long t_start, const long t_max) {
    std::vector<Path> path;
    if (vertex_map.find(src) == vertex_map.end()) {
        throw std::invalid_argument("Invalid source");
    }

    if (vertex_map.find(dst) == vertex_map.end()) {
        throw std::invalid_argument("Invalid destination");
    }

    Vertex source = vertex_map.at(src);
    Vertex destination = vertex_map.at(dst);

    std::vector<std::vector<Edge> > optimal_solutions;
    std::vector<Traversal> pareto_optimal_paths;

    std::shared_lock<std::shared_timed_mutex> graph_read_lock(graph_mutex, std::defer_lock);
    graph_read_lock.lock();

    boost::r_c_shortest_paths(
        g, get(&VertexProperty::index, g), get(&EdgeProperty::index, g),
        source, destination, optimal_solutions, pareto_optimal_paths,
        Traversal(0, t_start), TimeConstraint(t_max), TraversalDominance(),
        std::allocator<boost::r_c_shortest_paths_label<Graph, Traversal> >(),
        boost::default_r_c_shortest_paths_visitor()
    );

    for (auto const& solution: optimal_solutions) {
        Cost current{0, t_max};
        Vertex source, target;
        EdgeProperty eprop;

        for (auto const& edge: solution) {
            source = boost::source(edge, g);
            target = boost::target(edge, g);
            eprop = g[edge];
            path.push_back(Path{g[source].code, eprop.code, g[target].code, current.second, eprop.wait_time(current.second) + current.second, current.first});
            current = eprop.weight(current, t_max);
        }
        path.push_back(Path{g[target].code, "", "", current.second, P_L_INF, current.first});
        break;
    }

    return path;
}
