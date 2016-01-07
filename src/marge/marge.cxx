#include <marge.hpp>

Solver::Solver(std::string database, int mode) : database(database), mode(mode) {
    switch(mode) {
        case 0:
            path_finder = std::make_shared<ConstrainedEP>();
            break;
        case 1:
            path_finder = std::make_shared<SimpleEP>();
            break;
        default:
            path_finder = std::make_shared<SimpleEP>();
            break;
    }
}

void Solver::init() {
    auto mc = MongoReader{database};
    mc.init();
    bsoncxx::builder::stream::document filter;
    filter << "act" << true;

    for (auto const& node: mc.query(nodes_collection, filter, std::vector<std::string>{"cd", "nm"})) {
        path_finder->add_vertex(node.at("cd"), node.at("nm"));
    }

    for (
            auto edge: mc.query(edges_collection, filter, std::vector<std::string>{
                "ori", "dst", "dep", "dur", "cst", "tip", "tap", "top", "md", "nm", "md", "cap", "idx"})
    ) {
        if (mode < 2) {
            path_finder->add_edge(
                edge["ori"], edge["dst"],
                std::stod(edge["dep"]), std::stod(edge["dur"]),
                std::stod(edge["tip"]), std::stod(edge["tap"]), std::stod(edge["top"]),
                std::stod(edge["cst"]), edge["idx"]
            );
        }
        else {
            path_finder->add_edge(
                edge["ori"], edge["dst"],
                std::stod(edge["dep"]), std::stod(edge["dur"]),
                std::stod(edge["tip"]), std::stod(edge["tap"]), std::stod(edge["top"]),
                0, edge["idx"]
            );
        }
    }
}

std::vector<Path> Solver::solve(std::string origin, std::string destination, double dt_start, double dt_promise) {
    if (mode > 1)
        dt_promise = P_INF;

    auto paths = path_finder->find_path(origin, destination, dt_start, dt_promise);
    return paths;
}

std::tuple<std::shared_ptr<Connection>, std::shared_ptr<DeliveryCenter>, std::shared_ptr<DeliveryCenter> > Solver::lookup(std::string name) {
    return path_finder->lookup(name);
}
