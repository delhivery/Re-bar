/** @file graph.hpp
 * @brief Defines base graph and helper functions
 */
#ifndef GRAPH_HPP_INCLUDED
#define GRAPH_HPP_INCLUDED

#include <limits>
#include <shared_mutex>
#include <map>
#include <experimental/string_view>
#include <experimental/any>

#include <jeayeson/jeayeson.hpp>

#include <boost/graph/adjacency_list.hpp>

using namespace std;
using std::experimental::any;
using std::experimental::any_cast;
using std::experimental::string_view;

const double P_D_INF = numeric_limits<double>::infinity();
const double N_D_INF = -1 * P_D_INF;

const long P_L_INF = LONG_MAX;
const long N_L_INF = -1 * P_L_INF;

const long TIME_DURINAL = 24 * 3600;

/**
 * @brief A pair representing the cost incurred on traversal of an edge both in terms of physical costs and time spent
 */
typedef pair<double, long> Cost;

/**
 * @brief Helper function which allows comparison of two Costs
 */
bool operator < (const Cost&, const Cost&);

/**
 * @brief Structure representing a bundled properties of a vertex in a graph/tree.
 */
struct VertexProperty {
    /**
     * @brief Index of vertex in graph
     */
    size_t index;

    /**
     * @brief Unique human readeable name for vertex
     */
    string code;

    /**
     * @brief Default constructs an empty vertex property.
     */
    VertexProperty() {}

    /**
     * @brief Constructs a vertex property
     * @param[in] : index of ther vertex in graph
     * @param[in] : unique human readable name for vertex
     */
    VertexProperty(size_t, string_view);
};

/**
 * @brief Structure representing a bundled properties of an edge in a graph/tree.
 */
struct EdgeProperty {
    /**
     * @brief Flag to indicate connection as continuous or time-discrete
     */
    bool percon = false;

    /**
     * @brief Index of edge in graph
     */
    size_t index;

    /**
     * @brief Unique human readable name of the edge
     */
    string code;

    /**
     * @brief Actual departure time at source of edge
     */
    long _dep;

    /**
     * @brief Actual duration of traversal of edge
     */
    long _dur;

    /**
     * @brief Time to process inbound at edge destination
     */
    long _tip;

    /**
     * @brief Time to aggregate items for outbound via edge at edge source
     */
    long _tap;

    /**
     * @brief Time to process outbound via edge at edge source
     */
    long _top;

    /**
     * @brief Computed departure time at source of edge
     * @details Calculated generally as actual departure minus aggregation time minus outbound time
     */
    long dep;

    /**
     * @brief Computed duration for traversal via edge
     * @details Calculated generally as actual duration plus aggregation plus outbound plus inbound time
     */
    long dur;

    /**
     * @brief Actual cost of traversing the edge
     */
    double cost;

    /**
     * @brief Default constructs an empty edge property.
     */
    EdgeProperty() {}

    /**
     * @brief Constructs an edge property for a continuous edge(such as custody scan).
     * @details Continuous edges are free i.e. there is no physical cost associated against movement via them.
     * @param[in] :       Index of the edge in the underlying graph
     * @param[in] :   Processing time in seconds for outbound at source vertex
     * @param[in] :   Processing time in seconds for aggregation at source vertex
     * @param[in] :   Processing time in seconds for inbound at destination vertex.
     * @param[in] : Cost of iterating the edge.
     * @param[in] :  Unique human readable name for edge
     */
    EdgeProperty(const size_t, const long, const long, const long, const double, string_view);

    /**
     * @brief Constructs an edge property for a time-discrete edge
     * @details A time-discrete edge is an edge with a discrete start and duration attribute
     * @param[in] : Index of the edge in the underlying graph
     * @param[in] : Time of departure from source vertex
     * @param[in] : Duration of iterating the edge
     * @param[in] : Processing time in seconds for outbound at source vertex
     * @param[in] : Processing time in seconds for aggregation at source vertex
     * @param[in] : Processing time in seconds for inbound at destination vertex.
     * @param[in] : Cost of iterating the edge
     * @param[in] : Unique human readable name for edge
     */
    EdgeProperty(const size_t, const long, const long, const long, const long, const long, const double, string_view);

    /**
     * @brief Calculate the wait time to traverse this edge.
     * @param[in] : Time of arrival at edge source.
     * @return Time spent waiting at edge source before departing via this edge
     */
    long wait_time(const long) const;

    /**
     * @brief Returns the Cost to traverse this edge.
     * @param[in] : Cost of arrival at edge source
     * @param[in] : Maximum time permissible to reach destination
     * @return Cost on traversing this edge.
     */
    Cost weight(const Cost&, const long);
};

/**
 * @brief Structure representing a segment in the traversal of the graph/tree.
 */
struct Path {
    /**
     * @brief Source vertex in segment
     */
    string_view src;
    /**
     * @brief Edge used to traverse to destination vertex in segment
     */
    string_view conn;
    /**
     * @brief Destination vertex in segment
     */
    string_view dst;
    /**
     * @brief Time of arrival at source vertex
     */
    long arr;

    /**
     * @brief Time of departure from source vertex
     */
    long dep;

    /**
     * @brief Cumulative cost of arriving at the source vertex
     */
    double cost;

    /**
     * @brief Default constructs an empty traversal of the graph
     */
    Path() {}

