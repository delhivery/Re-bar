#include <mutex>

#include "graph.hpp"

bool operator < (const Cost& first, const Cost& second) {
    if (second.second == P_L_INF) {
        return true;
    }
    return (first.first < second.first) ? true : ((first.first > second.first) ? false : (first.second < second.second));
}

Path::Path(string_view _src, string_view _conn, string_view _dst, long _arr, long _dep, double _cost) : src(_src), conn(_conn), dst(_dst), arr(_arr), dep(_dep), cost(_cost) {}

VertexProperty::VertexProperty(size_t _index, string_view _code) {
    index = _index;
    code = _code.to_string();
}

EdgeProperty::EdgeProperty(const size_t _index, const long __tip, const long __tap, const long __top, const double _cost, string_view _code) : index(_index), code(_code.to_string()), _tip(__tip), _tap(__tap), _top(__top), cost(_cost) {
    percon = true;
}

EdgeProperty::EdgeProperty(const size_t _index, const long __dep, const long __dur, const long __tip, const long __tap, const long __top, const double _cost, string_view _code) {
    index = _index;
    _dep = __dep;
    _dur = __dur;
    _tip = __tip;
    _tap = __tap;
    _top = __top;
    cost = _cost;
    code = _code.to_string();

    dep = _dep - _tap - _top;
    dur = _dur + _tap + _top + _tip;
}

long EdgeProperty::wait_time(const long t_departure) const {
    if (percon)
        return 0;

    auto t_departure_durinal = t_departure % TIME_DURINAL;
    return (t_departure_durinal > dep) ? (TIME_DURINAL - t_departure_durinal + dep) : (dep - t_departure_durinal);
}

Cost EdgeProperty::weight(const Cost& start, const long t_max) {
    if (percon) {
        return Cost{start.first, start.second + _tip + _tap + _top};
    }

    auto cost_total = start.first + cost;
    long time_total = wait_time(start.second) + dur + start.second;
    time_total = (time_total > t_max) ? P_L_INF : time_total;
    cost_total = (time_total == P_L_INF) ? P_D_INF : cost_total;

    return Cost{cost_total, time_total};
}

void BaseGraph::check_kwargs(const map<string, any>& kwargs, string_view key) {
    if (kwargs.find(key.to_string()) == kwargs.end()) {
        char buffer[255];
        sprintf(buffer, "Missing required argument \"%s\"", key.to_string().c_str());
        throw invalid_argument(buffer);
    }
}

void BaseGraph::check_kwargs(const map<string, any>& kwargs, const list<string_view>& keys) {
    for(string_view key: keys) {
        check_kwargs(kwargs, key);
    }
}

void BaseGraph::add_vertex(string_view code) {
    unique_lock<shared_timed_mutex> graph_write_lock(graph_mutex, defer_lock);
    graph_write_lock.lock();

    if (vertex_map.find(code) == vertex_map.end()) {

        VertexProperty vprop{boost::num_vertices(g), code};
        Vertex created = boost::add_vertex(vprop, g);
        vertex_map[vprop.code] = created;
    }
    else {
        throw invalid_argument("Unable to add vertex. Duplicate code specified");
    }
}

void BaseGraph::add_edge(string_view src, string_view dst, string_view conn, const long tip, const long tap, const long top, const double cost) {
    if (vertex_map.find(src) == vertex_map.end()) {
        throw domain_error("Invalid source <" + src.to_string() + "> specified");
    }

    if (vertex_map.find(dst) == vertex_map.end()) {
        throw domain_error("Invalid source <" + dst.to_string() + "> specified");
    }

    if (edge_map.find(conn) == edge_map.end()) {
        size_t sindex = vertex_map.at(src.to_string());
        size_t dindex = vertex_map.at(dst.to_string());

        unique_lock<shared_timed_mutex> graph_write_lock(graph_mutex, defer_lock);
        graph_write_lock.lock();

        EdgeProperty eprop{boost::num_edges(g), tip, tap, top, cost, conn};
        auto created = boost::add_edge(sindex, dindex, eprop, g);

        if (created.second) {
            edge_map[eprop.code] = created.first;
        }
        else {
            throw runtime_error("Unable to create edge");
        }
    }
    else {
        throw invalid_argument("Unable to create edge. Duplicate connection specified");
    }
}

void BaseGraph::add_edge(string_view src, string_view dst, string_view conn, const long dep, const long dur, const long tip, const long tap, const long top, const double cost) {
    if (vertex_map.find(src) == vertex_map.end()) {
        throw domain_error("Invalid source <" + src.to_string() + "> specified");
    }

    if (vertex_map.find(dst) == vertex_map.end()) {
        throw domain_error("Invalid source <" + dst.to_string() + "> specified");
    }

    if (edge_map.find(conn) == edge_map.end()) {

        size_t sindex = vertex_map.at(src.to_string());
        size_t dindex = vertex_map.at(dst.to_string());

        unique_lock<shared_timed_mutex> graph_write_lock(graph_mutex, defer_lock);
        graph_write_lock.lock();

        EdgeProperty eprop{boost::num_edges(g), dep, dur, tip, tap, top, cost, conn};
        auto created = boost::add_edge(sindex, dindex, eprop, g);

        if (created.second) {
            edge_map[eprop.code] = created.first;
        }
        else {
            throw runtime_error("Unable to create edge.");
        }
    }
    else {
        throw invalid_argument("Unable to create edge. Duplicate connection specified");
    }
}

