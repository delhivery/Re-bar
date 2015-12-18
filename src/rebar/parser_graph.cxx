#include <bsoncxx/builder/stream/document.hpp>

#include <cstdlib>
#include <iostream>

#include <parser_graph.hpp>
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

ParserGraph::ParserGraph(std::string waybill, std::shared_ptr<Solver> solver) : waybill(waybill), solver(solver) {
    load_path();

    if (path_by_index.size() == 0)
        make_root();
}

void ParserGraph::make_root() {
    Segment seg{
        bsoncxx::oid(bsoncxx::oid::init_tag).to_string(),   // Unique Id
        "",                                                 // Code
        "",                                                 // Cname
        N_INF,                                              // p_arr
        P_INF,                                              // p_dep
        N_INF,                                              // a_arr
        P_INF,                                              // a_dep
        P_INF,                                              // t_inb_proc
        P_INF,                                              // t_agg_proc
        P_INF,                                              // t_out_proc
        0,                                                  // cost
        State::ACTIVE,                                      // state
        Comment::INFO_SEGMENT_PREDICTED,                    // comment
    };
    path.insert(seg);
}

void ParserGraph::load_path() {
    MongoReader mc{"rebar"};
    mc.init();

    mongocxx::options::find options;
    bsoncxx::builder::stream::document sorter;
    sorter << "_id" << 1;
    options.sort(sorter.view());

    bsoncxx::builder::stream::document filter;
    filter << "wbn" << waybill;

    for(auto result: mc.query(
            "paths",
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

            if (path_by_index.count(parent) != 1)
                throw std::invalid_argument("Unable to find parent with _id: " + parent);

            parent_ptr = std::shared_ptr<Segment>(&(*path_by_index.find(parent)));
        }

        path.insert(Segment{
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

void ParserGraph::make_path(std::string origin, std::string destination, double start_dt, double promise_dt, std::shared_ptr<Segment> parent) {
    auto solution = solver->solve(origin, destination, get_time(start_dt), promise_dt - start_dt);

    for(auto segment: solution) {
        Segment seg{
            bsoncxx::oid(bsoncxx::oid::init_tag).to_string(),   // Unique Id
            segment.destination,                                // Code
            segment.connection,                                 // Cname
            start_dt + segment.arrival,                         // p_arr
            P_INF,                                              // p_dep
            N_INF,                                              // a_arr
            P_INF,                                              // a_dep
            P_INF,                                              // t_inb_proc
            P_INF,                                              // t_agg_proc
            P_INF,                                              // t_out_proc
            segment.cost,                                       // cost
            State::FUTURE,                                      // state
            Comment::INFO_SEGMENT_PREDICTED,                    // comment
            parent
        };
        path.insert(seg);
        parent = std::shared_ptr<Segment>(&seg);
    }
}

void ParserGraph::parse_scan(std::string location, std::string destination, std::string action, double scan_dt, double promise_dt) {
    auto active = *path_by_state.find(State::ACTIVE);

    if (active.parent == NULL) {
        make_path(location, destination, scan_dt, promise_dt, std::shared_ptr<Segment>(&active));
    }
}