    /**
     * @brief Constructs a segment representing movement of an object.
     * @param[in] : Source Vertex
     * @param[in] : Edge used to reach destination from source
     * @param[in] : Destination Vertex
     * @param[in] : Arrival time at destination vertex
     * @param[in] : Departure time from source vertex
     * @param[in] : Cost incurred to arrive at source vertex
     */
    Path(string_view, string_view, string_view, long, long, double);
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, VertexProperty, EdgeProperty> Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
typedef boost::graph_traits<Graph>::edge_descriptor Edge;

/**
 * @brief Interface representing the graph which can be traversed in different ways to satisfy various constraints
 */
class BaseGraph {
    protected:
        /**
         * @brief Actual graph, stored as a bgl::adjacency_list
         */
        Graph g;

        /**
         * @brief Mapping for verbose vertex names to vertices in graph
         */
        map<string, Vertex, less<>> vertex_map;

        /**
         * @brief Mapping for verbose edge names to edges in graph
         */
        map<string, Edge, less<>> edge_map;

        /**
         * @brief Mutex to handle locks for read/write on graph
         */
        mutable shared_timed_mutex graph_mutex;

    public:
        /**
         * @brief Default constructs an empty Graph
         */
        BaseGraph() {}

        /**
         * @brief Destroys the graph
         */
        virtual ~BaseGraph() {}

        /**
         * @brief Utility function to verify if the specified key exists in the kwargs
         * @param[in] : A map of named arguments
         * @param[in] : Key to look for the the map
         */
        static void check_kwargs(const map<string, any>&, string_view);

        /**
         * @brief Utility function to verify if a list of specified keys exist in the kwargs
         * @param[in] : A map of named arguments
         * @param[in] : List of keys to look for in the map
         */
        static void check_kwargs(const map<string, any>&, const list<string_view>&);

        /**
         * @brief Adds a vertex to the graph
         * @param[in] : Unique human readable name for the vertex
         */
        void add_vertex(string_view);

        /**
         * @brief Adds a continuous edge to the graph
         * @param[in] : Source vertex of edge
         * @param[in] : Destination vertex of edge
         * @param[in] : Unique human readable name for edge
         * @param[in] : Processing time in seconds for inbound at destination vertex.
         * @param[in] : Processing time in seconds for aggregation at source vertex
         * @param[in] : Processing time in seconds for outbound at source vertex
         * @param[in] : Cost of iterating the edge
         */
        virtual void add_edge(string_view, string_view, string_view, const long, const long, const long, const double);

        /**
         * @brief Adds a discrete edge to the graph
         * @param[in] : Source vertex of edge
         * @param[in] : Destination vertex of edge
         * @param[in] : Unique human readable name for edge
         * @param[in] : Time of departure from source vertex
         * @param[in] : Duration of iterating the edge
         * @param[in] : Processing time in seconds for inbound at destination vertex.
         * @param[in] : Processing time in seconds for aggregation at source vertex
         * @param[in] : Processing time in seconds for outbound at source vertex
         * @param[in] : Cost of iterating the edge
         */
        virtual void add_edge(string_view, string_view, string_view, const long, const long, const long, const long, const long, const double);

        /**
         * @brief Finds the properties of an edge
         * @param[in] : Source vertex
         * @param[in] : Edge name
         * @return Property of matching edge
         */
        EdgeProperty lookup(string_view, string_view);

        /**
         * @brief Finds and returns a path based on various relaxation criteria
         * @param[in] : Source vertex
         * @param[in] : Destination vertex
         * @param[in] : Time of arrival at source vertex
         * @param[in] : Maximum time to arrive at destination vertex
         * @return A vector of Path representing an ideal path satisfying specified constraints
         */
        virtual vector<Path> find_path(string_view, string_view, long, long) = 0;

        /**
         * @brief Helper function to add vertex to graph.
         * @param[in] : Pointer to an instance of BaseGraph to which a vertex would be added
         * @param[in] : Named keyword arguments for adding a vertex
         * @return A json response indicating success of failure of the command and any additional output from the underlying command
         */
        static json_map addv(shared_ptr<BaseGraph>, const map<string, any>&);

        /**
         * @brief Helper function to add a time-discrete edge to BaseGraph.
         * @param[in] : Pointer to an instance of BaseGraph to which an edge would be added
         * @param[in] : Named keyword arguments for adding an edge
         * @return A json response indicating success of failure of the command and any additional output from the underlying command
         */
        static json_map adde(shared_ptr<BaseGraph>, const map<string, any>&);

        /**
         * @brief Helper function to add a continuous edge to BaseGraph.
         * @param[in] : Pointer to an instance of BaseGraph to which an edge would be added
         * @param[in] : Named keyword arguments for adding an edge
         * @return A json response indicating success of failure of the command and any additional output from the underlying command
         */
        static json_map addc(shared_ptr<BaseGraph>, const map<string, any>&);

        /**
         * @brief Helper function to find an edge in BaseGraph.
         * @param[in] : Pointer to an instance of BaseGraph against which lookup is performed
         * @param[in] : Named keyword arguments for lookup
         * @return A json response indicating success of failure of the command and any additional output from the underlying command
         */
        static json_map look(shared_ptr<BaseGraph>, const map<string, any>&);

        /**
         * @brief Helper function to find a multi-criteria shortest path in BaseGraph.
         * @param[in] : Pointer to an instance of BaseGraph against which a path is traversed
         * @param[in] : Named keyword arguments for path traversal
         * @return A json response indicating success of failure of the command and any additional output from the underlying command
         */
        static json_map find(shared_ptr<BaseGraph>, const map<string, any>&);
};
#endif
