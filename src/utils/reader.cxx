#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <bsoncxx/json.hpp>

#include <reader.hpp>


MongoReader::MongoReader(std::string database, std::vector<std::string> hosts, std::string username, std::string password, std::string auth_database, std::string replica_set) : database(database), hosts(hosts), username(username), password(password), auth_database(auth_database), replica_set(replica_set)  {}

MongoReader::MongoReader(std::string database, std::vector<std::string> hosts, std::string username, std::string password, std::string replica_set) : database(database), hosts(hosts), username(username), password(password), auth_database(database), replica_set(replica_set)  {}

MongoReader::MongoReader(std::string database, std::vector<std::string> hosts, std::string replica_set) : database(database), hosts(hosts), username(""), password(""), auth_database(""), replica_set(replica_set)  {}

MongoReader::MongoReader(std::string database, std::string replica_set) : database(database), username(""), password(""), auth_database(""), replica_set(replica_set) {
    hosts.push_back("127.0.0.1");
}

void MongoReader::init() {
    mongo_instance = mongocxx::instance{};

    std::ostringstream mongo_uri_ss;
    mongo_uri_ss << "mongodb://";

    if (username != "" && password != "") {
        mongo_uri_ss << username << ":" << password;
    }

    for(auto it = hosts.begin(); it != hosts.end(); ++it) {
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

const std::vector<std::string> MongoReader::split(const std::string& s, const char& c) {
    std::string buff{""};
    std::vector<std::string> v;

    for(auto n: s) {
        if (n != c)
            buff += n;
        else if (n == c && buff != "") {
            v.push_back(buff);
            buff = "";
        }
    }

    if(buff != "") {
        v.push_back(buff);
    }

    return v;
}

std::string MongoReader::fetch_field(bsoncxx::document::view view, std::vector<std::string> fields) {
    std::string pfield = fields[0];
    std::ostringstream ss;
    bsoncxx::document::element ele{view[pfield]};

    if(ele) {
        if (fields.size() > 1 && ele.type() == bsoncxx::type::k_document) {
            fields.erase(fields.begin() + 0);
            return MongoReader::fetch_field(ele.get_document(), fields);
        }

        switch(ele.type()) {
            case bsoncxx::type::k_double:
                return std::to_string(ele.get_value().get_double().value);
                break;
            case bsoncxx::type::k_utf8:
                ss << ele.get_value().get_utf8().value;
                return ss.str();
            default:
                return bsoncxx::to_json(ele.get_value());
                break;
        }
    }
    else {
        return std::string{""};
    }
}

std::vector<std::map<std::string, std::string> > MongoReader::query(
        std::string collection,
        bsoncxx::builder::stream::document& filter,
        std::vector<std::string> fields,
        mongocxx::options::find options
) {
    std::vector<std::map<std::string, std::string> > values;
    auto mongos_client = mongocxx::client{mongo_uri};
    auto db = mongos_client[database];

    auto coll = db[collection];
    auto cursor = coll.find(filter.view(), options);

    for (auto&& doc : cursor) {
        std::map<std::string, std::string> value;

        for (auto field: fields) {
            value[field] = MongoReader::fetch_field(doc, MongoReader::split(field, '.'));
        }
        values.push_back(value);
    }

    return values;
}
