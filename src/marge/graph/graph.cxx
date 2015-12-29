#include <iostream>
#include <utils.hpp>
#include <graph.hpp>

DeliveryCenter::DeliveryCenter() {}

DeliveryCenter::DeliveryCenter(int idx, std::string name, std::string code) : index(idx), code(code), name(name) {}

Connection::Connection() {}

Connection::Connection(
    std::size_t index, const double departure, const double duration, const double cost,
    std::string name) : index(index), departure(departure), duration(duration), cost(cost), name(name) {}

Connection::Connection(
        std::size_t index,
        const double _departure, const double _duration, const double _t_inb_proc, const double _t_agg_proc, const double _t_out_proc,
        const double cost,
        std::string name
) : index(index),
            _departure(_departure), _duration(_duration), _t_inb_proc(_t_inb_proc), _t_agg_proc(_t_agg_proc), _t_out_proc(_t_out_proc),
            cost(cost), name(name) {
    if(_duration < 0)
        std::cout << "Negative duration specified" << std::endl;
    departure = abs_durinal(_departure - _t_agg_proc - _t_out_proc);
    duration = _t_inb_proc + _t_agg_proc + _t_out_proc + _duration;
}

Path::Path(
    std::string destination, Connection& connection, double arrival, double cost
) : destination(destination), connection(connection), arrival(arrival), cost(cost) {}

Cost Connection::weight(Cost distance, double t_max) {
    double c_parent = distance.first;
    double t_parent = distance.second;

    double t_durinal = fmod(t_parent, HOURS_IN_DAY);
    double t_wait = t_durinal <= departure ? departure - t_durinal: departure + HOURS_IN_DAY - t_durinal;
    double t_total = t_parent + t_wait + duration;

    if (t_total <= t_max) {
        return std::pair<double, double>(c_parent + cost, t_total);
    }
    return std::pair<double, double>(P_INF, P_INF);
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

std::tuple<std::shared_ptr<Connection>, std::shared_ptr<DeliveryCenter>, std::shared_ptr<DeliveryCenter> > EPGraph::lookup(std::string name) {
    std::shared_ptr<Connection> conn = NULL;
    std::shared_ptr<DeliveryCenter> src = NULL;
    std::shared_ptr<DeliveryCenter> dst = NULL;

    typedef boost::graph_traits<Graph>::edge_iterator e_iter;
    e_iter ei, ei_end;

    for (tie(ei, ei_end) = boost::edges(g); ei != ei_end; ++ei) {
        auto connection = boost::get(boost::edge_bundle, g)[*ei];
        if (connection.name == name) {
            conn = std::make_shared<Connection>(connection);
            auto source = g[boost::source(*ei, g)];
            auto destination = g[boost::target(*ei, g)];
            src = std::make_shared<DeliveryCenter>(source);
            dst = std::make_shared<DeliveryCenter>(destination);

        }
    }
    return std::tie(conn, src, dst);
}

void EPGraph::add_edge(std::string src, std::string dest, double dep, double dur, double tip, double tap, double top, double cost, std::string name) {
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
    Connection conn{boost::num_edges(g), dep, dur, tip, tap, top, cost, name};
    boost::add_edge(source, destination, conn, g);
}
