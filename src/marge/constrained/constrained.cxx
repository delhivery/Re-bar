// Must be included before r_c_shortest_paths otherwise it complains of missing std::list
#include <constrained.hpp>

#include <iostream>
#include <boost/graph/r_c_shortest_paths.hpp>


Traversal::Traversal(double cost, double time) : cost(cost), time(time) {}

bool operator == (const Traversal& first, const Traversal& second) {
    return (first.cost == second.cost && first.time == second.time);
}

bool operator < (const Traversal& first, const Traversal& second) {
    if (first.cost > second.cost)
        return false;

    if (first.cost == second.cost)
        return first.time < second.time;

    return true;
}

TATExtension::TATExtension() {}

virtual std::vector<Path> ConstrainedEP::EPGraph::find_path(std::string src, std::string dest, double t_start, double t_max) {

    if (vertex_map.find(src) == vertex_map.end()) {
        throw std::invalid_argument("Unable to find source<" + src + "> in known vertices");
    }

    if (vertex_map.find(dest) == vertex_map.end()) {
        throw std::invalid_argument("Unable to find destination<" + dest + "> in known vertices");
    }

    Vertex source = vertex_map[src];
    Vertex destination = vertex_map[dest];

    std::vector<std::vector<boost::graph_traits<Graph>::edge_descriptor> > optimal_solutions;
    std::vector<Traversal> pareto_optimal_paths;

    boost::r_c_shortest_paths(
        g, get(&DeliveryCenter::index, g), get(&Connection::index, g),
        source, destination,
        optimal_solutions, pareto_optimal_paths, Traversal(0, t_start),
        TATExtension(t_max), DominanceTraversal(),
        std::allocator<boost::r_c_shortest_paths_label<Graph, Traversal> >(),
        boost::default_r_c_shortest_paths_visitor()
    );

    int num_optimal_solutions = static_cast<int>(optimal_solutions.size());
    std::vector<Path> path;

    if (num_optimal_solutions > 0) {
        auto optimal_solution = optimal_solutions[0];

        double cost = 0;
        double time = t_start;

        for (int i = static_cast<int>(optimal_solution.size()) - 1; i >= 0; --i) {
            auto optimal_segment = optimal_solution[i];
            Vertex target = boost::target(optimal_segment, g);
            Connection edge = boost::get(boost::edge_bundle, g)[optimal_segment];
            cost += edge.cost;
            time += edge.duration + edge.wait_time(time);
            path.push_back(Path{g[target].name, g[target].name, edge.name, time, cost});
        }
    }

    return path;
}
