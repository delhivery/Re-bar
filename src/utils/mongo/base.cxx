#include <sstream>
#include <mongo/base.hpp>

Mongo::Mongo(std::string database, std::vector<std::string> hosts, std::string username, std::string password, std::string auth_database, std::string replica_set) : database(database), hosts(hosts), username(username), password(password), auth_database(auth_database), replica_set(replica_set)  {}


void Mongo::init() {
    mongo_instance = mongocxx::instance{};

    std::ostringstream mongo_uri_ss;
    mongo_uri_ss << "mongodb://";

    if (username != "" && password != "") {
        mongo_uri_ss << username << ":" << password;
    }

    for (auto it = hosts.begin(); it != hosts.end(); ++it) {
        mongo_uri_ss << *it;
    }

    mongo_uri_ss << "/" << database;

    if (replica_set != "" or (username != "" and password != "")) {
        mongo_uri_ss << "?";
    }

    if (replica_set != "") {
        mongo_uri_ss << "replicaSet=" << replica_set;
    }

    if (username != "" and password != "") {
        if (replica_set != "")
            mongo_uri_ss << "&";
        mongo_uri_ss << "authenticationDatabase=" << auth_database;
    }

    mongo_uri = mongocxx::uri(mongo_uri_ss.str());
}
