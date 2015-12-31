#include <bsoncxx/builder/stream/document.hpp>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include <rebar.hpp>

Segment::Segment(
        std::string index, std::string code, std::string cname,
        double p_arr, double p_dep, double a_arr, double a_dep,
        double t_inb_proc, double t_agg_proc, double t_out_proc,
        double cost,
        State state, Comment comment, Segment* parent
) : 
        index(index), code(code), cname(cname),
        p_arr(p_arr), p_dep(p_dep), a_arr(a_arr), a_dep(a_dep),
        t_inb_proc(t_inb_proc), t_agg_proc(t_agg_proc), t_out_proc(t_out_proc),
        cost(cost), state(state), comment(comment), parent(parent) {
}

Segment::Segment(
        std::string index, std::string code, std::string cname,
        double p_arr, double p_dep, double a_arr, double a_dep,
        double t_inb_proc, double t_agg_proc, double t_out_proc,
        double cost,
        std::string state, std::string comment, Segment* parent
    ) {
    Segment(
        index, code, cname,
        p_arr, p_dep, a_arr, a_dep,
        t_inb_proc, t_agg_proc, t_out_proc,
        cost,
        State::_from_string(state.c_str()),
        Comment::_from_string(comment.c_str()),
        parent
    );
}

bool Segment::match(std::string _cname, double _a_dep) {
    if(cname == _cname) {
        if (std::abs(_a_dep - p_dep) < (HOURS_IN_DAY - t_agg_proc - t_out_proc)) {
            return true;
        }
    }

    return false;
}

boost::variant<int, double, std::string, bsoncxx::oid, std::nullptr_t> Segment::getattr(std::string attr) const {
    boost::variant<int, double, std::string, bsoncxx::oid, std::nullptr_t> result;

    if (attr == "_id") {
        result = bsoncxx::oid{index};
    }

    else if (attr == "cn") {
        result = code;
    }

    else if (attr == "ed") {
        result = cname;
    }

    else if (attr == "pa") {
        result = p_arr;
    }

    else if (attr == "pd") {
        result = p_dep;
    }

    else if (attr == "aa") {
        result = a_arr;
    }

    else if (attr == "ad") {
        result = a_dep;
    }

    else if (attr == "tip") {
        result = t_inb_proc;
    }

    else if (attr == "tap") {
        result = t_agg_proc;
    }

    else if (attr == "top") {
        result = t_out_proc;
    }

    else if (attr == "st") {
        result = state._to_string();
    }

    else if (attr == "rmk") {
        result = comment._to_string();
    }

    else if (attr == "cst") {
        result = cost;
    }

    else if (attr == "par") {
        if (parent == nullptr) {
            result = nullptr;
        }
        else if (parent->index != "")
            result = bsoncxx::oid{parent->index};
        else
            result = nullptr;
    }

    return result;
}

ParserGraph::ParserGraph(std::string waybill, std::shared_ptr<Solver> solver) : waybill(waybill), solver(solver) {
    load_segment();

    if (segment_by_index.size() == 0) {
        make_root();
    }
}

ParserGraph::~ParserGraph() {
    if (save_state) {
        MongoWriter mw{"rebar"};
        mw.init();
        std::map<std::string, std::string> meta_data;
        meta_data["wbn"] = waybill;
        boost::multi_index::index<SegmentContainer, SegmentId>::type& iter = segment.get<SegmentId>();
        mw.write("segments", iter, std::vector<std::string>{"_id", "cn", "ed", "pa", "pd", "aa", "ad", "tip", "tap", "top", "st", "rmk", "cst", "par"}, "_id", meta_data);
    }
}

void ParserGraph::make_root() {
    Segment seg{
        bsoncxx::oid(bsoncxx::oid::init_tag).to_string(),   // Unique Id
        "",                                                 // Code
        "",                                                 // Cname
        N_INF,                                              // p_arr
        N_INF,                                              // p_dep
        N_INF,                                              // a_arr
        N_INF,                                              // a_dep
        0,                                                  // t_inb_proc
        0,                                                  // t_agg_proc
        0,                                                  // t_out_proc
        0,                                                  // cost
        State::ACTIVE,                                      // state
        Comment::INFO_SEGMENT_PREDICTED,                    // comment
    };
    segment.insert(seg);
}

