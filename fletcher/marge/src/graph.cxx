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

EdgeProperty::EdgeProperty(const size_t _index, EdgeProperty another) {
    index = _index;
    _dep = another._dep;
    _dur = another._dur;
    _tip = another._tip;
    _tap = another._tap;
    _top = another._tip;
    cost = another.cost;
    code = another.code;
    dep = another.dep;
    dur = another.dur;
}

EdgeAll::EdgeAll(const size_t _index, const size_t _src, const size_t _dst, const long __tip, const long __tap, const long __top, const double _cost, string_view _code) : EdgeProperty(_index, __tip, __tap, __top, _cost, _code), src(_src), dst(_dst) {}

EdgeAll::EdgeAll(const size_t _index, const size_t _src, const size_t _dst, const long __dep, const long __dur, const long __tip, const long __tap, const long __top, const double _cost, string_view _code) : EdgeProperty::EdgeProperty(_index, __dep, __dur, __tip, __tap, __top, _cost, _code), src(_src), dst(_dst)  {}


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

    if (vertex_map.find(code.to_string()) == vertex_map.end()) {

        VertexProperty vprop{boost::num_vertices(g), code};
        Vertex created = boost::add_vertex(vprop, g);
        vertex_map[vprop.code] = created;
    }
    else {
        throw invalid_argument("Unable to add vertex. Duplicate code specified");
    }
}

void BaseGraph::add_edge(string_view src, string_view dst, string_view conn, const long tip, const long tap, const long top, const double cost) {
    if (vertex_map.find(src.to_string()) == vertex_map.end()) {
        throw domain_error("C: Invalid source <" + src.to_string() + "> specified");
    }

    if (vertex_map.find(dst.to_string()) == vertex_map.end()) {
        throw domain_error("C: Invalid destination <" + dst.to_string() + "> specified");
    }

    if (edge_map.find(conn.to_string()) == edge_map.end()) {
        size_t sindex = vertex_map.at(src.to_string());
        size_t dindex = vertex_map.at(dst.to_string());

        unique_lock<shared_timed_mutex> graph_write_lock(graph_mutex, defer_lock);
        graph_write_lock.lock();

        EdgeProperty eprop{boost::num_edges(g), tip, tap, top, cost, conn};
        auto created = boost::add_edge(sindex, dindex, eprop, g);

        if (created.second) {
            edge_map[eprop.code] = created.first;
            edge_map_all[eprop.code] = EdgeAll(eprop.index, sindex, dindex, tip, tap, top, cost, conn);
        }
        else {
            throw runtime_error("Unable to create edge");
        }
    }
    else {
        throw invalid_argument("Unable to create edge. Duplicate connection specified");
    }
}

void BaseGraph::toggle_edge(string_view conn, bool state) {
    if (state == true) {
        if (edge_map.find(conn.to_string()) == edge_map.end()) {
            if (edge_map_all.find(conn.to_string()) == edge_map_all.end()) {
                throw domain_error("Invalid edge <" + conn.to_string() + "> specified");
            }
            unique_lock<shared_timed_mutex> graph_write_lock(graph_mutex, defer_lock);
            graph_write_lock.lock();

            EdgeAll eprop_all = edge_map_all.at(conn.to_string());
            EdgeProperty eprop{boost::num_edges(g), eprop_all};
            auto created = boost::add_edge(eprop_all.src, eprop_all.dst, eprop, g);

            if (created.second) {
                edge_map[eprop.code] = created.first;
            }
            else {
                throw runtime_error("Unable to create edge");
            }
        }
    }
    else {
        if (edge_map.find(conn.to_string()) != edge_map.end()) {
            Edge edesc = edge_map.at(conn.to_string());
            boost::remove_edge(edesc, g);
            edge_map.erase(edge_map.find(conn.to_string()), edge_map.end());
        }
    }
}

void BaseGraph::add_edge(string_view src, string_view dst, string_view conn, const long dep, const long dur, const long tip, const long tap, const long top, const double cost) {
    if (vertex_map.find(src.to_string()) == vertex_map.end()) {
        throw domain_error("E: Invalid source <" + src.to_string() + "> specified");
    }

    if (vertex_map.find(dst.to_string()) == vertex_map.end()) {
        throw domain_error("E: Invalid destination <" + dst.to_string() + "> specified");
    }

    if (edge_map.find(conn.to_string()) == edge_map.end()) {

        size_t sindex = vertex_map.at(src.to_string());
        size_t dindex = vertex_map.at(dst.to_string());

        unique_lock<shared_timed_mutex> graph_write_lock(graph_mutex, defer_lock);
        graph_write_lock.lock();

        EdgeProperty eprop{boost::num_edges(g), dep, dur, tip, tap, top, cost, conn};
        auto created = boost::add_edge(sindex, dindex, eprop, g);

        if (created.second) {
            edge_map[eprop.code] = created.first;
            edge_map_all[eprop.code] = EdgeAll(eprop.index, sindex, dindex, tip, tap, top, cost, conn);
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
    if (vertex_map.find(vertex.to_string()) == vertex_map.end())
        throw domain_error("No source vertex<" + vertex.to_string() + "> found in database");

    if (edge_map.find(edge.to_string()) == edge_map.end())
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
        check_kwargs(kwargs, "code");
        string value = any_cast<string>(kwargs.at("code"));
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
        string src, dst, conn;
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

json_map BaseGraph::modc(shared_ptr<BaseGraph> solver, const map<string, any>& kwargs) {
    json_map response;

    try {
        string code;
        long state;
        bool enabled;
        check_kwargs(kwargs, list<string_view>{"code", "state"});

        code = any_cast<string>(kwargs.at("code"));
        state = any_cast<long>(kwargs.at("state"));

        enabled = false;

        if (state == 1) {
            enabled = true;
        }

        solver->toggle_edge(code, enabled);
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
        string src, dst, conn;
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
        string vertex = any_cast<string>(kwargs.at("src"));
        string edge = any_cast<string>(kwargs.at("conn"));
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
        string src = any_cast<string>(kwargs.at("src"));
        string dst = any_cast<string>(kwargs.at("dst"));
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
