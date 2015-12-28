#include <bsoncxx/builder/stream/document.hpp>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include <rebar.hpp>
#include <reader.hpp>


Segment::Segment(
        std::string index, std::string code, std::string cname,
        double p_arr, double p_dep, double a_arr, double a_dep,
        double t_inb_proc, double t_agg_proc, double t_out_proc,
        double cost,
        State state, Comment comment, std::shared_ptr<Segment> parent
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
        std::string state, std::string comment, std::shared_ptr<Segment> parent
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

ParserGraph::ParserGraph(std::string waybill, std::shared_ptr<Solver> solver) : waybill(waybill), solver(solver) {
    load_segment();

    if (segment_by_index.size() == 0) {
        make_root();
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

    for(auto result: mc.query(
            "segments",
            filter,
            std::vector<std::string>{
                "_id", "cn", "ed",
                "pa", "aa", "pd", "ad",
                "tip", "tap", "top",
                "st", "rmk",
                "cst", "par"
            },
            options
    )) {
        std::string index, code, cname, p_arr, a_arr, p_dep, a_dep, t_inb_proc, t_agg_proc, t_out_proc, cost, parent, state, comment;
        index = result["_id"];
        code = result["cn"];
        cname = result["ed"];
        p_arr = result["pa"];
        a_arr = result["aa"];
        p_dep = result["p_dep"];
        a_dep = result["a_dep"];
        t_inb_proc = result["tip"];
        t_agg_proc = result["tap"];
        t_out_proc = result["top"];
        state = result["st"];
        comment = result["rmk"];
        cost = result["cst"];
        parent = result["par"];

        std::shared_ptr<Segment> parent_ptr = NULL;

        if(parent != "") {

            if (segment_by_index.count(parent) != 1)
                throw std::invalid_argument("Unable to find parent with _id: " + parent);

            parent_ptr = std::shared_ptr<Segment>(&(*segment_by_index.find(parent)));
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

std::shared_ptr<Segment> ParserGraph::make_duplicate_active(Segment& seg, std::shared_ptr<Connection> conn, std::shared_ptr<Segment> parent, double scan_dt) {
    Segment newseg{
        bsoncxx::oid(bsoncxx::oid::init_tag).to_string(),
        seg.code,
        conn->name,
        seg.p_arr,
        seg.p_dep,
        seg.a_arr,
        scan_dt,
        seg.t_inb_proc,
        conn->_t_out_proc,
        conn->_t_agg_proc,
        seg.cost,
        State::REACHED,
        Comment::SUCCESS_SEGMENT_TRAVERSED,
        parent
    };

    segment.insert(newseg);
    return std::shared_ptr<Segment>(&newseg);
}

bool ParserGraph::make_path(std::string origin, std::string destination, double start_dt, double promise_dt, std::shared_ptr<Segment> parent) {
    double start_t = get_time(start_dt);
    double max_t = promise_dt - start_dt;
    auto solution = solver->solve(origin, destination, start_t, max_t);

    if (solution.size() == 0)
        return false;

    auto origin_center = origin;
    auto origin_arrival = start_dt - start_t;
    auto arrival_cost = start_t;
    double base_shipping_cost = 0;

    if (parent != NULL) {
        base_shipping_cost = parent->cost;
    }

    double shipping_cost = 0;
    double inbound = 0;

    for (auto path: solution) {
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
        parent = std::shared_ptr<Segment>(&seg);
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

void ParserGraph::parse_scan(std::string location, std::string destination, std::string connection, Actions action, double scan_dt, double promise_dt) {
    auto active = *segment_by_state.find(State::ACTIVE);

    if (active.parent == NULL) {
        if ((action == Actions::LOCATION) or (action == Actions::INSCAN)) {
            // generate new path against root
            make_path(location, destination, scan_dt, promise_dt, std::shared_ptr<Segment>{&active});

            std::shared_ptr<Segment> reached{&(*segment_by_state.find(State::ACTIVE))};
            segment_by_state.modify(segment_by_state.find(State::ACTIVE), [&scan_dt](Segment& seg) {
                seg.state = State::REACHED;
            });

            std::shared_ptr<Segment> active{&(*segment_by_state_and_parent.find(std::make_tuple(State::FUTURE, reached)))};
            segment_by_state_and_parent.modify(
                segment_by_state_and_parent.find(std::make_tuple(State::FUTURE, reached)),
                [&scan_dt](Segment& seg) {
                    seg.state = State::ACTIVE;
                    seg.a_arr = scan_dt;
            });
        }
        else {
            // Root and outbound, Do nothing
        }
    }
    else if(active.code == location) {

        if (action == Actions::OUTSCAN) {

            if (active.match(connection, scan_dt)) {
                // Connection matches
                auto comment = Comment::SUCCESS_SEGMENT_TRAVERSED;

                if (active.p_dep < scan_dt)
                    comment = Comment::WARNING_CENTER_DELAYED_CONNECTION;

                // Mark active as reached
                segment_by_index.modify(
                    segment_by_index.find(active.index),
                    [&scan_dt, &comment](Segment &seg) {
                        seg.comment = comment;
                        seg.state = State::REACHED;
                        seg.a_dep = scan_dt;
                });

                segment_by_state_and_parent.modify(
                    segment_by_state_and_parent.find(std::make_tuple(State::FUTURE, active)),
                    [&scan_dt](Segment& seg) {
                        seg.state = State::ACTIVE;
                        seg.a_arr = scan_dt;
                });
            }
            else {
                auto old_parent = active.parent;

                segment_by_state.modify(
                    segment_by_state.find(State::FUTURE),
                    [](Segment& seg) {
                        seg.comment = Comment::INFO_SEGMENT_BAD_DATA;
                        seg.state = State::INACTIVE;
                });

                segment_by_index.modify(
                    segment_by_index.find(active.index),
                    [&scan_dt](Segment& seg) {
                        seg.state = State::FAIL;
                        seg.comment = Comment::FAILURE_CENTER_OVERRIDE_CONNECTION;
                });

                std::shared_ptr<Connection> conn;
                std::shared_ptr<DeliveryCenter> src, dst;
                std::tuple<std::shared_ptr<Connection>, std::shared_ptr<DeliveryCenter>, std::shared_ptr<DeliveryCenter> > {conn, src, dst} = solver->lookup(
                    connection);

                if (conn == NULL) {
                    throw std::invalid_argument("Unable to find a connection with index: " + connection);
                }

                auto reached = make_duplicate_active(active, conn, old_parent, scan_dt);
                auto scan_t = get_time(scan_dt);

                make_path(
                    dst->code,
                    destination,
                    scan_dt - scan_t + conn->departure + conn->duration,
                    promise_dt,
                    reached
                );

                segment_by_state_and_parent.modify(
                    segment_by_state_and_parent.find(std::make_tuple(State::FUTURE, reached)),
                    [](Segment& seg) {
                        seg.state = State::ACTIVE;
                });
            }
            // Outscanned from current location
            // Mark active reached
            // Mark future active
        }
        else if (action == Actions::INSCAN) {
            // Inscanned at active
            // Record actual arrival
            if (scan_dt < active.p_dep) {
                auto comment = Comment::INFO_SEGMENT_PREDICTED;

                if (scan_dt > active.a_arr)
                    comment = Comment::WARNING_CONNECTION_LATE_ARRIVAL;

                segment_by_index.modify(
                    segment_by_index.find(active.index),
                    [&scan_dt, &comment](Segment& seg) {
                        seg.comment = comment;
                        seg.a_arr = scan_dt;
                });
            }
            else {
                segment_by_state.modify(
                    segment_by_state.find(State::FUTURE),
                    [&scan_dt](Segment& seg) {
                        seg.state = State::INACTIVE;
                        seg.a_arr = scan_dt;
                });

                segment_by_state.modify(
                    segment_by_state.find(State::ACTIVE),
                    [](Segment& seg) {
                        seg.state = State::FAIL;
                        seg.comment = Comment::FAILURE_CONNECTION_LATE_ARRIVAL;
                });

                make_path(location, destination, scan_dt, promise_dt, active.parent);

                segment_by_state_and_parent.modify(
                    segment_by_state_and_parent.find(std::make_tuple(State::FUTURE, active.parent)),
                    [&scan_dt](Segment& seg) {
                        seg.state = State::ACTIVE;
                        seg.a_arr = scan_dt;
                });
            }
        }
    }
    else if (active.code != location) {
        if (action == Actions::LOCATION or action == Actions::INSCAN) {
            auto old_parent = active.parent;
            segment_by_state.modify(
                segment_by_state.find(State::FUTURE),
                [](Segment &seg) {
                    seg.state = State::INACTIVE;
            });

            segment_by_state.modify(
                segment_by_state.find(State::ACTIVE),
                [](Segment &seg) {
                    seg.state = State::FAIL;
                    seg.comment = Comment::INFO_SEGMENT_BAD_DATA;
            });

            make_path(location, destination, scan_dt, promise_dt, old_parent);

            segment_by_state_and_parent.modify(
                segment_by_state_and_parent.find(std::make_tuple(State::FUTURE, old_parent)),
                [&scan_dt](Segment &seg) {
                    seg.state = State::ACTIVE;
                    seg.a_arr = scan_dt;
            });
        }
    }
}
