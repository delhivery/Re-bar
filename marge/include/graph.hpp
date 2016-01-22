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
 * A pair representing the cost incurred on traversal of an edge both in terms of physical costs and time spent
 */
typedef pair<double, long> Cost;

/**
 * Helper function which allows comparison of two Costs
 */
bool operator < (const Cost&, const Cost&);

/**
 * Structure representing a bundled properties of a vertex in a graph/tree.
 */
struct VertexProperty {
    size_t index;
    string code;

    /**
     * Default constructs an empty vertex property.
     */
    VertexProperty() {}

    /**
     * Constructs a vertex property
     * @param size_t: index of ther vertex in graph
     * @param string_view: unique human readable name for vertex
     */
    VertexProperty(size_t, string_view);
};

/**
 * Structure representing a bundled properties of an edge in a graph/tree.
 */
struct EdgeProperty {
    bool percon = false;
    size_t index;
    string code;

    long _dep, _dur, _tip, _tap, _top, dep, dur;

    double cost;

    /**
     * Default constructs an empty edge property.
     */
    EdgeProperty() {}

    /**
     * Constructs an edge property for a continuous edge(such as custody scan).
     * Continuous edges are free i.e. there is no physical cost associated against movement via them.
     * @param[in] size_t:       Index of the edge in the underlying graph
     * @param[in] const long:   Processing time in seconds for outbound at source vertex
     * @param[in] const long:   Processing time in seconds for aggregation at source vertex
     * @param[in] const long:   Processing time in seconds for inbound at destination vertex.
     * @param[in] const double: Cost of iterating the edge.
     * @param[in] string_view:  Unique human readable name for edge
     */
    EdgeProperty(
        const size_t, const long, const long, const long, const double, string_view);

    /**
     * Constructs an edge property for a time-discrete edge, taking the index of edge in the graph, unique human readeable code, the time and duration of departure in seconds, the processing time in seconds for inbound/aggregation and outbound respectively and the cost of traversal via this edge
     * @param[in] size_t:       Index of the edge in the underlying graph
     * @param[in] const long:   Time of departure from source vertex
     * @param[in] const long:   Duration of iterating the edge
     * @param[in] const long:   Processing time in seconds for outbound at source vertex
     * @param[in] const long:   Processing time in seconds for aggregation at source vertex
     * @param[in] const long:   Processing time in seconds for inbound at destination vertex.
     * @param[in] const double: Cost of iterating the edge
     * @param[in] string_view:  Unique human readable name for edge
     */
    EdgeProperty(const size_t, const long, const long, const long, const long, const long, const double, string_view);

    /**
     * Returns the wait time to traverse this edge.
     * @param[in] t_start:      Time of arrival at edge source.
     */
    long wait_time(const long) const;

    /**
     * Returns the Cost to traverse this edge.
     * @param[in] start:        Cost of arrival at edge source
     * @param[in] t_max:        Maximum time permissible to reach destination
     */
    Cost weight(const Cost&, const long);
};

/**
 * Structure representing a segment in the traversal of the graph/tree.
 */
struct Path {
    string_view src, conn, dst;
    long arr, dep;
    double cost;

    /**
     * Default constructs an empty traversal of the graph
     */
    Path() {}

    /**
     * Constructs a segment representing movement of an object.
     * @param[in] string_view:  Source Vertex
     * @param[in] string_view:  Edge used to reach destination from source
     * @param[in] string_view:  Destination Vertex
     * @param[in] long:         Arrival time at destination vertex
     * @param[in] long:         Departure time from source vertex
     * @param[in] double:       Cost incurred to arrive at source vertex
     */
    Path(string_view, string_view, string_view, long, long, double);
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, VertexProperty, EdgeProperty> Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
typedef boost::graph_traits<Graph>::edge_descriptor Edge;

/**
 * Interface representing the graph which can be traversed in different ways to satisfy various constraints
 */
class BaseGraph {
    protected:
        Graph g;
        map<string, Vertex, less<>> vertex_map;
        map<string, Edge, less<>> edge_map;
        mutable shared_timed_mutex graph_mutex;

