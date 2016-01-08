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

    for (auto const& node: mc.query(nodes_collection, filter)) {
        path_finder->add_vertex(
            std::experimental::any_cast<std::string>(node.at("cd")),
            std::experimental::any_cast<std::string>(node.at("nm"))
        );
    }

    for (
            auto const& edge: mc.query(edges_collection, filter)
    ) {
        double cost = 0;

        if (mode < 2) {
            cost = std::experimental::any_cast<double>(edge.at("cst"));
        }

        path_finder->add_edge(
            std::experimental::any_cast<std::string>(edge.at("ori")),
            std::experimental::any_cast<std::string>(edge.at("dst")),
            std::experimental::any_cast<double>(edge.at("dep")),
            std::experimental::any_cast<double>(edge.at("dur")),
            std::experimental::any_cast<double>(edge.at("tip")),
            std::experimental::any_cast<double>(edge.at("tap")),
            std::experimental::any_cast<double>(edge.at("top")),
            cost,
            std::experimental::any_cast<std::string>(edge.at("idx"))
        );
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
