#include <marge.hpp>

Solver::Solver(std::string database, int mode) : database(database) {
    mc = MongoReader{database};

    switch(mode) {
        case 0:
            path_finder = new ConstrainedEP();
            break;
        case 1:
            path_finder = new SimpleEP();
            break;
        default:
            path_finder = new SimpleEP();
            break;
    }
}

void Solver::init() {
    mc.init();
    bsoncxx::builder::stream::document filter;
    filter << "active" << true;

    for (auto node: mc.query(nodes_collection, filter, std::vector<std::string>{"code", "name"})) {
        path_finder->add_vertex(node["code"], node["name"]);
    }

    for (
            auto edge: mc.query(edges_collection, filter, std::vector<std::string>{
                "origin.code", "destination.code", "departure", "duration", "cost"})
    ) {
        path_finder->add_edge(
            edge["origin.code"], edge["destination.code"], std::stod(edge["departure"]),
            std::stod(edge["duration"]), std::stod(edge["cost"])
        );
    }
}

auto Solver::solve(std::string origin, std::string destination, double dt_start, double dt_promise) {
    double t_start = dt_start;
    double t_tat = dt_promise - dt_start;

    auto paths = path_finder->find_path(origin, destination, t_start, t_tat);
    return paths;
}
