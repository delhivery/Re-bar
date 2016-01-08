#include <bsoncxx/builder/stream/document.hpp>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include <rebar.hpp>

const std::vector<std::string> Segment::_keywords = std::vector<std::string>{"_id", "cn", "ed", "sol", "pa", "pd", "aa", "ad", "tip", "tap", "top", "cst", "st", "rmk", "par"};

Segment::Segment(
        std::string index, std::string code, std::string cname, std::string soltype,
        double p_arr, double p_dep, double a_arr, double a_dep,
        double t_inb_proc, double t_agg_proc, double t_out_proc,
        double cost,
        State state, Comment comment, const Segment* parent
) : 
        index(index), code(code), cname(cname), soltype(soltype),
        p_arr(p_arr), p_dep(p_dep), a_arr(a_arr), a_dep(a_dep),
        t_inb_proc(t_inb_proc), t_agg_proc(t_agg_proc), t_out_proc(t_out_proc),
        cost(cost), state(state), comment(comment), parent(parent) {
}

Segment::Segment(
        std::string index, std::string code, std::string cname, std::string soltype,
        double p_arr, double p_dep, double a_arr, double a_dep,
        double t_inb_proc, double t_agg_proc, double t_out_proc,
        double cost,
        std::string state, std::string comment, const Segment* parent
    ) {
    Segment(
        index, code, cname, soltype,
        p_arr, p_dep, a_arr, a_dep,
        t_inb_proc, t_agg_proc, t_out_proc,
        cost,
        State::_from_string(state.c_str()),
        Comment::_from_string(comment.c_str()),
        parent
    );
}

void Segment::set_meta(std::map<std::string, std::experimental::any> _meta_data) {
    for (auto item: _meta_data) {
        meta_data[item.first] = item.second;
    }
}

long double_to_long(double val) {
    if(val > 4102444800) {
        return 4102444800;
    }
    else if(val < 946684800) {
        return 946684800;
    }
    return long(val);
}

std::map<std::string, std::experimental::any> Segment::to_store() const {
    std::map<std::string, std::experimental::any> data;
    data["_id"] = bsoncxx::oid(index);
    data["cn"] = code;
    data["ed"] = cname;
    data["sol"] = soltype;
    data["pa"] = time_t(double_to_long(p_arr));
    data["pd"] = time_t(double_to_long(p_dep));
    data["aa"] = time_t(double_to_long(a_arr));
    data["ad"] = time_t(double_to_long(a_dep));
    data["cst"] = cost;
    data["tip"] = t_inb_proc;
    data["tap"] = t_agg_proc;
    data["top"] = t_out_proc;
    data["st"] = std::string(state._to_string());
    data["rmk"] = std::string(comment._to_string());
    data["par"] = std::string("");
    auto par = pindex();

    if (par != "") {
        data["par"] = bsoncxx::oid(par);
    }

    for (auto const& element: meta_data) {
        data[element.first] = element.second;
    }

    return data;
}


bool Segment::match(std::string _cname, double _a_dep) {
    if(cname == _cname) {
        if (std::abs(_a_dep - p_dep) < (HOURS_IN_DAY - t_agg_proc - t_out_proc)) {
            return true;
        }
    }

    return false;
}

std::string Segment::pindex() const {
    return (parent == nullptr) ? "" : parent->index;
}

ParserGraph::ParserGraph(std::string waybill, double promise_dt, std::shared_ptr<Solver> solver, std::shared_ptr<Solver> fallback) : waybill(waybill), promise_dt(promise_dt), solver(solver), fallback(fallback) {
    load_segment();

    if (segment_by_index.size() == 0) {
        make_root();
    }
}

ParserGraph::~ParserGraph() {
    if (save_state and segment_by_index.size() > 1) {
        MongoWriter mw{"rebar"};
        mw.init();
        boost::multi_index::index<SegmentContainer, SegmentId>::type& iter = segment.get<SegmentId>();
        mw.write("segments", iter);
    }
}

