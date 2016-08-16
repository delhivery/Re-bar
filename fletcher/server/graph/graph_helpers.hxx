//
// Created by amitprakash on 8/1/16.
//

#ifndef FLETCHER_GRAPH_HELPERS_HXX
#define FLETCHER_GRAPH_HELPERS_HXX

#include <experimental/string_view>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>


template<typename Graph>
auto get_vertex_by_code(
        std::experimental::string_view code,
        const Graph& g) noexcept {
    //ToDo Read lock
    const auto iter = boost::vertices(g);
    const auto vertex = std::find_if(
            iter.first, iter.second,
            [g, code] (const typename boost::graph_traits<Graph>::vertex_descriptor v) {
                const auto vertex_name_map = get(boost::vertex_name, g);
                return get(vertex_name_map, v) == code;
            }
    );
    //ToDo Read unlock

    return (vertex != iter.second) ? std::make_pair(*vertex, true) : std::make_pair(*vertex, false);
}

template <typename Graph>
auto
get_edge_by_code(
        std::experimental::string_view code,
        const Graph& g) noexcept {
    //ToDo Read lock
    const auto iter = boost::edges(g);
    const auto edge = std::find_if(
            iter.first, iter.second, [g, code](const typename boost::graph_traits<Graph>::edge_descriptor e) {
                return g[e].code == code;
            }
    );
    //ToDo Read unlock

    return (edge != iter.second) ? std::make_pair(*edge, true) : std::make_pair(*edge, false);
}

template <typename Graph>
typename Graph::vertex_descriptor
add_center(
        std::experimental::string_view code,
        Graph& g) noexcept {
    typename boost::graph_traits<Graph>::vertex_descriptor vertex;
    bool vertex_exists;
    std::tie(vertex, vertex_exists) = get_vertex_by_code(code, g);

    if (vertex_exists) {
        return vertex;
    }

    //ToDo Write lock
    const auto vertex_d = boost::add_vertex(g);
    auto vertex_name_map = get(boost::vertex_name, g);
    put(vertex_name_map, vertex_d, code.to_string());
    //ToDo Write unlock
    return vertex_d;
}

template <typename Graph, typename EdgeProperty, typename Features>
typename Graph::edge_descriptor
add_connection(
        std::experimental::string_view code,
        std::experimental::string_view src,
        std::experimental::string_view dst,
        const long tap,
        const long top,
        const long tip,
        const double cost,
        Features& features,
        Graph& g) {
    auto edge = get_edge_by_code(code, g);

    if (edge.second) {
        //ToDo Write lock
        boost::remove_edge(edge.first, g);
        //ToDo Write unlock
    }

    EdgeProperty e{tap, top, tip, cost, code, features};
    e.percon = true;
    auto source = add_center(src, g);
    auto destination = add_center(dst, g);
    //ToDo Write lock
    auto edge_d = boost::add_edge(source, destination, e, g);
    //ToDo Write unlock
    return edge_d.first;
}

template <typename Graph, typename EdgeProperty, typename Features>
typename Graph::edge_descriptor
add_connection(
        std::experimental::string_view code,
        std::experimental::string_view src,
        std::experimental::string_view dst,
        const long dep,
        const long dur,
        const long tap,
        const long top,
        const long tip,
        const double cost,
        Features& features,
        Graph& g) {
    auto edge = get_edge_by_code(code, g);

    if (edge.second) {
        //ToDo Write lock
        boost::remove_edge(edge.first, g);
        //ToDo Write unlock
    }
    EdgeProperty e{dep, dur, tap, top, tip, cost, code, features};
    auto source = add_center(src, g);
    auto destination = add_center(dst, g);

    //ToDo Write lock
    auto edge_d = boost::add_edge(source, destination, e, g);
    //ToDo Write unlock
    return edge_d.first;
}

template <typename G>
void
remove_connection(
        std::experimental::string_view code,
        G& g
) {
    auto edge = get_edge_by_code(code, g);

    if (edge.second) {
        //ToDo Write lock
        boost::remove_edge(edge.first, g);
        //ToDo Write unlock
    }
}

template <typename EdgeFeatureMap, typename F>
struct
filtered_edge {
    F filters;
    EdgeFeatureMap e_map;

    filtered_edge() = default;

    filtered_edge(EdgeFeatureMap e_map_, F filters_) : e_map(e_map_), filters(filters_) {}

    template <typename E> bool operator()(const E& e) {
        auto e_feat = get(e_map, e);

        for(auto it: filters) {
            auto value_it = e_feat.find(it.first);

            if (value_it == e_feat.end()) {
                return false;
            }
            else if ((*value_it).second != it.second) {
                return false;
            }
        }
        return true;
    }
};

template <typename EdgeFeatureMap, typename F>
filtered_edge<EdgeFeatureMap, F> make_filtered_edge_predicate(EdgeFeatureMap map, F filters) {
    return filtered_edge<EdgeFeatureMap, F>(map, filters);
};

template <typename G, typename EdgePredicate>
boost::filtered_graph<G, EdgePredicate> get_filtered_graph(G& g, EdgePredicate ep) {
    //ToDo Read lock
    return boost::filtered_graph<G, EdgePredicate>(g, ep);
    //ToDo Read unlock
};

#endif //FLETCHER_GRAPH_HELPERS_HXX
