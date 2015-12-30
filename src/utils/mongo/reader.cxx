#include <map>
#include <sstream>

#include <bsoncxx/json.hpp>

#include <mongo/reader.hpp>

const std::vector<std::string> MongoReader::split(const std::string& s, const char& c) {
    std::string buff{""};
    std::vector<std::string> v;

    for (auto const& n: s) {
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
            case bsoncxx::type::k_oid:
                ss << ele.get_value().get_oid().value.to_string();
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

    for (auto const& doc : cursor) {
        std::map<std::string, std::string> value;

        for (auto const& field: fields) {
            value[field] = MongoReader::fetch_field(doc, MongoReader::split(field, '.'));
        }
        values.push_back(value);
    }

    return values;
}
