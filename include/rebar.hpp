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
#include <boost/multi_index/composite_key.hpp>

#include <enum.h>
#include <marge.hpp>
#include <utils.hpp>
#include <constants.hpp>

enum Actions{
    LOCATION,
    INSCAN,
    OUTSCAN
};

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
    INFO_SEGMENT_PREDICTED,
    // Segment invalid due to missing scan
    INFO_SEGMENT_BAD_DATA
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
struct SegmentStateAndParent{};

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

typedef boost::multi_index::ordered_non_unique<
    boost::multi_index::tag<SegmentStateAndParent>,
    boost::multi_index::composite_key<
        Segment,
        boost::multi_index::member<Segment, State, &Segment::state>,
        boost::multi_index::member<Segment, std::shared_ptr<Segment>, &Segment::parent>
    >
> SegmentStateAndParentIndex;

typedef boost::multi_index_container<Segment, boost::multi_index::indexed_by<SegmentIdIndex, SegmentCodeIndex, SegmentStateIndex, SegmentStateAndParentIndex> > SegmentContainer;
typedef SegmentContainer::index<SegmentId>::type SegmentByIndex;
typedef SegmentContainer::index<SegmentCode>::type SegmentByCode;
typedef SegmentContainer::index<SegmentState>::type SegmentByState;
typedef SegmentContainer::index<SegmentStateAndParent>::type SegmentByStateAndParent;

class ParserGraph {
    private:
        std::string waybill;
        std::shared_ptr<Solver> solver;
        SegmentContainer segment;
        SegmentByIndex& segment_by_index = segment.get<SegmentId>();
        SegmentByCode& segment_by_code = segment.get<SegmentCode>();
        SegmentByState& segment_by_state = segment.get<SegmentState>();
        SegmentByStateAndParent& segment_by_state_and_parent = segment.get<SegmentStateAndParent>();

    public:
        ParserGraph(std::string waybill, std::shared_ptr<Solver> solver);

        void load_segment();

        void make_root();

        bool make_path(std::string origin, std::string destination, double start_dt, double promise_dt, std::shared_ptr<Segment> parent);

        std::shared_ptr<Segment> make_duplicate_active(Segment& seg, std::shared_ptr<Connection> conn, std::shared_ptr<Segment> parent, double scan_dt);

        void read_scan(std::string location, std::string destination, std::string connection, std::string action, std::string scan_dt, std::string promise_dt);

        void parse_scan(std::string location, std::string destination, std::string connection, Actions action, double scan_dt, double promise_dt);
};
#endif
