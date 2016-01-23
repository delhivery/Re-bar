/** @file optimal.hpp
 * @brief Defines a single objective path solver.
 * @details Defines a graph iterator to find a path from a source to destination while optimizing on a singular value without any additional constraints (least time/cost) etc.
 */
#ifndef OPTIMAL_HPP_INCLUDED
#define OPTIMAL_HPP_INCLUDED

#include "graph.hpp"

typedef vector<Cost> DistanceMap;
typedef vector<Edge> PredecessorMap;

typedef typename boost::graph_traits<Graph>::out_edge_iterator out_edge_iter;

/**
 * @brief Comparison operator to compare a pair of vertices and their associated costs
 */
struct Compare {
    public:
        /**
         * @brief Function operator to compare pair of vertices and their associated costs
         * @param[in] : First pair of Vertex with associated Cost
         * @param[in] : Second pair of Vertex with associated Cost
         * @return True if first < second else false
         */
        bool operator () (const pair<Vertex, Cost>&, const pair<Vertex, Cost>&) const;
};

/**
 * @brief Extends BaseGraph to implement a single criteria path optimization
 */
class Optimal : public BaseGraph {
    private:
        /**
         * @brief Boolean to handle optimization on time(True) or cost(False)
         */
        bool ignore_cost;

        /**
         * @brief Actual implementation of the path finding algorithm as a dijkstra
         * @param[in] :         Source vertex
         * @param[in] :         Destination vertex
         * @param[in,out] :     Map of vertices to their distances from source
         * @param[in, out] :    Map of vertices to their predecessor when iterating from source
         * @param[in] :         Infinite Cost
         * @param[in] :         Zero/Base Cost
         * @param[in] :         Maximum duration by which the destination vertex must be reached
         */
        void run_dijkstra(Vertex, Vertex, DistanceMap&, PredecessorMap&, Cost, Cost, long);
    public:
        /**
         * @brief Default constructs the solver
         * @param[in] : Optional parameter to specify optimization on time or cost. Defaults to time
         */
        Optimal(bool = false);

        /**
         * @brief Implementation of path finder declared in BaseGraph
         * @param[in] : Name of source vertex
         * @param[in] : Name of destination vertex
         * @param[in] : Time of arrival at source vertex
         * @param[in] : Time limit by which destination vertex needs to be arrived at
         */
        vector<Path> find_path(string_view, string_view, long, long);
};

#endif