void ParserGraph::load_segment() {
    MongoReader mc{"rebar"};
    mc.init();

    mongocxx::options::find options;
    bsoncxx::builder::stream::document sorter;
    sorter << "_id" << 1;
    options.sort(sorter.view());

    bsoncxx::builder::stream::document filter;
    filter << "wbn" << waybill;

    for (auto const& result: mc.query(
            "segments",
            filter,
            std::vector<std::string>{
                "_id", "wbn", "cn", "ed",
                "pa", "aa", "pd", "ad",
                "tip", "tap", "top",
                "st", "rmk",
                "cst", "par"
            },
            options
    )) {
        std::string index, code, cname, p_arr, a_arr, p_dep, a_dep, t_inb_proc, t_agg_proc, t_out_proc, cost, parent, state, comment;
        index = result.at("_id");
        code = result.at("cn");
        cname = result.at("ed");
        p_arr = result.at("pa");
        a_arr = result.at("aa");
        p_dep = result.at("pd");
        a_dep = result.at("ad");
        t_inb_proc = result.at("tip");
        t_agg_proc = result.at("tap");
        t_out_proc = result.at("top");
        state = result.at("st");
        comment = result.at("rmk");
        cost = result.at("cst");
        parent = result.at("par");

        Segment* parent_ptr = nullptr;

        if(parent != "") {

            if (segment_by_index.count(parent) != 1)
                throw std::invalid_argument("Unable to find parent with _id: " + parent);
            parent_ptr = (Segment*)(&(*segment_by_index.find(parent)));
        }

        segment.insert(Segment{
            index, code, cname, 
            std::stod(p_arr), std::stod(p_dep), std::stod(a_arr), std::stod(a_dep),
            std::stod(t_inb_proc), std::stod(t_agg_proc), std::stod(t_out_proc),
            std::stod(cost),
            State::_from_string(state.c_str()),
            Comment::_from_string(comment.c_str()),
            parent_ptr
        });
    }
}

std::string ParserGraph::make_duplicate_active(Segment* seg, std::shared_ptr<Connection> conn, Segment* parent, double scan_dt) {
    Segment newseg{
        bsoncxx::oid(bsoncxx::oid::init_tag).to_string(),
        seg->code,
        conn->name,
        seg->p_arr,
        seg->p_dep,
        seg->a_arr,
        scan_dt,
        seg->t_inb_proc,
        conn->_t_out_proc,
        conn->_t_agg_proc,
        seg->cost,
        State::REACHED,
        Comment::SUCCESS_SEGMENT_TRAVERSED,
        parent
    };

    segment.insert(newseg);
    return newseg.index;
}

bool ParserGraph::make_path(std::string origin, std::string destination, double start_dt, double promise_dt, Segment* parent) {
    double start_t = get_time(start_dt);
    double max_t = promise_dt - start_dt;
    auto solution = solver->solve(origin, destination, start_t, max_t);

    if (solution.size() == 0)
        return false;

    auto origin_center = origin;
    auto origin_arrival = start_dt - start_t;
    auto arrival_cost = start_t;
    double base_shipping_cost = 0;

    if (parent != nullptr) {
        base_shipping_cost = parent->cost;
    }

    double shipping_cost = 0;
    double inbound = 0;

    for (auto const& path: solution) {
        auto connection = path.connection;
        Segment seg{
            bsoncxx::oid(bsoncxx::oid::init_tag).to_string(),
            origin_center,
            connection.name,
            origin_arrival + arrival_cost,
            origin_arrival + arrival_cost + wait_time(start_t, connection._departure),
            P_INF,
            P_INF,
            inbound,
            connection._t_agg_proc,
            connection._t_out_proc,
            base_shipping_cost + shipping_cost,
            State::FUTURE,
            Comment::INFO_SEGMENT_PREDICTED,
            parent
        };

        arrival_cost = path.arrival;
        shipping_cost = path.cost;
        origin_center = path.destination;
        inbound = connection._t_inb_proc;
        segment.insert(seg);
        parent = (Segment*)(&(*segment_by_index.find(seg.index)));
    }

    // Create terminal segment
    Segment seg{
        bsoncxx::oid(bsoncxx::oid::init_tag).to_string(),
        origin_center,
        "",
        origin_arrival + arrival_cost,
        P_INF,
        P_INF,
        P_INF,
        inbound,
        0,
        0,
        base_shipping_cost + shipping_cost,
        State::FUTURE,
        Comment::INFO_SEGMENT_PREDICTED,
        parent
    };
    segment.insert(seg);

    return true;
}

