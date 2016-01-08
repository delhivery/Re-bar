#ifndef READER_HPP_INCLUDED
#define READER_HPP_INCLUDED

#include <mongo/base.hpp>

class MongoReader : public Mongo {
    public:

        MongoReader(
            std::string database, std::vector<std::string> hosts, std::string username,
            std::string password, std::string auth_database, std::string replica_set=""
        ) : Mongo(
            database, hosts, username,
            password, auth_database, replica_set
        ) {}

        MongoReader(
            std::string database, std::vector<std::string> hosts, std::string username,
            std::string password, std::string replica_set=""
        ) : Mongo(
            database, hosts, username,
            password, "", replica_set
        ) {}

        MongoReader(
            std::string database, std::vector<std::string> hosts, std::string replica_set=""
        ) : Mongo(
            database, hosts, "",
            "", "", replica_set
        ) {}

        MongoReader(
            std::string database, std::string replica_set=""
        ) : Mongo(
            database, std::vector<std::string>{"127.0.0.1"}, "",
            "", "", replica_set
        ) {}

        std::vector<std::map<std::string, std::experimental::any> > query(
            std::string collection, bsoncxx::builder::stream::document & filter, mongocxx::options::find options=mongocxx::options::find{});
};
#endif
