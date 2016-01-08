#ifndef PARSER_GRAPH_HPP_INCLUDED
#define PARSER_GRAPH_HPP_INCLUDED

#include <string>
#include <limits>
#include <memory>

#include <experimental/any>

#include <boost/date_time.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include <enum.h>
#include <marge.hpp>
#include <utils.hpp>
#include <constants.hpp>

#include <mongo/reader.hpp>
#include <mongo/writer.hpp>

BETTER_ENUM(
    Actions, char,
    LOCATION = 0,
    INSCAN,
    OUTSCAN
);

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
    static const std::vector<std::string> _keywords;

    template <typename T> static Segment from_map(const std::map<std::string, std::experimental::any>& data, const T& container) {
        Segment* parent_ptr = nullptr;
        std::string parent = std::experimental::any_cast<std::string>(data.at("par"));
        std::map<std::string, std::experimental::any> meta_data;

        if (parent != "") {
            if (container.count(parent) != 1)
                throw std::invalid_argument("Unable to find parent with _id: " + parent);
            parent_ptr = (Segment*)(&(*container.find(parent)));
        }

        Segment seg{
            std::experimental::any_cast<std::string>(data.at("_id")),
            std::experimental::any_cast<std::string>(data.at("cn")),
            std::experimental::any_cast<std::string>(data.at("ed")),
            std::experimental::any_cast<std::string>(data.at("sol")),
            double(std::experimental::any_cast<std::int64_t>(data.at("pa"))),
            double(std::experimental::any_cast<std::int64_t>(data.at("pd"))),
            double(std::experimental::any_cast<std::int64_t>(data.at("aa"))),
            double(std::experimental::any_cast<std::int64_t>(data.at("ad"))),
            std::experimental::any_cast<double>(data.at("tip")),
            std::experimental::any_cast<double>(data.at("tap")),
            std::experimental::any_cast<double>(data.at("top")),
            std::experimental::any_cast<double>(data.at("cst")),
            State::_from_string(std::experimental::any_cast<std::string>(data.at("st")).c_str()),
            Comment::_from_string(std::experimental::any_cast<std::string>(data.at("rmk")).c_str()),
            parent_ptr
        };

        for (auto const& elem: data) {
            if(std::find(_keywords.begin(), _keywords.end(), elem.first) == _keywords.end()) {
                meta_data[elem.first] = elem.second;
            }
        }

        seg.set_meta(meta_data);
        return seg;
    }

    std::string index, code, cname, soltype;
    double p_arr, p_dep, a_arr, a_dep, t_inb_proc, t_agg_proc, t_out_proc;
    double cost;

    State state = State::ACTIVE;
    Comment comment = Comment::INFO_SEGMENT_PREDICTED;

    const Segment* parent;

    std::map<std::string, std::experimental::any> meta_data;


    Segment(
        std::string index, std::string code, std::string cname, std::string soltype,
        double p_arr, double p_dep, double a_arr, double a_dep,
        double t_inb_proc, double t_agg_proc, double t_out_proc,
        double cost,
        State state, Comment comment, const Segment* parent=nullptr
    );

    Segment(
        std::string index, std::string code, std::string cname, std::string soltype,
        double p_arr, double p_dep, double a_arr, double a_dep,
        double t_inb_proc, double t_agg_proc, double t_out_proc,
        double cost,
        std::string state, std::string comment, const Segment* parent=nullptr
    );

    void set_meta(std::map<std::string, std::experimental::any> _meta_data);

    bool operator < (Segment& segment) {
        return index < segment.index;
    }

    bool match(std::string cname, double a_dep);

    std::string pindex() const;

    std::map<std::string, std::experimental::any> to_store() const;
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
        boost::multi_index::const_mem_fun<Segment, std::string, &Segment::pindex>
    >
> SegmentStateAndParentIndex;

typedef boost::multi_index::random_access<boost::multi_index::tag<struct aux> > AuxIndex;

typedef boost::multi_index_container<Segment, boost::multi_index::indexed_by<SegmentIdIndex, SegmentCodeIndex, SegmentStateIndex, SegmentStateAndParentIndex, AuxIndex> > SegmentContainer;
typedef SegmentContainer::index<SegmentId>::type SegmentByIndex;
typedef SegmentContainer::index<SegmentCode>::type SegmentByCode;
typedef SegmentContainer::index<SegmentState>::type SegmentByState;
typedef SegmentContainer::index<SegmentStateAndParent>::type SegmentByStateAndParent;

class ParserGraph {
    private:
        bool save_state = true;
        std::string waybill;
        double promise_dt;

        std::shared_ptr<Solver> solver;
        std::shared_ptr<Solver> fallback;

        SegmentContainer segment;
        SegmentByIndex& segment_by_index = segment.get<SegmentId>();
        SegmentByCode& segment_by_code = segment.get<SegmentCode>();
        SegmentByState& segment_by_state = segment.get<SegmentState>();
        SegmentByStateAndParent& segment_by_state_and_parent = segment.get<SegmentStateAndParent>();

    public:
        ParserGraph(std::string waybill, double promise_dt, std::shared_ptr<Solver> solver, std::shared_ptr<Solver> fallback);
        ~ParserGraph();

        void load_segment();

        void make_root();

        bool make_path(std::string origin, std::string destination, double start_dt, const Segment* parent);

        std::string make_duplicate_active(Segment* seg, std::shared_ptr<Connection> conn, const Segment* parent, double scan_dt);

        void parse_scan(std::string location, std::string destination, std::string connection, Actions action, double scan_dt);

        void save(bool _save_state=true);

        void show();

        template <typename T, typename F> Segment* find(T& iterable, F filters) {
            auto iter = iterable.find(filters);

            if (iter != iterable.end()) {
                return (Segment*)(&(*iter));
            }
            return nullptr;
        }

        template <typename T, typename F> Segment* find_and_modify(T& iterable, F filters, State s, double a_arr=-1, double a_dep=-1, Comment rmk=Comment::INFO_SEGMENT_PREDICTED) {
            auto iter = iterable.find(filters);

            if (iter == iterable.end())
                return nullptr;
            std::pair<typename T::iterator, typename T::iterator> iter_main = iterable.equal_range(filters);

            typedef std::vector<typename T::iterator> buffer_type;
            buffer_type vec;

            while (iter_main.first != iter_main.second) {
                vec.push_back(iter_main.first++);
            }

            for (typename buffer_type::iterator it = vec.begin(), it_end = vec.end(); it != it_end; it++) {
                iterable.modify(
                    *it,
                    [&s, &a_arr, &a_dep, &rmk](Segment& seg) {
                        seg.state = s;
                        if (a_arr != -1)
                            seg.a_arr = a_arr;

                        if (a_dep != -1)
                            seg.a_dep = a_dep;

                        if (rmk != +Comment::INFO_SEGMENT_PREDICTED) {
                            seg.comment = rmk;
                        }
                    }
                );
            }
            return (Segment *)(&(*iter));
        }
};
#endif
