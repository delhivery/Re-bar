#ifndef BASE_HPP_INCLUDED
#define BASE_HPP_INCLUDED

#include <experimental/any>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>


class Mongo {
    protected:
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
        Mongo(std::string database, std::vector<std::string> hosts, std::string username, std::string password, std::string auth_database, std::string replica_set="");

        void init();
};
#endif
