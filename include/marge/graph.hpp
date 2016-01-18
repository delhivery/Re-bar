#ifndef GRAPH_HPP_INCLUDED
#define GRAPH_HPP_INCLUDED

#include <limits>
#include <shared_mutex>
#include <map>
#include <experimental/string_view>
#include <experimental/any>

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

        map<string_view, string_view> add_vertex(map<string, any> kwargs);
        map<string_view, string_view> add_edge(map<string, any> kwargs);
        map<string_view, string_view> lookup(map<string, any> kwargs);
        map<string_view, string_view> find_path(map<string, any> kwargs);
};
#endif