    public:
        /**
         * Default constructs an empty Graph
         */
        BaseGraph() {}

        /**
         * Destroys the graph
         */
        virtual ~BaseGraph() {}

        /**
         * Utility function to verify if the specified key exists in the kwargs
         * @param[in] const map<string, any>&:  A map of named arguments
         * @param[in] string_view:              Key to look for the the map
         */
        static void check_kwargs(const map<string, any>&, string_view);

        /**
         * Utility function to verify if a list of specified keys exist in the kwargs
         * @param[in] const map<string, any>&:  A map of named arguments
         * @param[in] const list<string_view>&: List of keys to look for in the map
         */
        static void check_kwargs(const map<string, any>&, const list<string_view>&);

        /**
         * Adds a vertex to the graph
         * @param[in] string_view:              Unique human readable name for the vertex
         */
        void add_vertex(string_view);

        /**
         * Adds a continuous edge to the graph
         * @param[in] string_view:              Source vertex of edge
         * @param[in] string_view:              Destination vertex of edge
         * @param[in] stirng_view:              Unique human readable name for edge
         * @param[in] const long:               Processing time in seconds for inbound at destination vertex.
         * @param[in] const long:               Processing time in seconds for aggregation at source vertex
         * @param[in] const long:               Processing time in seconds for outbound at source vertex
         * @param[in] const double:             Cost of iterating the edge
         */
        virtual void add_edge(string_view, string_view, string_view, const long, const long, const long, const double);

        /**
         * Adds a discrete edge to the graph
         * @param[in] string_view:              Source vertex of edge
         * @param[in] string_view:              Destination vertex of edge
         * @param[in] stirng_view:              Unique human readable name for edge
         * @param[in] const long:               Time of departure from source vertex
         * @param[in] const long:               Duration of iterating the edge
         * @param[in] const long:               Processing time in seconds for inbound at destination vertex.
         * @param[in] const long:               Processing time in seconds for aggregation at source vertex
         * @param[in] const long:               Processing time in seconds for outbound at source vertex
         * @param[in] const double:             Cost of iterating the edge
         */
        virtual void add_edge(string_view, string_view, string_view, const long, const long, const long, const long, const long, const double);

        /**
         * Finds and returns the properties of an edge
         * @param[in] string_view:              Source vertex
         * @param[in] string_view:              Edge name
         */
        EdgeProperty lookup(string_view, string_view);

        virtual vector<Path> find_path(string_view, string_view, long, long) = 0;

        /**
         * Helper function which takes in an instance of the Graph and kwargs and invokes add vertex on the instance.
         * Returns a json structure specifying if the action was successful or raises an error message.
         */
        static json_map addv(shared_ptr<BaseGraph> solver, const map<string, any>& kwargs) {
            json_map response;
            try {
                string_view value;
                check_kwargs(kwargs, "code");
                value = any_cast<string>(kwargs.at("code"));
                solver->add_vertex(value);
                response["success"] = true;
            }
            catch (const exception& exc) {
                response["error"] = exc.what();
            }

            return response;
        }


        /**
         * Helper function which takes in an instance of the Graph and kwargs and invokes add edge on the instance.
         * Returns a json structure specifying if the action was successful or raises an error message.
         */
        static json_map adde(shared_ptr<BaseGraph> solver, const map<string, any>& kwargs) {
            json_map response;
            try {
                string_view src, dst, conn;
                long dep, dur, tip, top, tap;
                double cost;

                check_kwargs(kwargs, list<string_view>{"src", "dst", "conn", "dep", "dur", "tip", "tap", "top", "cost"});
                src  = any_cast<string>(kwargs.at("src"));
                dst  = any_cast<string>(kwargs.at("dst"));
                conn = any_cast<string>(kwargs.at("conn"));

                dep  = any_cast<long>(kwargs.at("dep"));
                dur  = any_cast<long>(kwargs.at("dur"));
                tip  = any_cast<long>(kwargs.at("tip"));
                tap  = any_cast<long>(kwargs.at("tap"));
                top  = any_cast<long>(kwargs.at("top"));

                cost = any_cast<double>(kwargs.at("cost"));

                solver->add_edge(src, dst, conn, dep, dur, tip, tap, top, cost);
                response["success"] = true;
            }
            catch (const exception& exc) {
                response["error"] = exc.what();
            }
            return response;
        }

