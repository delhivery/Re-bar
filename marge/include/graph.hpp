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

const double P_D_INF = std::numeric_limits<double>::infinity();
const double N_D_INF = -1 * P_D_INF;

const long P_L_INF = 0;
const long N_L_INF = -1 * P_L_INF;

const long TIME_DURINAL = 24 * 3600;

typedef std::pair<double, long> Cost;

bool operator < (const Cost& first, const Cost& second);

struct VertexProperty {
    size_t index;
    std::string code;

    VertexProperty() {}
    VertexProperty(size_t _index, std::experimental::string_view _code);
};

struct EdgeProperty {
    bool percon = false;
    size_t index;
    std::string code;

    long _dep, _dur, _tip, _tap, _top, dep, dur;

    double cost;

    EdgeProperty() {}
    EdgeProperty(const size_t _index, std::experimental::string_view code, const long __tip, const long __tap, const long __top);
    EdgeProperty(const size_t _index, const long __dep, const long __dur, const long __tip, const long __tap, const long __top, const double _cost, std::experimental::string_view _code);

    long wait_time(const long t_start) const;

    Cost weight(const Cost& start, const long t_max);
};

struct Path {
    std::experimental::string_view src, conn, dst;
    long arr, dep;
    double cost;

    Path() {}
    Path(std::experimental::string_view _src, std::experimental::string_view _conn, std::experimental::string_view _dst, long _arr, long _dep, double _cost) : src(_src), conn(_conn), dst(_dst), arr(_arr), dep(_dep), cost(_cost) {}
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, VertexProperty, EdgeProperty> Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
typedef boost::graph_traits<Graph>::edge_descriptor Edge;

class BaseGraph {
    protected:
        Graph g;
        std::map<std::experimental::string_view, Vertex> vertex_map;
        std::map<std::experimental::string_view, Edge> edge_map;
        mutable std::shared_timed_mutex graph_mutex;

    public:
        BaseGraph() {}
        virtual ~BaseGraph() {}

        static void check_kwargs(const std::map<std::string, std::experimental::any>& kwargs, std::experimental::string_view key);

        static void check_kwargs(const std::map<std::string, std::experimental::any>& kwargs, const std::list<std::experimental::string_view>& keys);

        void add_vertex(std::experimental::string_view code);
        void add_edge(std::experimental::string_view src, std::experimental::string_view dest, std::experimental::string_view code, const long dep, const long dur, const long tip, const long tap, const long top, const double cost);
        EdgeProperty lookup(std::experimental::string_view vertex, std::experimental::string_view edge);

        virtual std::vector<Path> find_path(std::experimental::string_view src, std::experimental::string_view dst, const long t_start, const long t_max) = 0;

        static json_map addv(std::shared_ptr<BaseGraph> solver, const std::map<std::string, std::experimental::any>& kwargs) {
            json_map response;
            try {
                std::experimental::string_view value;
                check_kwargs(kwargs, "code");
                value = std::experimental::any_cast<std::experimental::string_view>(kwargs.at("code"));
                solver->add_vertex(value);
                response["success"] = true;
            }
            catch (const std::exception& exc) {
                response["error"] = exc.what();
            }

            return response;
        }


        static json_map adde(std::shared_ptr<BaseGraph> solver, const std::map<std::string, std::experimental::any>& kwargs) {
            json_map response;
            try {
                std::string src, dst, conn;
                long dep, dur, tip, top, tap;
                double cost;

                check_kwargs(kwargs, std::list<std::experimental::string_view>{"src", "dst", "conn", "dep", "dur", "tip", "tap", "top", "cost"});

                src  = std::experimental::any_cast<std::string>(kwargs.at("src"));
                dst  = std::experimental::any_cast<std::string>(kwargs.at("dst"));
                conn = std::experimental::any_cast<std::string>(kwargs.at("conn"));

                dep  = std::experimental::any_cast<long>(kwargs.at("dst"));
                dur  = std::experimental::any_cast<long>(kwargs.at("dur"));
                tip  = std::experimental::any_cast<long>(kwargs.at("tip"));
                tap  = std::experimental::any_cast<long>(kwargs.at("tap"));
                top  = std::experimental::any_cast<long>(kwargs.at("top"));

                cost = std::experimental::any_cast<double>(kwargs.at("cost"));

                solver->add_edge(src, dst, conn, dep, dur, tip, tap, top, cost);
                response["success"] = true;
            }
            catch (const std::exception& exc) {
                response["error"] = exc.what();
            }
            return response;
        }

        static json_map look(std::shared_ptr<BaseGraph> solver, const std::map<std::string, std::experimental::any>& kwargs) {
            json_map response;

            try {
                check_kwargs(kwargs, std::list<std::experimental::string_view>{"src", "conn"});
                std::experimental::string_view vertex = std::experimental::any_cast<std::string>(kwargs.at("src"));
                std::experimental::string_view edge = std::experimental::any_cast<std::string>(kwargs.at("conn"));
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
            catch (const std::exception& exc) {
                response["error"] = exc.what();
            }
            return response;
        }

        static json_map find(std::shared_ptr<BaseGraph> solver, const std::map<std::string, std::experimental::any>& kwargs) {
            json_map response;
            try {
                check_kwargs(kwargs, std::list<std::experimental::string_view>{"src", "dst", "beg", "tmax"});
                std::experimental::string_view src = std::experimental::any_cast<std::string>(kwargs.at("src"));
                std::experimental::string_view dst = std::experimental::any_cast<std::string>(kwargs.at("dst"));
                long t_start                       = std::experimental::any_cast<long>(kwargs.at("beg"));
                long t_max                         = std::experimental::any_cast<long>(kwargs.at("tmax"));

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
            catch (const std::exception& exc) {
                response["error"] = exc.what();
            }
            return response;
        }
};
#endif