void ParserGraph::make_root() {
    Segment seg{
        bsoncxx::oid(bsoncxx::oid::init_tag).to_string(),   // Unique Id
        "",                                                 // Code
        "",                                                 // Cname
        "AUTOGEN",                                          // Soltype
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

    seg.set_meta(std::map<std::string, std::experimental::any>{{"wbn", waybill},{"pdd", time_t(long(promise_dt))}});
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

    for (auto const& result: mc.query("segments", filter, options)) {
        segment.insert(Segment::from_map(result, segment_by_index));
    }
}

std::string ParserGraph::make_duplicate_active(Segment* seg, std::shared_ptr<Connection> conn, const Segment* parent, double scan_dt) {
    Segment newseg{
        bsoncxx::oid(bsoncxx::oid::init_tag).to_string(),
        seg->code,
        conn->name,
        seg->soltype,
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

bool ParserGraph::make_path(std::string origin, std::string destination, double start_dt, const Segment* parent) {
    double start_t = get_time(start_dt);
    double max_t = promise_dt - start_dt;
    std::string soltype = "RCSP";
    auto solution = solver->solve(origin, destination, start_t, max_t);

    if (solution.size() == 0) {
        solution = fallback->solve(origin, destination, start_t, max_t);
        soltype = "STSP";
    }

    if (solution.size() == 0) {
        return false;
    }

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
            soltype,
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
        seg.set_meta(std::map<std::string, std::experimental::any>{{"wbn", waybill},{"pdd", time_t(long(promise_dt))}});
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
        "TERMINAL",
        soltype,
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
    seg.set_meta(std::map<std::string, std::experimental::any>{{"wbn", waybill},{"pdd", time_t(long(promise_dt))}});
    segment.insert(seg);

    return true;
}

void ParserGraph::save(bool _save_state) {
    save_state = _save_state;
}

void ParserGraph::show() {
    auto const& raii = segment.get<aux>();

    for(auto iter = raii.begin(); iter != raii.end(); iter++ ) {
        auto elem = *iter;
        std::cout << "Idx: " << elem.index << ",State: " << elem.state << ",Parent Index: " << ((elem.parent != nullptr)?elem.parent->index:"") << std::endl;
    }
}

void ParserGraph::parse_scan(std::string location, std::string destination, std::string connection, Actions action, double scan_dt) {
    Segment* active = find(segment_by_state, State::ACTIVE);

    if (active == nullptr)
        return;

    if (active->parent == nullptr) {
        if ((action == +Actions::LOCATION) or (action == +Actions::INSCAN)) {
            // generate new path against root
            make_path(location, destination, scan_dt, active);

            Segment* reached = find_and_modify(segment_by_state, State::ACTIVE, State::REACHED);

            if (reached != nullptr) {
                active = find_and_modify(segment_by_state_and_parent, std::make_tuple(State::FUTURE, reached->index), State::ACTIVE, scan_dt);
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
                find_and_modify(segment_by_state_and_parent, std::make_tuple(State::FUTURE, active->index), State::ACTIVE, -1, scan_dt);
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

                    make_path(dst->code, destination, scan_dt - scan_t + conn->departure + conn->duration, forked);
                    active = find_and_modify(segment_by_state_and_parent, std::make_tuple(State::FUTURE, forked->index), State::ACTIVE);
                }
            }
        }
        else if (action == +Actions::INSCAN) {
            if (scan_dt < active->p_dep) {
                auto comment = Comment::INFO_SEGMENT_PREDICTED;

                if (scan_dt > active->a_arr)
                    comment = Comment::WARNING_CONNECTION_LATE_ARRIVAL;
                find_and_modify(segment_by_index, active->index, active->state, scan_dt, -1, comment);
            }
            else {
                find_and_modify(segment_by_state, State::FUTURE, State::INACTIVE, scan_dt);
                find_and_modify(segment_by_state, State::ACTIVE, State::FAIL, Comment::FAILURE_CONNECTION_LATE_ARRIVAL);
                make_path(location, destination, scan_dt, active->parent);
                active = find_and_modify(segment_by_state_and_parent, std::make_tuple(State::FUTURE, active->parent->index), State::ACTIVE, scan_dt);
            }
        }
    }
    else if (active->code != location) {
        if (action == +Actions::LOCATION or action == +Actions::INSCAN) {
            find_and_modify(segment_by_state, State::FUTURE, State::INACTIVE);
            find_and_modify(segment_by_state, State::ACTIVE, State::FAIL, Comment::INFO_SEGMENT_BAD_DATA);
            make_path(location, destination, scan_dt, active->parent);
            active = find_and_modify(segment_by_state_and_parent, std::make_tuple(State::FUTURE, active->parent->index), State::ACTIVE, scan_dt);
        }
    }
}
