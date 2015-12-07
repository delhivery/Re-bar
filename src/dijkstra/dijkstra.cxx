#include <iostream>
#include <queue>
#include <math.h>

#include <dijkstra.hpp>

bool smaller(Cost first, Cost second) {
    if (second.second == std::numeric_limits<double>::infinity()) {
        return true;
    }

    return (first.first < second.first) ? true : ((first.first > second.first) ? false : (first.second < second.second));
}

std::vector<std::string> SimpleEP::EPGraph::time_dependent_shortest_path(
        std::string src, std::string dest, double t_start, double t_max) {

    if (vertex_map.find(src) == vertex_map.end()) {
        std::cout << "Unable to find source in vertex map" << std::endl;
    }

    if (vertex_map.find(dest) == vertex_map.end()) {
        std::cout << "Unable to find destination in vertex map" << std::endl;
    }

    Vertex source = vertex_map[src];
    Vertex destination = vertex_map[dest];
    Cost zero = std::pair<double, double>{t_start, 0};
    Cost inf = std::pair<double, double>{std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};

    DistanceMap distances(boost::num_vertices(g));
    PredecessorMap predecessors(boost::num_vertices(g));

    time_dependent_shortest_path(source, destination, distances, predecessors, smaller, inf, zero, t_max);

    std::vector<std::string> response = std::vector<std::string>{""};
    return response;
}

template <typename Compare> void SimpleEP::EPGraph::time_dependent_shortest_path(
        Vertex source, Vertex destination, DistanceMap& distances, PredecessorMap& predecessors, Compare& cmp, Cost inf, Cost zero, double t_max) {
    typedef typename boost::graph_traits<Graph>::vertex_iterator v_iter;
    typedef typename boost::graph_traits<Graph>::out_edge_iterator o_e_iter;

    std::vector<int> visited(boost::num_vertices(g));

    std::queue<Vertex> bin_heap;

    std::pair<v_iter, v_iter> vertices;
    o_e_iter e_iter, e_iter_end;

    for(vertices = boost::vertices(g); vertices.first != vertices.second; vertices.first++) {
        int v_index = *vertices.first;
        distances[v_index] = inf;
        visited[v_index] = 0;
    }

    distances[source] = zero;
    bin_heap.push(source);
    visited[source] = 1;

    while (!bin_heap.empty()) {
        Vertex current = bin_heap.front();
        bin_heap.pop();

        visited[current] = 2;

        for (tie(e_iter, e_iter_end) = boost::out_edges(current, g); e_iter != e_iter_end; e_iter++) {
            Connection conn = boost::get(boost::edge_bundle, g)[*e_iter];
            Vertex target = boost::target(*e_iter, g);

            Cost fresh = conn.weight(distances[current], t_max);

            if (fresh != inf) {
                if (visited[target] == 0) {
                    bin_heap.push(target);
                    distances[target] = fresh;
                    predecessors[target].first = current;
                    predecessors[target].second = conn;
                    visited[target] = 1;
                }
                else if (visited[target] == 1) {
                    if(cmp(fresh, distances[target])) {
                        distances[target] = fresh;
                        predecessors[target].first = current;
                        predecessors[target].second = conn;
                    }
                }
            }
        }
    }
}

std::pair<DistanceMap, PredecessorMap> SimpleEP::tdsp_wrapper(std::string src, std::string dst, double t_start, double t_max) {
    DistanceMap distances(boost::num_vertices(g));
    PredecessorMap predecessors(boost::num_vertices(g));

    if (vertex_map.find(src) == vertex_map.end()){
        std::cout << "Unable to find source " << src << " in vertex map" << std::endl;
        throw "No source matching code";
    }

    if (vertex_map.find(dst) == vertex_map.end()) {
        std::cout << "Unable to find destination " << dst << " in vertex map" << std::endl;
    }

    Vertex source = vertex_map[src];
    Vertex destination = vertex_map[dst];

    Cost zero = std::pair<double, double>(0, t_start);

    time_dependent_shortest_path(source, destination, distances, predecessors, smaller, UNRECHEABLE, zero, t_max);

    std::pair<DistanceMap, PredecessorMap> result;
    result.first = distances;
    result.second = predecessors;
    return result;
}

std::vector<std::pair<Cost, std::pair<std::string, std::string> > > SimpleEP::get_path(
        std::string src, std::string dst, double t_start, double t_max) {


    std::vector<std::pair<Cost, std::pair<std::string, std::string> > > path;
    std::pair<DistanceMap, PredecessorMap> result = tdsp_wrapper(src, dst, t_start, t_max);
    std::string code = dst;

    while(true) {
        std::pair<Cost, std::pair<std::string, std::string> > segment;
        segment.first = result.first[vertex_map[code]];

        std::pair<std::string, std::string> vertex_edge;
        vertex_edge.first = code;
        vertex_edge.second = result.second[vertex_map[code]].second.name;

        segment.second = vertex_edge;
        path.push_back(segment);

        if ((segment.first == UNRECHEABLE) || ( code == src)) {
            break;
        }
        code = g[result.second[vertex_map[code]].first].code;
    }
    return path;
}