pair<EdgeProperty, VertexProperty> BaseGraph::lookup(string_view vertex, string_view edge) {
    if (vertex_map.find(vertex) == vertex_map.end())
        throw domain_error("No source vertex<" + vertex.to_string() + "> found in database");

    if (edge_map.find(edge) == edge_map.end())
        throw domain_error("Connection<" + edge.to_string() + "> not in database");

    Vertex vdesc = vertex_map.at(vertex.to_string());
    Edge edesc = edge_map.at(edge.to_string());

    if (boost::source(edesc, g) != vdesc) {
        throw invalid_argument("No connection<" + edge.to_string() + "> from source<" + vertex.to_string() + ">");
    }

    Vertex edest = boost::target(edesc, g);
    return std::make_pair(g[edesc], g[edest]);
}

json_map BaseGraph::addv(shared_ptr<BaseGraph> solver, const map<string, any>& kwargs) {
    json_map response;
    try {
        string_view value;
        check_kwargs(kwargs, "code");
        value = any_cast<string>(kwargs.at("code"));
        solver->add_vertex(value);
        response["success"] = true;
    }
    catch (const exception& exc) {
        response["error"] = exc.what();
    }

    return response;
}


json_map BaseGraph::adde(shared_ptr<BaseGraph> solver, const map<string, any>& kwargs) {
    json_map response;
    try {
        string_view src, dst, conn;
        long dep, dur, tip, top, tap;
        double cost;

        check_kwargs(kwargs, list<string_view>{"src", "dst", "conn", "dep", "dur", "tip", "tap", "top", "cost"});
        src  = any_cast<string>(kwargs.at("src"));
        dst  = any_cast<string>(kwargs.at("dst"));
        conn = any_cast<string>(kwargs.at("conn"));

        dep  = any_cast<long>(kwargs.at("dep"));
        dur  = any_cast<long>(kwargs.at("dur"));
        tip  = any_cast<long>(kwargs.at("tip"));
        tap  = any_cast<long>(kwargs.at("tap"));
        top  = any_cast<long>(kwargs.at("top"));

        cost = any_cast<double>(kwargs.at("cost"));

        solver->add_edge(src, dst, conn, dep, dur, tip, tap, top, cost);
        response["success"] = true;
    }
    catch (const exception& exc) {
        response["error"] = exc.what();
    }
    return response;
}

json_map BaseGraph::addc(shared_ptr<BaseGraph> solver, const map<string, any>& kwargs) {
    json_map response;
    try {
        string_view src, dst, conn;
        long tip, top, tap;
        double cost = 0.30;
        
        check_kwargs(kwargs, list<string_view>{"src", "dst", "conn", "tip", "tap", "top"});
        src = any_cast<string>(kwargs.at("src"));
        dst = any_cast<string>(kwargs.at("dst"));
        conn = any_cast<string>(kwargs.at("conn"));

        top = any_cast<long>(kwargs.at("tap"));
        tap = any_cast<long>(kwargs.at("top"));
        tip = any_cast<long>(kwargs.at("tip"));
        solver->add_edge(src, dst, conn, tip, top, tap, cost);
        response["success"] = true;
    }
    catch (const exception& exc) {
        response["error"] = exc.what();
    }
    return response;
}

json_map BaseGraph::look(shared_ptr<BaseGraph> solver, const map<string, any>& kwargs) {
    json_map response;

    try {
        check_kwargs(kwargs, list<string_view>{"src", "conn"});
        string_view vertex = any_cast<string>(kwargs.at("src"));
        string_view edge = any_cast<string>(kwargs.at("conn"));
        auto edge_property = solver->lookup(vertex, edge);

        json_map conn;
        conn["code"] = edge_property.first.code;
        conn["dep"]  = edge_property.first._dep;
        conn["dur"]  = edge_property.first._dur;
        conn["tip"]  = edge_property.first._tip;
        conn["tap"]  = edge_property.first._tap;
        conn["top"]  = edge_property.first._top;
        conn["cost"] = edge_property.first.cost;
        conn["dst"] = edge_property.second.code;

        response["connection"] = conn;
        response["success"]    = true;
    }
    catch (const exception& exc) {
        response["error"] = exc.what();
    }
    return response;
}

json_map BaseGraph::find(shared_ptr<BaseGraph> solver, const map<string, any>& kwargs) {
    json_map response;
    try {
        check_kwargs(kwargs, list<string_view>{"src", "dst", "beg", "tmax"});
        string_view src = any_cast<string>(kwargs.at("src"));
        string_view dst = any_cast<string>(kwargs.at("dst"));
        long t_start    = any_cast<long>(kwargs.at("beg"));
        long t_max      = any_cast<long>(kwargs.at("tmax"));

        auto path = solver->find_path(src, dst, t_start, t_max);
        json_array segments;

        for (auto const& segment: path) {
            json_map seg;
            seg["source"] = segment.src.to_string();
            seg["connection"] = segment.conn.to_string();
            seg["destination"] = segment.dst.to_string();
            seg["arrival_at_source"] = segment.arr;
            seg["departure_from_source"] = segment.dep;
            seg["cost_reaching_source"] = segment.cost;
            segments.push_back(seg);
        }
        response["path"] = segments;
        response["success"] = true;
    }
    catch (const exception& exc) {
        response["error"] = exc.what();
    }
    return response;
}