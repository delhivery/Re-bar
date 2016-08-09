//
// Created by amitprakash on 8/3/16.
//

#ifndef FLETCHER_EXPATH_SERVER_HXX
#define FLETCHER_EXPATH_SERVER_HXX

#include <experimental/string_view>

#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>

#include "expath.grpc.pb.h"
#include "structures.hxx"
#include "graph/graph_helpers.hxx"

typedef std::map<std::string, std::string> Features;
typedef EdgeProperty<Features, Cost> EProp;
typedef boost::adjacency_list<
        boost::vecS,
        boost::vecS,
        boost::directedS,
        boost::property<boost::vertex_name_t, std::string>,
        EProp> Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
typedef boost::graph_traits<Graph>::edge_descriptor Edge;


std::map<std::string, std::string> getFeatures(auto const& features) {
    std::map<std::string, std::string> features_;

    for (auto const& feature: features){
        features_[feature.type()] = feature.value();
    }
    return features_;
};

auto validate_edge(
        const expath::Edge_ConnectionType type,
        std::experimental::string_view code,
        std::experimental::string_view source,
        std::experimental::string_view destination,
        const long departure,
        const long duration) {
    if (type == expath::Edge_ConnectionType_UNDEFINED) {
        return std::make_pair(false, "Missing connection type information");
    }

    if (type == expath::Edge_ConnectionType_CUSTODY) {
        if (duration != 0) {
            return std::make_pair(false, "Custody connections can not have a transit duration");
        }

        if (departure != 0) {
            return std::make_pair(false, "Custody connections can not have a departure time");
        }
    }

    if ((type == expath::Edge_ConnectionType_PERSISTENT) and (duration == 0)) {
        return std::make_pair(false, "Persistant connections can not have zero duration");
    }

    if(code.empty()) {
        return std::make_pair(false, "Missing/Invalid connection identifier");
    }

    if(source.empty()) {
        return std::make_pair(false, "Missing/Invalid source identifier");
    }

    if(destination.empty()) {
        return std::make_pair(false, "Missing/Invalid destination identifier");
    }

    return std::make_pair(true, "");
}

class EPServicesImpl final : public expath::EPServices::Service {
private:
    Graph graph{};

public:

    grpc::Status
    AddEdge(
            grpc::ServerContext* context,
            const expath::Edge* edge,
            expath::Response* response
    ) noexcept override {
        auto is_valid = validate_edge(
                edge->type(),
                edge->code(),
                edge->source(),
                edge->destination(),
                edge->departure(),
                edge->duration());

        std::map<std::string, std::string> feats = getFeatures(edge->features());

        if (is_valid.first) {
            if (edge->type() == expath::Edge_ConnectionType_CUSTODY) {
                add_connection<Graph, EProp, Features>(
                        edge->code(),
                        edge->source(),
                        edge->destination(),
                        long(edge->aggregate()),
                        long(edge->outbound()),
                        long(edge->inbound()),
                        edge->cost(),
                        feats,
                        graph);
            }
            else {
                add_connection<Graph, EProp, Features>(
                        edge->code(),
                        edge->source(),
                        edge->destination(),
                        long(edge->departure()),
                        long(edge->duration()),
                        long(edge->aggregate()),
                        long(edge->outbound()),
                        long(edge->inbound()),
                        edge->cost(),
                        feats,
                        graph);

            }
            response->set_success(true);
            response->set_message("CREATED");
        }
        else {
            response->set_success(false);
            response->set_message(is_valid.second);
        }
        return grpc::Status::OK;
    }

    grpc::Status
    RemoveEdge(
            grpc::ServerContext* context,
            const expath::Edge* edge,
            expath::Response* response
    ) override  {
        if (edge->code().empty()) {
            response->set_success(false);
            response->set_message("Invalid/Missing connection identifier.");
        }
        remove_connection(edge->code(), graph);
        response->set_success(true);
        response->set_message("");
        return grpc::Status::OK;
    }

    grpc::Status
    SearchEdge(
            grpc::ServerContext* context,
            const expath::Edge* edge,
            expath::FindResponse* response
    ) override {
        if (edge->code().empty()) {
            response->mutable_response()->set_success(false);
            response->mutable_response()->set_message("Invalid/Missing connection identifier");
        }
        else {
            Edge ed;
            bool ed_exists;
            std::tie(ed, ed_exists) = get_edge_by_code(edge->code(), graph);
            EProp ep = graph[ed];
            expath::Edge_ConnectionType type;

            if (ep.percon) {
                type = expath::Edge_ConnectionType_CUSTODY;
            }
            else {
                type = expath::Edge_ConnectionType_PERSISTENT;
            }

            response->mutable_edge()->set_code(ep.code);
            response->mutable_edge()->set_source(
                    get(
                            get(boost::vertex_name, graph),
                            boost::source(ed, graph)
                    )
            );
            response->mutable_edge()->set_destination(
                    get(
                            get(boost::vertex_name, graph),
                            boost::target(ed, graph)
                    )
            );
            response->mutable_edge()->set_aggregate(ep._tap);
            response->mutable_edge()->set_outbound(ep._top);
            response->mutable_edge()->set_inbound(ep._tip);
            response->mutable_edge()->set_departure(ep._dep);
            response->mutable_edge()->set_duration(ep._dur);
            response->mutable_edge()->set_type(type);
            response->mutable_response()->set_success(true);
            response->mutable_response()->set_message("");
        }
        return grpc::Status::OK;
    }
};

void RunServer() {
    std::string address{"0.0.0.0:8001"};
    EPServicesImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << address << std::endl;
    server->Wait();
}

#endif //FLETCHER_EXPATH_SERVER_HXX
