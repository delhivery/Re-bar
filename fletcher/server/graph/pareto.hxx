//
// Created by amitprakash on 8/1/16.
//

#ifndef FLETCHER_PARETO_HXX
#define FLETCHER_PARETO_HXX

#include <tuple>
#include <vector>

#include <experimental/string_view>

#include <boost/graph/r_c_shortest_paths.hpp>

#include "graph_helpers.hxx"

template <typename Cost>
struct Resource {
    Cost cost;

    Resource() = default;

    Resource(Cost _cost=Cost{0, 0}) : cost(_cost) {}

    Resource& operator = (Resource other) {
        std::swap(cost, other.cost);
        return *this;
    }
};

template <typename Cost>
class ResourceExtension {
private:
    Cost max;
public:
    ResourceExtension() = default;

    ResourceExtension(Cost max_ = Cost{0, 0}) : max(max_) {}

    inline bool operator() (const auto& g, Resource<Cost>& next, const Resource<Cost>& prev, auto edge) const {
        auto e_prop = g[edge];
        Cost distance = e_prop.weight(prev.cost, max);
        next.cost = distance;
        return next.cost <= max;
    }
};

template <typename Cost>
class ResourceDominance {
public:
    inline bool operator() (const Resource<Cost>& first, const Resource<Cost>& second) const {
        return first.cost < second.cost;
    }
};

template <typename Graph, typename Segment>
std::vector<std::vector<Segment> >
parse_solution(
        auto solutions
) {
    auto solution = std::vector<Segment>{};
    std::vector<std::vector<Segment> > all;
    all.push_back(solution);
    return all;
};

template <typename Graph, typename Segment, typename Cost>
std::vector<std::vector<Segment> >
find_constrained(
        std::experimental::string_view src,
        std::experimental::string_view dst,
        Cost arr,
        long m_dur,
        const Graph& g) {
    typedef typename boost::graph_traits<Graph>::edge_descriptor Edge;
    typedef typename boost::graph_traits<Graph>::vertex_descriptor Vertex;
    typedef typename boost::r_c_shortest_paths_label<Graph, Resource<Cost> > Label;
    Vertex source, destination;
    bool src_exists, dst_exists;
    std::tie(source, src_exists)= get_vertex_by_code(src, g);
    std::tie(destination, dst_exists) = get_vertex_by_code(dst, g);

    if (!src_exists) {
        throw "Invalid source specified";
    }

    if (!dst_exists) {
        throw "Invalid destination specified";
    }

    typename std::vector<std::vector<Edge> > solutions;
    std::vector<Resource<Cost> > pareto_optimal_resource_container;
    Resource<Cost> resource_initial{arr};
    ResourceExtension<Cost> resource_extension_functor{Cost{std::numeric_limits<double>::infinity(), m_dur}};
    ResourceDominance<Cost> dominance_functor;

    boost::r_c_shortest_paths(
            g,
            get(Graph::vertex_index_t, g),
            get(Graph::edge_index_t, g),
            source,
            destination,
            solutions,
            pareto_optimal_resource_container,
            resource_initial,
            resource_extension_functor,
            dominance_functor,
            std::allocator<Label> (),
            boost::default_r_c_shortest_paths_visitor()
    );
    return solutions;
};
#endif //FLETCHER_PARETO_HXX
