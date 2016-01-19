#ifndef GRAPH_HPP_INCLUDED
#define GRAPH_HPP_INCLUDED

#include <limits>
#include <shared_mutex>
#include <map>
#include <experimental/string_view>
#include <experimental/any>

#include <jeayeson/jeayeson.hpp>

#include <boost/graph/adjacency_list.hpp>

#ifdef BOOST_NO_EXCEPTIONS
    void boost::throw_exception(std::exception const& exc) {
        std::cout << exc.what() << std::endl;
    }
#endif

using namespace std;
using namespace std::experimental;

const double P_D_INF = std::numeric_limits<double>::infinity();
const double N_D_INF = -1 * P_D_INF;

const long P_L_INF = 0;
const long N_L_INF = -1 * P_L_INF;

const long TIME_DURINAL = 24 * 3600;

typedef std::pair<double, long> Cost;

bool operator < (const Cost& first, const Cost& second);

struct VertexProperty {
    size_t index;
    string code;

    VertexProperty() {}
    VertexProperty(size_t _index, string_view _code);
};

struct EdgeProperty {
    bool percon = false;
    size_t index;
    string code;

    long _dep, _dur, _tip, _tap, _top, dep, dur;

    double cost;

    EdgeProperty() {}
    EdgeProperty(const size_t _index, string_view code, const long __tip, const long __tap, const long __top);
    EdgeProperty(const size_t _index, const long __dep, const long __dur, const long __tip, const long __tap, const long __top, const double _cost, string_view _code);

    long wait_time(const long t_start) const;

    Cost weight(const Cost& start, const long t_max);
};

struct Path {
    string_view src, conn, dst;
    long arr, dep;
    double cost;

    Path() {}
    Path(string_view _src, string_view _conn, string_view _dst, long _arr, long _dep, double _cost) : src(_src), conn(_conn), dst(_dst), arr(_arr), dep(_dep), cost(_cost) {}
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, VertexProperty, EdgeProperty> Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
typedef boost::graph_traits<Graph>::edge_descriptor Edge;

class BaseGraph {
    protected:
        Graph g;
        map<string_view, Vertex> vertex_map;
        map<string_view, Edge> edge_map;
        mutable shared_timed_mutex graph_mutex;

    public:
        BaseGraph() {}
        virtual ~BaseGraph() {}

        static void check_kwargs(const map<string, any>& kwargs, string_view key);

        static void check_kwargs(const map<string, any>& kwargs, const list<string_view>& keys);

        void add_vertex(string_view code);
        void add_edge(string_view src, string_view dest, string_view code, const long dep, const long dur, const long tip, const long tap, const long top, const double cost);
        EdgeProperty lookup(string_view vertex, string_view edge);

        virtual std::vector<Path> find_path(string_view src, string_view dst, const long t_start, const long t_max) = 0;

        static json_map addv(shared_ptr<BaseGraph> solver, const map<string, any>& kwargs) {
            json_map response;
            try {
                string_view value;
                check_kwargs(kwargs, "code");
                value = any_cast<string_view>(kwargs.at("code"));
                solver->add_vertex(value);
                response["success"] = true;
            }
            catch (const exception& exc) {
                response["error"] = exc.what();
            }

            return response;
        }


        static json_map adde(shared_ptr<BaseGraph> solver, const map<string, any>& kwargs) {
            json_map response;
            try {
                string src, dst, conn;
                long dep, dur, tip, top, tap;
                double cost;

                check_kwargs(kwargs, list<string_view>{"src", "dst", "conn", "dep", "dur", "tip", "tap", "top", "cost"});

                src  = any_cast<string>(kwargs.at("src"));
                dst  = any_cast<string>(kwargs.at("dst"));
                conn = any_cast<string>(kwargs.at("conn"));

                dep  = any_cast<long>(kwargs.at("dst"));
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
