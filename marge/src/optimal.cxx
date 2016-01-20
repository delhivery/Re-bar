#include <queue>
#include "optimal.hpp"

bool Compare::operator () (const std::pair<Vertex, Cost>& first, const std::pair<Vertex, Cost>& second) const {
    return first.second > second.second;
}

void Optimal::run_dijkstra(Vertex src, Vertex dst, DistanceMap& dmap, PredecessorMap& pmap, Cost inf, Cost zero, long t_max) {

    std::vector<int> visited(boost::num_vertices(g));
    std::priority_queue<std::pair<Vertex, Cost>, std::vector<std::pair<Vertex, Cost> >, Compare> bin_heap;

    for (auto vertices = boost::vertices(g); vertices.first != vertices.second; vertices.first++) {
        Vertex vertex = *vertices.first;
        dmap[vertex] = inf;
        visited[vertex] = 0;
    }

    dmap[src] = zero;

    for (auto vertices = boost::vertices(g); vertices.first != vertices.second; vertices.first++) {
        Vertex vertex = *vertices.first;
        bin_heap.push(make_pair(vertex, dmap[vertex]));
    }

    while (!bin_heap.empty()) {
        auto current = bin_heap.top();
        bin_heap.pop();

        if (dmap[current.first] == inf) {
            break;
        }

        if (current.first == dst) {
            break;
        }

        out_edge_iter e_iter, e_iter_end;

        for ( tie(e_iter, e_iter_end) = boost::out_edges(current.first, g); e_iter != e_iter_end; e_iter++) {
            EdgeProperty edge = g[*e_iter];
            Vertex target = boost::target(*e_iter, g);

            Cost edge_iterated = edge.weight(dmap[current.first], t_max);

            if (edge_iterated != inf) {
                if (visited[target] == 0) {
                    dmap[target] = edge_iterated;
                    pmap[target] = *e_iter;
                    bin_heap.push(make_pair(target, dmap[target]));
                    visited[target] = 1;
                }
                else if (visited[target] == 1) {
                    if (edge_iterated < dmap[target]) {
                        dmap[target] = edge_iterated;
                        pmap[target] = *e_iter;
                        bin_heap.push(make_pair(target, dmap[target]));
                    }
                }
            }
        }
    }
}

Optimal::Optimal(bool _ignore_cost) : ignore_cost(_ignore_cost) {}

void Optimal::add_edge(string_view src, string_view dst, string_view code, const long dep, const long dur, const long tip, const long tap, const long top, const double cost) {
    if (ignore_cost) {
        BaseGraph::add_edge(src, dst, code, dep, dur, tip, tap, top, 0);
    }
    else {
        BaseGraph::add_edge(src, dst, code, dep, dur, tip, tap, top, cost);
    }
}

std::vector<Path> Optimal::find_path(string_view src, string_view dst, const long t_start, const long t_max) {
    if (vertex_map.find(src) == vertex_map.end()) {
        throw std::invalid_argument("No source <> found");
    }

    if (vertex_map.find(dst) == vertex_map.end()) {
        throw std::invalid_argument("No destination<> found");
    }

    Vertex source = vertex_map.at(src);
    Vertex destination = vertex_map.at(dst);

    Cost zero = std::make_pair(0, t_start);
    Cost inf = std::make_pair(P_D_INF, P_L_INF);

    std::shared_lock<std::shared_timed_mutex> graph_read_lock(graph_mutex, std::defer_lock);
    graph_read_lock.lock();

    DistanceMap distances(boost::num_vertices(g));
    PredecessorMap predecessors(boost::num_vertices(g));

    run_dijkstra(source, destination, distances, predecessors, inf, zero, t_max);

    std::vector<Path> path;

    Vertex current = destination;
    Edge inbound;
    long departure = P_L_INF;
    bool first = true;

    do {
        auto distance = distances[current];

        if (distance.second == P_L_INF) {
            return path;
        }

        VertexProperty vprop = g[current];

        if(first) {
            path.push_back(Path{vprop.code, "", "", distance.second, departure, distance.first});
            first = false;
        }
        else {
            EdgeProperty eprop = g[inbound];
            departure = distance.second + eprop.wait_time(distance.second);
            path.push_back(Path{vprop.code, eprop.code, g[boost::target(inbound, g)].code, distance.second, departure, distance.first});
        }

        if (current == source) {
            break;
        }

        inbound = predecessors[current];
        current = boost::source(inbound, g);
    } while (true);
    return path;
}