        static json_map addc(shared_ptr<BaseGraph> solver, const map<string, any>& kwargs) {
            json_map response;
            try {
                string_view src, dst, conn;
                long tip, top, tap;
                double cost = 0.30;
                
                check_kwargs(kwargs, list<string_view>{"src", "dst", "conn", "tip", "tap", "top"});
                src = any_cast<string>(kwargs.at("src"));
                dst = any_cast<string>(kwargs.at("dst"));
                conn = any_cast<string>(kwargs.at("conn"));

                top = any_cast<long>(kwargs.at("tap"));
                tap = any_cast<long>(kwargs.at("top"));
                tip = any_cast<long>(kwargs.at("tip"));
                solver->add_edge(src, dst, conn, tip, top, tap, cost);
                response["success"] = true;
            }
            catch (const exception& exc) {
                response["error"] = exc.what();
            }
            return response;
        }

        /**
         * Helper function which takes in an instance of the Graph and kwargs and invokes lookup on the instance.
         * Returns a json structure specifying if the action was successful with recovered edge properties or raises an error message.
         */
        static json_map look(shared_ptr<BaseGraph> solver, const map<string, any>& kwargs) {
            json_map response;

            try {
                check_kwargs(kwargs, list<string_view>{"src", "conn"});
                string_view vertex = any_cast<string>(kwargs.at("src"));
                string_view edge = any_cast<string>(kwargs.at("conn"));
                auto edge_property = solver->lookup(vertex, edge);

                json_map conn;
                conn["code"] = edge_property.code;
                conn["dep"]  = edge_property._dep;
                conn["dur"]  = edge_property._dur;
                conn["tip"]  = edge_property._tip;
                conn["tap"]  = edge_property._tap;
                conn["top"]  = edge_property._top;
                conn["cost"] = edge_property.cost;

                response["connection"] = conn;
                response["success"]    = true;
            }
            catch (const exception& exc) {
                response["error"] = exc.what();
            }
            return response;
        }

        /**
         * Helper function which takes in an instance of the Graph and kwargs and invokes traversal on the instance.
         * Returns a json structure specifying if the action was successful with the traversal recommendation or raises an error message.
         */
        static json_map find(shared_ptr<BaseGraph> solver, const map<string, any>& kwargs) {
            json_map response;
            try {
                check_kwargs(kwargs, list<string_view>{"src", "dst", "beg", "tmax"});
                string_view src = any_cast<string>(kwargs.at("src"));
                string_view dst = any_cast<string>(kwargs.at("dst"));
                long t_start    = any_cast<long>(kwargs.at("beg"));
                long t_max      = any_cast<long>(kwargs.at("tmax"));

                auto path = solver->find_path(src, dst, t_start, t_max);
                json_array segments;

                for (auto const& segment: path) {
                    json_map seg;
                    seg["source"] = segment.src.to_string();
                    seg["connection"] = segment.conn.to_string();
                    seg["destination"] = segment.dst.to_string();
                    seg["arrival_at_source"] = segment.arr;
                    seg["departure_from_source"] = segment.dep;
                    seg["cost_reaching_source"] = segment.cost;
                    segments.push_back(seg);
                }
                response["path"] = segments;
                response["success"] = true;
            }
            catch (const exception& exc) {
                response["error"] = exc.what();
            }
            return response;
        }
};
#endif
