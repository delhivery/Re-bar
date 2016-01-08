#include <map>
#include <sstream>

#include <bsoncxx/json.hpp>

#include <mongo/reader.hpp>

std::vector<std::map<std::string, std::experimental::any> > MongoReader::query(
        std::string collection, bsoncxx::builder::stream::document& filter, mongocxx::options::find options) {
    std::vector<std::map<std::string, std::experimental::any> > data;

    auto mongo_client = mongocxx::client{mongo_uri};
    auto db = mongo_client[database];
    auto coll = db[collection];

    for (auto const& doc: coll.find(filter.view(), options)) {
        std::map<std::string, std::experimental::any> record;

        for (auto const& ele: doc) {
            auto key = ele.key().to_string();

            switch(ele.type()) {
                case bsoncxx::type::k_double:
                    record[key] = ele.get_double().value;
                    break;
                case bsoncxx::type::k_utf8:
                    record[key] = ele.get_utf8().value.to_string();
                    break;
                case bsoncxx::type::k_oid:
                    record[key] = ele.get_oid().value.to_string();
                    break;
                case bsoncxx::type::k_bool:
                    record[key] = ele.get_bool().value;
                    break;
                case bsoncxx::type::k_int32:
                    record[key] = ele.get_int32().value;
                    break;
                case bsoncxx::type::k_int64:
                    record[key] = ele.get_int64().value;
                    break;
                case bsoncxx::type::k_timestamp:
                    record[key] = ele.get_timestamp().timestamp;
                    break;
                case bsoncxx::type::k_date:
                    record[key] = ele.get_date().value;
                    break;
                case bsoncxx::type::k_dbpointer:
                    record[key] = ele.get_dbpointer().value.to_string();
                    break;
                default:
                    continue;
            }
        }
        data.push_back(record);
    }
    return data;
}
