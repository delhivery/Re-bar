#ifndef SOLVER_GRAPH_HPP_INCLUDED
#define SOLVER_GRAPH_HPP_INCLUDED

#include <string>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/property_map/property_map.hpp>

#include <constants.hpp>

#ifdef BOOST_NO_EXCEPTIONS
    void boost::throw_exception(std::exception const& exc) {
            std::cout << exc.what() << std::endl;
    }
#endif


// Cost is shipping_cost, time_cost
typedef std::pair<double, double> Cost;

struct DeliveryCenter {
    std::size_t index;
    std::string code;
    std::string name;

    DeliveryCenter();
    DeliveryCenter(int idx, std::string name, std::string code);
};

struct Connection {
    std::size_t index;

    double _departure, _duration, _t_inb_proc, _t_agg_proc, _t_out_proc = 0;

    double departure, duration;
    double cost;
    std::string name;
    
    Connection();
    Connection(std::size_t index, const double departure, const double duration, const double cost, std::string name);

    Connection(
        std::size_t index,
        const double _departure, const double _duration, const double _t_inb_proc, const double _t_agg_proc, const double _t_out_proc,
        const double cost,
        std::string name);

    Cost weight(Cost distance, double t_max);

    double wait_time(double t_parent) const;
};

struct Path {
    std::string destination;
    Connection connection;
    double arrival;
    double cost;

    Path(std::string destination, Connection& connection, double arrival, double cost);
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, DeliveryCenter, Connection> Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
typedef boost::graph_traits<Graph>::edge_descriptor Edge;
typedef std::vector<Cost> DistanceMap;
typedef std::vector<std::pair<Vertex, Connection> > PredecessorMap;

class EPGraph {
    protected:
        Graph g;
        std::map<std::string, Vertex> vertex_map;

    public:
        EPGraph() {}

        virtual ~EPGraph() {}

        void add_vertex(std::string code, std::string name);

        void add_vertex(std::string code);

        void add_edge(std::string src, std::string dest, double dep, double dur, double cost, std::string name);

        void add_edge(std::string src, std::string dest, double dep, double dur, double cost);

        void add_edge(std::string src, std::string dest, double dep, double dur, double tip, double tap, double top, double cost, std::string name);

        std::tuple<std::shared_ptr<Connection>, std::shared_ptr<DeliveryCenter>, std::shared_ptr<DeliveryCenter> > lookup(std::string name);

        virtual std::vector<Path> find_path(std::string src, std::string dst, double t_start, double t_max) = 0;
};
#endif
