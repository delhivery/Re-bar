#include <iostream>
#include <queue>
#include <math.h>

#include <dijkstra.hpp>

bool smaller(Cost first, Cost second) {
    if (second.second == P_T_INF) {
        return true;
    }

    return (first.first < second.first) ? true : ((first.first > second.first) ? false : (first.second < second.second));
}

SimpleEP::~SimpleEP() {
}

template <typename Compare> void SimpleEP::run_dijkstra(
        Vertex source, Vertex destination, DistanceMap& distances, PredecessorMap& predecessors,
        Compare& cmp, Cost inf, Cost zero, long t_max) {
    typedef typename boost::graph_traits<Graph>::vertex_iterator v_iter;
    typedef typename boost::graph_traits<Graph>::out_edge_iterator o_e_iter;

    std::vector<int> visited(boost::num_vertices(g));

    std::queue<Vertex> bin_heap;

    std::pair<v_iter, v_iter> vertices;
    o_e_iter e_iter, e_iter_end;

    for(vertices = boost::vertices(g); vertices.first != vertices.second; vertices.first++) {
        int v_index = *vertices.first;
        distances[v_index] = inf;
        predecessors[v_index] = std::pair<Vertex, Connection>(source, Connection());
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
                    if (cmp(fresh, distances[target])) {
                        distances[target] = fresh;
                        predecessors[target].first = current;
                        predecessors[target].second = conn;
                    }
                }
            }
        }
    }
}

std::vector<Path> SimpleEP::find_path(std::string src, std::string dest, long t_start, long t_max) {
    if (vertex_map.find(src) == vertex_map.end()) {
        throw std::invalid_argument("Unable to find source<" + src + "> in known vertices");
    }

    if (vertex_map.find(dest) == vertex_map.end()) {
        throw std::invalid_argument("Unable to find destination<" + dest + "> in known vertices");
    }

    Vertex source = vertex_map[src];
    Vertex destination = vertex_map[dest];

    Cost zero = std::pair<double, long>{0, t_start};
    Cost inf = std::pair<double, long>{P_INF, P_T_INF};

    DistanceMap distances(boost::num_vertices(g));
    PredecessorMap predecessors(boost::num_vertices(g));

    run_dijkstra(source, destination, distances, predecessors, smaller, inf, zero, t_max);

    std::vector<Path> path;

    Vertex target = vertex_map[dest];
    Connection connection_outbound;

    do {
        auto distance = distances[target].second;
        auto distance_t = distances[target].first;

        if(distance == P_INF or distance_t == P_T_INF) {
            return path;
        }

        auto target_name = g[target].name;
        path.push_back(Path{target_name, connection_outbound, distance, distance_t});

        if (target == source)
            break;

        connection_outbound = predecessors[target].second;
        target = predecessors[target].first;
    } while (true); 
    return path;
}
