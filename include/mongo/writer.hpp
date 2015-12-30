#ifndef WRITER_HPP_INCLUDED
#define WRITER_HPP_INCLUDED

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

        template <typename T> void write(std::string collection, T& iterable, std::vector<std::string> fields, std::string p_key) {
            auto mongos_client = mongocxx::client{mongo_uri};
            auto db = mongos_client[database];
            auto coll = db[collection];

            // Unordered bulk write
            mongocxx::bulk_write bulk_writer(false);

            for (auto const& element: iterable) {
                // filter and update params
                bsoncxx::builder::stream::document filter, object;

                auto pkey_val = element.getattr(p_key);

                if (int* value = boost::get<int>(&pkey_val))
                    filter << p_key << *value;

                else if (double* value = boost::get<double>(&pkey_val))
                    filter << p_key << *value;

                else if (std::string* value = boost::get<std::string>(&pkey_val))
                    filter << p_key << *value;

                else if (bsoncxx::oid* value = boost::get<bsoncxx::oid>(&pkey_val))
                    filter << p_key << *value;

                for (auto const& key: fields) {
                    auto val = element.getattr(key);

                    if (int* value = boost::get<int>(&val))
                        object << key << *value;

                    if (double* value = boost::get<double>(&val))
                        object << key << *value;

                    if (std::string* value = boost::get<std::string>(&val))
                        object << key << *value;

                    if (bsoncxx::oid* value = boost::get<bsoncxx::oid>(&val))
                        object << key << *value;
                }

                // Allow upserts
                mongocxx::model::update_one write_document{filter.view(), object.view()};
                write_document.upsert(true);

                bulk_writer.append(mongocxx::model::write{write_document});
            }
            auto results = coll.bulk_write(bulk_writer).optional::value();
            std::cout << "Write complete." << std::endl;
            std::cout << "Inserted: <" << results.inserted_count() << "> ";
            std::cout << "Matched: <" << results.matched_count() << "> ";
            std::cout << "Modified: <" << results.modified_count() << "> ";
            std::cout << "Deleted: <" << results.deleted_count() << "> ";
            std::cout << "Upserted: <" << results.upserted_count() << ">" << std::endl;
        }
};
#endif
