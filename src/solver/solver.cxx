#include "mongo.hpp"
#include "graph.hpp"
#include "rcsp.hpp"

#include <iostream>

struct Edge {
    std::string origin;
    std::string destination;
    double departure, duration, cost;

    Edge(std::string o, std::string d, double dep, double dur, double c) : origin(o), destination(d), departure(dep), duration(dur), cost(c) {}
};

int main() {
    MongoClient mc{"rebar"};
    mc.init();

    expath::RCExpath path_finder;
    // expath::Expath path_finder;
    bsoncxx::builder::stream::document filter;
    filter << "active" << true;
    std::string code, name;

    std::vector<std::pair<std::string, std::string> > nodes;

    for (auto result: mc.query("nodes", filter, std::vector<std::string>{"code", "name"})) {
        path_finder.add_vertex(result["code"], result["name"]);
    }

    for (auto edge: mc.query("edges", filter, std::vector<std::string>{"origin.code", "destination.code", "departure", "duration", "cost"})) {
        path_finder.add_edge(
            edge["origin.code"], edge["destination.code"], std::stod(edge["departure"]),
            std::stod(edge["duration"]), std::stod(edge["cost"])
        );
    }

    int cases;
    std::cin >> cases;

    std::cout << "Multiplier,Origin,Destination,Start,TAT,Intermediary,Connection,Cost,Arrival" << std::endl;

    for(int i = 0; i < cases; i++) {
        std::string origin, destination;
        double tat, start;
        std::cin >> origin;
        std::cin >> destination;
        std::cin >> start;
        std::cin >> tat;

        std::vector<std::string> response_ncost;
        auto paths = path_finder.time_dependent_shortest_path(origin, destination, start, tat + start);
        // auto paths = path_finder.get_path(origin, destination, start, tat + start);

        int idx = 0;
        for(auto iter = paths.rbegin(); iter != paths.rend(); iter++) {
            auto path = *iter;
            if(idx > 0) {
                std::ostringstream ss;
                ss <<  "1," << origin << "," << destination << "," << start << "," << tat << ",";
                ss << path;
                // ss << path.second.first << "," << path.second.second << "," << path.first.first << "," << path.first.second;
                response_ncost.push_back(ss.str());
            }
            idx++;
        }

        if (response_ncost.size() == 0) {
            std::cout << "1," << origin << "," << destination << "," << start << "," << tat << ",No valid paths" << std::endl;
        }

        for (size_t i = 0; i < response_ncost.size(); i++) {
            std::cout << response_ncost[i] << std::endl;
        }

        if (response_ncost.size() == 0) {
            response_ncost.empty();
            paths = path_finder.time_dependent_shortest_path(origin, destination, start, tat * 10 + start);
            // paths = path_finder.get_path(origin, destination, start, tat * 10 + start);

            idx = 0;
            for(auto iter = paths.rbegin(); iter != paths.rend(); iter++) {
                auto path = *iter;
                if(idx > 0) {
                    std::ostringstream ss;
                    ss <<  "10," << origin << "," << destination << "," << start << "," << tat * 10 << ",";
                    ss << path;
                    // ss << path.second.first << "," << path.second.second << "," << path.first.first << "," << path.first.second;
                    response_ncost.push_back(ss.str());
                }
                idx++;
            }

            if(response_ncost.size() == 0) {
                std::cout << "10," << origin << "," << destination << "," << start << "," << tat * 10 << ",No valid paths" << std::endl;
            }
            for(size_t i = 0; i < response_ncost.size(); i++) {
                std::cout << response_ncost[i] << std::endl;
            }
        }
    }

    return 0;
}
