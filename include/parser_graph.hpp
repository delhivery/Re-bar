#ifndef PARSER_GRAPH_HPP_INCLUDED
#define PARSER_GRAPH_HPP_INCLUDED

#include <string>
#include <limits>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/property_map/property_map.hpp>

#ifdef BOOST_NO_EXCEPTIONS
    void boost::throw_exception(std::exception const& exc) {
            std::cout << exc.what() << std::endl;
    }
#endif

const double P_INF = std::numeric_limits<double>::infinity();
const double M_INF = -1 * P_INF;

// State of the nodes
enum class State {
    REACHED,                                // Node has been visited
    ACTIVE,                                 // Shipment is currently at this node or outbound from it
    FUTURE,                                 // Shipment is expected at this node in the future
    FAIL,                                   // Node inbound or outbound failed
    INACTIVE                                // Node has been deactivated since a parent node has failed
};

// Reasons behind the state of the nodes
enum class Comment {
    SUCCESS_SEGMENT_TRAVERSED,              // Segment has been traversed successfully
    FAILURE_CENTER_OVERRIDE_CONNECTION,     // Center connected via a different connection
    WARNING_CENTER_DELAYED_CONNECTION,      // Center delayed connection outbound
    FAILURE_CONNECTION_LATE_ARRIVAL,        // Connection arrived to late to outbound
    WARNING_CONNECTION_LATE_ARRIVAL,        // Connection arrived late but allows outbound
    INFO_REGEN_TAT_CHANGE,                  // Regenerated due to change in TAT
    INFO_REGEN_DC_CHANGE,                   // Regenerated due to change in Destination
    INFO_SEGMENT_PREDICTED
};

// The outbound connection for the segment
struct Connection {
    // Unique identifier for the connection
    std::string name;
    // Predicted arrival date time
    // Predicted departure date time
    // Actual arrival date time
    // Actual departure date time
    // Time taken to process inbound
    // Time taken to aggregate/bag shipments
    // Time taken to process outbound
    double p_arr, p_dep, a_arr, a_dep, cost, t_inb_proc, t_agg_proc, t_out_proc;

    Connection(
        std::string name, double p_arr, double p_dep, double a_arr, double a_dep,
        double cost, double t_inb_proc, double t_agg_proc, double t_out_proc);
};

// A path for a shipments consists of multiple segments
struct Segment {
    std::size_t index;
    std::string ucode;

    Connection edge;
    State state;
    Comment comment;

    Segment(std::size_t index, std::string ucode, Connection edge, State state=State::FUTURE, Comment comment=Comment::INFO_SEGMENT_PREDICTED);
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, Segment> Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor Node;

class ParserGraph {
    protected:
        Graph g;

    public:
        void add_segment(
            std::string cname, std::string ucode,
            double p_arr=M_INF, double p_dep=P_INF, double a_arr=M_INF, double a_dep=P_INF,
            double cost=0, double t_inb_proc=0, double t_agg_proc=0, double t_out_proc=0);

        void add_edge(Node origin, Node destination);

        Node get_active();
};
#endif
