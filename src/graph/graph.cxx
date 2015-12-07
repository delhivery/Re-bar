#include <iostream>

#include <graph.hpp>

DeliveryCenter::DeliveryCenter() {}

DeliveryCenter::DeliveryCenter(int idx, std::string name, std::string code) : index(idx), code(code), name(name) {}

Connection::Connection() {}

Connection::Connection(
    std::size_t index, const double departure, const double duration, const double cost,
    std::string name) : index(index), departure(departure), duration(duration), cost(cost), name(name) {}

Cost Connection::weight(Cost distance, double t_max) {
    double c_parent = distance.first;
    double t_parent = distance.second;

    double t_durinal = fmod(t_parent, HOURS_IN_DAY);
    double t_wait = t_durinal <= departure ? departure - t_durinal: departure + HOURS_IN_DAY - t_durinal;
    double t_total = t_parent + t_wait + duration;

    if (t_total <= t_max) {
        return std::pair<double, double>(c_parent + cost, t_total);
    }
    return std::pair<double, double>(std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity());
}

double Connection::wait_time(double t_parent) const {
    double t_durinal = fmod(t_parent, HOURS_IN_DAY);
    double t_wait = t_durinal <= departure ? departure - t_durinal: departure + HOURS_IN_DAY - t_durinal;
    return t_wait;
}

void EPGraph::add_vertex(std::string code, std::string name) {
    Vertex vertex = boost::add_vertex(DeliveryCenter(boost::num_vertices(g), code, name), g);
    vertex_map[code] = vertex;
}

void EPGraph::add_vertex(std::string code) {
    add_vertex(code, code);
}

void EPGraph::add_edge(std::string src, std::string dest, double dep, double dur, double cost, std::string name) {
    if (vertex_map.find(src) == vertex_map.end()) {
        std::cout << "Unable to find source " << src << " for edge in vertex map" << std::endl;
        return;
    }

    if (vertex_map.find(dest) == vertex_map.end()) {
        std::cout << "Unable to find destination " << dest << " for edge in vertex map" << std::endl;
        return;
    }

    Vertex source = vertex_map[src];
    Vertex destination = vertex_map[dest];
    
    Connection conn = Connection(boost::num_edges(g), dep, dur, cost, name);
    boost::add_edge(source, destination, conn, g);
}

void EPGraph::add_edge(std::string src, std::string dest, double dep, double dur, double cost) {
    std::ostringstream ss;
    ss << dep;
    add_edge(src, dest, dep, dur, cost, src + "-" + dest + "@" + ss.str());
}
