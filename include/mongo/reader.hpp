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

        const std::vector<std::string> split(const std::string& s, const char& c);

        std::string fetch_field(bsoncxx::document::view view, std::vector<std::string> field);

        std::vector<std::map<std::string, std::string> > query(
            std::string collection,
            bsoncxx::builder::stream::document& filter,
            std::vector<std::string> fields,
            mongocxx::options::find options = mongocxx::options::find{}
        );
};
#endif
