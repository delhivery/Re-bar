#ifndef MONGOCLIENT_DEFINED
#define MONGOCLIENT_DEFINED

#include <experimental/string_view>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/uri.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>


class MongoClient {
    private:
        std::string database;
        std::vector<std::string> hosts;
        std::string username;
        std::string password;
        std::string auth_database;
        std::string replica_set;

        mongocxx::uri mongo_uri;
        mongocxx::instance mongo_instance;
        mongocxx::client mongo_client;
        bsoncxx::builder::stream::document document;

    public:
        MongoClient(std::string database, std::vector<std::string> hosts, std::string username, std::string password, std::string auth_database, std::string replica_set="");
        MongoClient(std::string database, std::vector<std::string> hosts, std::string username, std::string password, std::string replica_set="");
        MongoClient(std::string database, std::vector<std::string> hosts, std::string replica_set="");
        MongoClient(std::string database, std::string replica_set="");

        void init();

        std::vector<std::map<std::string, std::string> > query(
            std::string collection, bsoncxx::builder::stream::document& filter, std::vector<std::string> fields);
        const std::vector<std::string> split(const std::string& s, const char& c);
        std::string fetch_field(bsoncxx::document::view view, std::vector<std::string> field);
};

#endif
