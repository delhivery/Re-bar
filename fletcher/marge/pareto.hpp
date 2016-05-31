/** @file pareto.hpp
 * @brief Defines a multi objective path solver.
 * @details Defines a graph iterator to find a path from a source to destination while optimizing on a singular value without additional constraints on other parameters.
 */
#ifndef PARETO_HPP_DEFINED
#define PARETO_HPP_DEFINED

#include "graph.hpp"

/**
 * @brief Structure to hold cost of traversal to a Vertex
 */
struct Traversal {
    /**
     * @brief Cost of traversal
     */
    double cost;

    /**
     * @brief Time of traversal
     */
    long time;

    /**
     * @brief Default constucts a Traversal
     * @param[in] : Optional cost of traversal. Defaults to 0
     * @param[in] : Optional time of traversal. Defaults to 0
     */
    Traversal(double = 0, long = 0);

    /**
     * @brief Equality constructor to construct a traversal object from another
     * @param[in] : Constant reference to the traversal object being copied.
     */
    Traversal& operator = (const Traversal&);
};

/**
 * @brief Equality operator to compare if two Traversal objects are equal
 * @param[in] : LHS Argument to compare
 * @param[in] : RHS Argument to compare
 * @return True if LHS == RHS else False
 */
bool operator == (const Traversal&, const Traversal&);

/**
 * @brief Comparison operator to compare if one Traversal is less than another
 * @param[in] : LHS Argument to compare
 * @param[in] : RHS Argument to compare
 * @return True if LHS < RHS else False
 */
bool operator < (const Traversal&, const Traversal&);

/**
 * @brief Structure to enfore constraints on time parameter
 */
class TimeConstraint {
    private:
        /**
         * @brief The maximum time at which all vertices in the recommended solution(s) should be reached
         */
        long t_max;

    public:
        /**
         * @brief Default constructs a TimeConstraint
         */
        TimeConstraint();

        /**
         * @brief Constructs a TimeConstraint
         * @param[in] : The maximum time by which all vertices in recommended solution(s) should be reached
         */
        TimeConstraint(long = 0);

        /**
         * @brief Function operator to construct a Traversal given an existing Traversal and an edge to be traversed
         * @param[in] : Graph being traversed
         * @param[in,out] : Reference to new Traversal
         * @param[in,out] : Reference to existing Traversal
         * @param[in] : Edge being Traversed
         * @return True if edge can be traversed without violating constraints else False
         */
        inline bool operator () (const Graph&, Traversal&, const Traversal&, Edge) const;
};

/**
 * @brief Structure to identify a dominating Traversal
 */
class TraversalDominance {
    public:
        /**
         * @brief Function operator to identify dominating Traversal
         * @param[in] : Reference to primary Traversal
         * @param[in] : Reference to seconday Traversal
         * @return True if primary dominates secondary else False
         */
        inline bool operator () (const Traversal&, const Traversal&) const;
};

/**
 * @brief Extends BaseGraph to implement a multi criteria path optimization
 */
class Pareto : public BaseGraph {
    public:
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