void ParserGraph::save(bool _save_state) {
    save_state = _save_state;
}

void ParserGraph::parse_scan(std::string location, std::string destination, std::string connection, Actions action, double scan_dt, double promise_dt) {
    Segment* active = find(segment_by_state, State::ACTIVE);

    if (active == nullptr)
        return;

    if (active->parent == nullptr) {
        if ((action == +Actions::LOCATION) or (action == +Actions::INSCAN)) {
            // generate new path against root
            make_path(location, destination, scan_dt, promise_dt, active);

            Segment* reached = find_and_modify(segment_by_state, State::ACTIVE, State::REACHED);

            if (reached != nullptr) {
                active = find_and_modify(segment_by_state_and_parent, std::make_tuple(State::FUTURE, reached), State::ACTIVE, scan_dt);
            }
        }
    }
    else if(active->code == location) {

        if (action == +Actions::OUTSCAN) {

            if (active->match(connection, scan_dt)) {
                // Connection matches
                auto comment = Comment::SUCCESS_SEGMENT_TRAVERSED;

                if (active->p_dep < scan_dt)
                    comment = Comment::WARNING_CENTER_DELAYED_CONNECTION;

                // Mark active as reached
                find_and_modify(segment_by_index, active->index, State::REACHED, scan_dt, comment);
                find_and_modify(segment_by_state_and_parent, std::make_tuple(State::FUTURE, active), State::ACTIVE, scan_dt);
            }
            else {
                find_and_modify(segment_by_state, State::FUTURE, State::INACTIVE, Comment::INFO_SEGMENT_BAD_DATA);
                find_and_modify(segment_by_index, active->index, State::FAIL, Comment::FAILURE_CENTER_OVERRIDE_CONNECTION);

                std::shared_ptr<Connection> conn;
                std::shared_ptr<DeliveryCenter> src, dst;
                std::tuple<std::shared_ptr<Connection>, std::shared_ptr<DeliveryCenter>, std::shared_ptr<DeliveryCenter> > {conn, src, dst} = solver->lookup(
                    connection);

                if (conn == nullptr) {
                    throw std::invalid_argument("Unable to find a connection with index: " + connection);
                }

                auto forked = find(segment_by_index, make_duplicate_active(active, conn, active->parent, scan_dt));

                if (forked != nullptr) {
                    auto scan_t = get_time(scan_dt);

                    make_path(dst->code, destination, scan_dt - scan_t + conn->departure + conn->duration, promise_dt, forked);
                    active = find_and_modify(segment_by_state_and_parent, std::make_tuple(State::FUTURE, forked), State::ACTIVE);
                }
            }
        }
        else if (action == +Actions::INSCAN) {
            if (scan_dt < active->p_dep) {
                auto comment = Comment::INFO_SEGMENT_PREDICTED;

                if (scan_dt > active->a_arr)
                    comment = Comment::WARNING_CONNECTION_LATE_ARRIVAL;
                find_and_modify(segment_by_index, active->index, active->state, scan_dt, comment);
            }
            else {
                find_and_modify(segment_by_state, State::FUTURE, State::INACTIVE, scan_dt);
                find_and_modify(segment_by_state, State::ACTIVE, State::FAIL, Comment::FAILURE_CONNECTION_LATE_ARRIVAL);
                make_path(location, destination, scan_dt, promise_dt, active->parent);
                active = find_and_modify(segment_by_state_and_parent, std::make_tuple(State::FUTURE, active->parent), State::ACTIVE, scan_dt);
            }
        }
    }
    else if (active->code != location) {
        if (action == +Actions::LOCATION or action == +Actions::INSCAN) {
            find_and_modify(segment_by_state, State::FUTURE, State::INACTIVE);
            find_and_modify(segment_by_state, State::ACTIVE, State::FAIL, Comment::INFO_SEGMENT_BAD_DATA);
            make_path(location, destination, scan_dt, promise_dt, active->parent);
            active = find_and_modify(segment_by_state_and_parent, std::make_tuple(State::FUTURE, active->parent), State::ACTIVE, scan_dt);
        }
    }
}
