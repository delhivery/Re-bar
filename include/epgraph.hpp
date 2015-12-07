#ifndef EPGRAPH_DEFINED
#define EPGRAPH_DEFINED

#include <string>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/property_map/property_map.hpp>

#ifdef BOOST_NO_EXCEPTIONS
    void boost::throw_exception(std::exception const& exc) {
            std::cout << exc.what() << std::endl;
    }
#endif


namespace expath {
    const int HOURS_IN_DAY = 24 * 3600;

    struct DeliveryCenter;
    struct Connection;

    typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, DeliveryCenter, Connection> Graph;
    typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
    typedef boost::graph_traits<Graph>::edge_descriptor Edge;
    typedef std::pair<double, double> Cost;

    typedef std::vector<Cost> DistanceMap;
    typedef std::vector<std::pair<Vertex, Connection> > PredecessorMap;

    const Cost UNRECHEABLE = Cost(std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());

    struct DeliveryCenter {
        std::size_t index;
        std::string code;
        std::string name;

        DeliveryCenter();
        DeliveryCenter(int idx, std::string name, std::string code);
    };

    struct Connection {
        std::size_t index;

        double departure, duration, cost;
        std::string name;
        
        Connection();
        Connection(std::size_t index, const double departure, const double duration, const double cost, std::string name);

        Cost weight(Cost distance, double t_max);

        double wait_time(double t_parent) const;
    };

    class EPGraph {
        protected:
            Graph g;
            std::map<std::string, Vertex> vertex_map;

        public:
            void add_vertex(std::string code, std::string name);

            void add_vertex(std::string code);

            void add_edge(std::string src, std::string dest, double dep, double dur, double cost, std::string name);

            void add_edge(std::string src, std::string dest, double dep, double dur, double cost);
    
            std::vector<std::string> time_dependent_shortest_path(
                std::string src, std::string dest, double t_start, double t_max);

            template <typename Compare> void time_dependent_shortest_path(
                Vertex source, Vertex destination, DistanceMap& d, PredecessorMap& p, Compare& cmp, Cost inf, Cost zero, double t_max);
    };
}

#endif
