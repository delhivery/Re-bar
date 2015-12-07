#include "rcsp.hpp"

#include <iostream>
#include <boost/config.hpp>
#include <boost/graph/r_c_shortest_paths.hpp>

namespace expath {
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

    std::vector<std::string> RCExpath::EPGraph::time_dependent_shortest_path(std::string src, std::string dest, double t_start, double tmax) {
        std::vector<std::string> result;

        if (vertex_map.find(src) == vertex_map.end()) {
            std::cout << "Unable to find source<" << src << "> in vertex map" << std::endl;
        }
        if (vertex_map.find(dest) == vertex_map.end()) {
            std::cout << "Unable to find destination<" << dest << "> in vertex map" << std::endl;
        }

        Vertex source = vertex_map[src];
        Vertex destination = vertex_map[dest];

        std::vector<std::vector<boost::graph_traits<Graph>::edge_descriptor> > optimal_solutions;
        std::vector<Traversal> pareto_optimal_paths;

        boost::r_c_shortest_paths(
            g, get(&DeliveryCenter::index, g), get(&Connection::index, g), source,
            destination, optimal_solutions, pareto_optimal_paths, Traversal(0, t_start),
            TATExtension(tmax), DominanceTraversal(),
            std::allocator<boost::r_c_shortest_paths_label<Graph, Traversal> >(),
            boost::default_r_c_shortest_paths_visitor()
        );

        for (int i = 0; i < static_cast<int>(optimal_solutions.size()) and i < 1; ++i) {
            double cost = 0;
            double time = t_start;

            for (int j = static_cast<int>(optimal_solutions[i].size()) - 1; j >= 0; --j) {
                Vertex target = boost::target(optimal_solutions[i][j], g);
                Connection edge = boost::get(boost::edge_bundle, g)[optimal_solutions[i][j]];
                cost += edge.cost;
                time += edge.duration + edge.wait_time(time);
                std::ostringstream response;
                response << g[target].name << "," << edge.name << "," << cost << "," << time;
                result.push_back(response.str());
            }
        }

        return result;
    }

}
