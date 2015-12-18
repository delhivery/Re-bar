#ifndef PARSER_GRAPH_HPP_INCLUDED
#define PARSER_GRAPH_HPP_INCLUDED

#include <string>
#include <limits>
#include <memory>

#include <boost/date_time.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>

#include <enum.h>
#include <marge.hpp>
#include <utils.cxx>

const double P_INF = std::numeric_limits<double>::infinity();
const double N_INF = -1 * P_INF;

// State of the nodes
BETTER_ENUM(
    State, char,
    // Node has been visited
    REACHED = 0,
    // Shipment is currently at this node or outbound from it
    ACTIVE,
    // Shipment is expected at this node in the future
    FUTURE,
    // Node inbound or outbound failed
    FAIL,
    // Node has been deactivated since a parent node has failed
    INACTIVE
);

BETTER_ENUM(
    Comment, char,
    // Segment has been traversed successfully
    SUCCESS_SEGMENT_TRAVERSED=0,
    // Center connected via a different connection
    FAILURE_CENTER_OVERRIDE_CONNECTION,
    // Center delayed connection outbound
    WARNING_CENTER_DELAYED_CONNECTION,
    // Connection arrived to late to outbound
    FAILURE_CONNECTION_LATE_ARRIVAL,
    // Connection arrived late but allows outbound
    WARNING_CONNECTION_LATE_ARRIVAL,
    // Regenerated due to change in TAT
    INFO_REGEN_TAT_CHANGE,
    // Regenerated due to change in Destination
    INFO_REGEN_DC_CHANGE,
    // Segment has been predicted
    INFO_SEGMENT_PREDICTED
);

struct Segment {
    std::string index, code, cname;
    double p_arr, p_dep, a_arr, a_dep, t_inb_proc, t_agg_proc, t_out_proc;
    double cost;

    State state = State::ACTIVE;
    Comment comment = Comment::INFO_SEGMENT_PREDICTED;

    std::shared_ptr<Segment> parent;

    Segment(
        std::string index, std::string code, std::string cname,
        double p_arr, double p_dep, double a_arr, double a_dep,
        double t_inb_proc, double t_agg_proc, double t_out_proc,
        double cost,
        State state, Comment comment, std::shared_ptr<Segment> parent=NULL
    );

    Segment(
        std::string index, std::string code, std::string cname,
        double p_arr, double p_dep, double a_arr, double a_dep,
        double t_inb_proc, double t_agg_proc, double t_out_proc,
        double cost,
        std::string state, std::string comment, std::shared_ptr<Segment> parent=NULL
    );


    bool operator < (Segment& segment) {
        return index < segment.index;
    }

    bool match(std::string cname, double a_dep);
};

struct SegmentId{};
struct SegmentCode{};
struct SegmentState{};

typedef boost::multi_index::ordered_unique<
    boost::multi_index::tag<SegmentId>,
    boost::multi_index::member<Segment, std::string, &Segment::index>
> SegmentIdIndex;

typedef boost::multi_index::ordered_non_unique<
    boost::multi_index::tag<SegmentCode>,
    boost::multi_index::member<Segment, std::string, &Segment::code>
> SegmentCodeIndex;

typedef boost::multi_index::ordered_non_unique<
    boost::multi_index::tag<SegmentState>,
    boost::multi_index::member<Segment, State, &Segment::state>
> SegmentStateIndex;

typedef boost::multi_index_container<Segment, boost::multi_index::indexed_by<SegmentIdIndex, SegmentCodeIndex, SegmentStateIndex> > SegmentContainer;

typedef SegmentContainer::index<SegmentId>::type SegmentByIndex;
typedef SegmentContainer::index<SegmentCode>::type SegmentByCode;
typedef SegmentContainer::index<SegmentState>::type SegmentByState;

class ParserGraph {
    private:
        std::string waybill;
        std::shared_ptr<Solver> solver;
        SegmentContainer path;
        SegmentByIndex& path_by_index = path.get<SegmentId>();
        SegmentByCode& path_by_code = path.get<SegmentCode>();
        SegmentByState& path_by_state = path.get<SegmentState>();

    public:
        ParserGraph(std::string waybill, std::shared_ptr<Solver> solver);

        void load_path();

        void make_root();

        void make_path(std::string origin, std::string destination, double start_dt, double promise_dt, std::shared_ptr<Segment> parent);

        void read_scan(std::string location, std::string destination, std::string action, std::string scan_dt, std::string promise_dt);

        void parse_scan(std::string location, std::string destination, std::string action, double scan_dt, double promise_dt);
};
#endif
