#include <queue>
#include "optimal.hpp"

bool Compare::operator () (const pair<Vertex, Cost>& first, const pair<Vertex, Cost>& second) const {
    return first.second > second.second;
}

void Optimal::run_dijkstra(Vertex src, Vertex dst, DistanceMap& dmap, PredecessorMap& pmap, Cost inf, Cost zero, long t_max) {

    vector<int> visited(boost::num_vertices(g));
    priority_queue<pair<Vertex, Cost>, vector<pair<Vertex, Cost> >, Compare> bin_heap;

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
                    bool less = false;

                    if (ignore_cost) {
                        less = edge_iterated.second < dmap[target].second;
                    }
                    else {
                        less = edge_iterated < dmap[target];
                    }

                    if (less) {
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

vector<Path> Optimal::find_path(string_view src, string_view dst, long t_start, long t_max) {

    if (ignore_cost)
        t_max = P_L_INF;

    if (vertex_map.find(src) == vertex_map.end()) {
        throw invalid_argument("No source <> found");
    }

    if (vertex_map.find(dst) == vertex_map.end()) {
        throw invalid_argument("No destination<> found");
    }

    Vertex source = vertex_map.at(src.to_string());
    Vertex destination = vertex_map.at(dst.to_string());

    Cost zero = make_pair(0, t_start);
    Cost inf = make_pair(P_D_INF, P_L_INF);

    shared_lock<shared_timed_mutex> graph_read_lock(graph_mutex, defer_lock);
    graph_read_lock.lock();

    DistanceMap distances(boost::num_vertices(g));
    PredecessorMap predecessors(boost::num_vertices(g));

    run_dijkstra(source, destination, distances, predecessors, inf, zero, t_max);

    vector<Path> path;

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
            path.push_back(Path{g[current].code, "", "", distance.second, departure, distance.first});
            first = false;
        }
        else {
            EdgeProperty eprop = g[inbound];
            departure = distance.second + eprop.wait_time(distance.second);
            path.push_back(Path{g[current].code, g[inbound].code, g[boost::target(inbound, g)].code, distance.second, departure, distance.first});
        }

        if (current == source) {
            break;
        }

        inbound = predecessors[current];
        current = boost::source(inbound, g);
    } while (true);

    std::reverse(path.begin(), path.end());
    return path;
}
