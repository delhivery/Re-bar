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
bool operator < (const Cost& first, const Cost& second);

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
     * Constructs a vertex property specifying the index of vertex in the graph and a unique human readeable code for representation
     */
    VertexProperty(size_t _index, string_view _code);
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
     * @param[in] _index: index of the edge in the underlying graph
     * @param[in] code: a unique code for human readable representation
     * @param[in] __tip: Processing time in seconds for outbound at source vertex
     * @param[in] __tap: Processing time in seconds for aggregation at source vertex
     * @param[in] __top: Processing time in seconds for inbound at destination vertex.
     */
    EdgeProperty(
        const size_t _index, string_view code, const long __tip, const long __tap, const long __top);

    /**
     * Constructs an edge property for a time-discrete edge, taking the index of edge in the graph, unique human readeable code, the time and duration of departure in seconds, the processing time in seconds for inbound/aggregation and outbound respectively and the cost of traversal via this edge
     * @param[in] _index: index of the edge in the underlying graph
     * @param[in] __dep: Time of departure from source vertex
     * @param[in] __dur: Duration to traverse the edge
     * @param[in] __tip: Processing time in seconds for outbound at source vertex
     * @param[in] __tap: Processing time in seconds for aggregation at source vertex
     * @param[in] __top: Processing time in seconds for inbound at destination vertex.
     * @param[in] _cost: Cost incurred on traversing the edge
     * @param[in] code: a unique code for human readable representation
     */
    EdgeProperty(const size_t _index, const long __dep, const long __dur, const long __tip, const long __tap, const long __top, const double _cost, string_view _code);

    /**
     * Returns the wait time to traverse this edge.
     * @param[in] t_start: Time of arrival at edge source.
     */
    long wait_time(const long t_start) const;

    /**
     * Returns the Cost to traverse this edge.
     * @param[in] start: Cost of arrival at edge source
     * @param[in] t_max: Maximum time permissible to reach destination
     */
    Cost weight(const Cost& start, const long t_max);
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
     * @param[in] _src: Source Vertex
     * @param[in] _conn: Edge used to reach destination from source
     * @param[in] _dst: Destination Vertex
     * @param[in] _arr: Arrival time at destination vertex
     * @param[in] _dep: Departure time from source vertex
     * @param[in] _cost: Cost incurred to arrive at destination vertex.
     */
    Path(string_view _src, string_view _conn, string_view _dst, long _arr, long _dep, double _cost) : src(_src), conn(_conn), dst(_dst), arr(_arr), dep(_dep), cost(_cost) {}
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
         */
        static void check_kwargs(const map<string, any>& kwargs, string_view key);

        /**
         * Utility function to verify if a list of specified keys exist in the kwargs
         */
        static void check_kwargs(const map<string, any>& kwargs, const list<string_view>& keys);

        /**
         * Adds a vertex to the graph given its unique human readable code
         */
        void add_vertex(string_view code);

        /**
         * Adds an edge to the graph given its source vertex src, destination vertex dst, unique human readable code, time of departure, duration of traversal, processing time incurred for aggregation at source/outbound at source/inbound at destination and the cost incurred on traversal via the edge.
         */
        void add_edge(string_view src, string_view dest, string_view code, const long dep, const long dur, const long tip, const long tap, const long top, const double cost);

        /**
         * Finds and returns the properties of an edge given its source vertex and its unique human readeable name
         */
        EdgeProperty lookup(string_view vertex, string_view edge);

        virtual vector<Path> find_path(string_view src, string_view dst, long t_start, long t_max) = 0;

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
