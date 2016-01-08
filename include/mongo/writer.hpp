#ifndef WRITER_HPP_INCLUDED
#define WRITER_HPP_INCLUDED

#include <experimental/any>

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


        void write(std::string collection, std::map<std::string, std::string> record) {
            auto mongos_client = mongocxx::client{mongo_uri};
            auto db = mongos_client[database];
            auto coll = db[collection];

            bsoncxx::builder::stream::document doc_builder;

            for (auto const& element: record) {
                doc_builder << element.first << element.second;
            }

            coll.insert_one(doc_builder.view());
        }

        template <typename T> void write(std::string collection, T& iterable) {
            auto mongos_client = mongocxx::client{mongo_uri};
            auto db = mongos_client[database];
            auto coll = db[collection];

            mongocxx::bulk_write bulk_writer(mongocxx::options::bulk_write{});

            for (auto const& element: iterable) {
                // filter and update params
                bsoncxx::builder::stream::document filter_builder, update_builder;
                update_builder << "$set" << bsoncxx::builder::stream::open_document;

                auto data = element.to_store();

                for(auto const& element: data) {

                    if(element.first == "_id") {
                        std::cout << "Casting oid" << std::endl;
                        filter_builder << element.first << std::experimental::any_cast<bsoncxx::oid>(element.second);
                        std::cout << "Suckcess oid" << std::endl;
                    }

                    if(element.second.type() == typeid(int)) {
                        std::cout << "Casting int" << std::endl;
                        update_builder << element.first << std::experimental::any_cast<int>(element.second);
                        std::cout << "Suckcess int" << std::endl;
                    }
                    else if(element.second.type() == typeid(double)) {
                        std::cout << "Casting double" << std::endl;
                        update_builder << element.first << std::experimental::any_cast<double>(element.second);
                        std::cout << "Suckcess double" << std::endl;
                    }
                    else if(element.second.type() == typeid(long)) {
                        std::cout << "Casting long" << std::endl;
                        update_builder << element.first << std::experimental::any_cast<long>(element.second);
                        std::cout << "Suckcess long" << std::endl;
                    }
                    else if(element.second.type() == typeid(bsoncxx::oid)) {
                        std::cout << "Casting oid" << std::endl;
                        update_builder << element.first << std::experimental::any_cast<bsoncxx::oid>(element.second);
                        std::cout << "Suckcess oid" << std::endl;
                    }
                    else if(element.second.type() == typeid(bool)) {
                        std::cout << "Casting bool" << std::endl;
                        update_builder << element.first << std::experimental::any_cast<bool>(element.second);
                        std::cout << "Suckcess bool" << std::endl;
                    }
                    else if(element.second.type() == typeid(time_t)) {
                        update_builder << element.first << bsoncxx::types::b_date{std::experimental::any_cast<time_t>(element.second)};
                    }
                    else {
                        update_builder << element.first << std::experimental::any_cast<std::string>(element.second);
                    }
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
