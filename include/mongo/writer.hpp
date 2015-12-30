#ifndef WRITER_HPP_INCLUDED
#define WRITER_HPP_INCLUDED

#include <bsoncxx/json.hpp>

#include <mongo/base.hpp>
#include <boost/variant.hpp>

class MongoWriter : public Mongo {
    public:
        MongoWriter(
            std::string database, std::vector<std::string> hosts, std::string username,
            std::string password, std::string auth_database, std::string replica_set=""
        ) : Mongo(
            database, hosts, username,
            password, auth_database, replica_set
        ) {}

        MongoWriter(
            std::string database, std::vector<std::string> hosts, std::string username,
            std::string password, std::string replica_set=""
        ) : Mongo(
            database, hosts, username,
            password, "", replica_set
        ) {}

        MongoWriter(
            std::string database, std::vector<std::string> hosts, std::string replica_set=""
        ) : Mongo(
            database, hosts, "",
            "", "", replica_set
        ) {}

        MongoWriter(
            std::string database, std::string replica_set=""
        ) : Mongo(
            database, std::vector<std::string>{"127.0.0.1"}, "",
            "", "", replica_set
        ) {}

        template <typename T> void write(
                std::string collection, T& iterable, std::vector<std::string> fields, std::string p_key,
                std::map<std::string, std::string> meta_data
        ) {
            auto mongos_client = mongocxx::client{mongo_uri};
            auto db = mongos_client[database];
            auto coll = db[collection];

            mongocxx::bulk_write bulk_writer(false);

            for (auto const& element: iterable) {
                // filter and update params
                bsoncxx::builder::stream::document filter_builder, update_builder;

                auto pkey_val = element.getattr(p_key);

                if (int* value = boost::get<int>(&pkey_val))
                    filter_builder << p_key << *value;

                else if (double* value = boost::get<double>(&pkey_val))
                    filter_builder << p_key << *value;

                else if (std::string* value = boost::get<std::string>(&pkey_val))
                    filter_builder << p_key << *value;

                else if (bsoncxx::oid* value = boost::get<bsoncxx::oid>(&pkey_val))
                    filter_builder << p_key << *value;

                update_builder << "$set" << bsoncxx::builder::stream::open_document ;

                for (auto const& key: fields) {
                    auto val = element.getattr(key);

                    if (int* value = boost::get<int>(&val))
                        update_builder << key << *value;

                    if (double* value = boost::get<double>(&val))
                        update_builder << key << *value;

                    if (std::string* value = boost::get<std::string>(&val))
                        update_builder << key << *value;

                    if (bsoncxx::oid* value = boost::get<bsoncxx::oid>(&val))
                        update_builder << key << *value;
                }

                for(auto item: meta_data) {
                    update_builder << item.first << item.second;
                }

                update_builder << bsoncxx::builder::stream::close_document;

                mongocxx::model::update_one write_document{filter_builder.view(), update_builder.view()};
                write_document.upsert(true);
                bulk_writer.append(write_document);
            }

            auto results = coll.bulk_write(bulk_writer).optional::value();
        }
};
#endif