#include <mongo/writer.hpp>


template <typename Iterator> void MongoWriter::write(std::string collection, Iterator& iterator, std::vector<std::string> fields, std::string p_key) {
    auto mongos_client = mongocxx::client{mongo_uri};
    auto db = mongos_client[database];
    auto coll = db[collection];

    // Unordered bulk write
    mongocxx::bulk_write bulk_writer(false);

    for (auto element: iterator) {
        // filter and update params
        bsoncxx::builder::stream::document filter, object;
        filter << p_key << element.getattr(p_key);

        for (auto field: fields) {
            object << field << element.getattr(field);
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
