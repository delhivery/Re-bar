#include <iostream>
#include <marge.hpp>
#include <rebar.hpp>
#include <consumer.cxx>

#include <shared_queue.hpp>

#include <jeayeson/jeayeson.hpp>

#include <utils.hpp>

using namespace std;

std::string json_to_str(jeayeson::value val) {
    if ( val.is( jeayeson::value::type::null ) ) {
        return "";
    }
    return val.as<std::string>();
}

void getsize(Queue<std::string>& queue) {
    while (true) {
        std::cout << "Queue size: " << queue.size() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void process(std::shared_ptr<Solver> solver_ptr, std::shared_ptr<Solver> fallback_ptr, Queue<std::string>& queue) {
    try { 
        while(true) {
            std::string waybill, location, destination, connection, action, ps, pid;
            double scan_dt, promise_dt;
            bool attempt = false;

            try {
                std::string::size_type pos_braces;

                auto doc = jeayeson::map_t{jeayeson::data{queue.pop()}};
                std::map<std::string, std::string> datum;

                waybill = json_to_str(doc["wbn"]);

                location = json_to_str(doc["cs"]["sl"]);
                pos_braces = location.find(" (");
                location = (pos_braces == std::string::npos) ? location : location.substr(0, pos_braces);
                
                destination = json_to_str(doc["cn"]);
                pos_braces = destination.find(" (");
                destination = (pos_braces == std::string::npos) ? destination : destination.substr(0, pos_braces);

                connection = json_to_str(doc["cs"]["cid"]);

                action = json_to_str(doc["cs"]["act"]);

                ps = json_to_str(doc["cs"]["ps"]);

                pid = json_to_str(doc["cs"]["pid"]);

                if (action == "+L") {
                    action = "OUTSCAN";
                }
                else if (action == "<L" and ps == pid) {
                    action = "INSCAN";
                }
                else {
                    action = "LOCATION";
                }

                scan_dt = time_from_string(json_to_str(doc["cs"]["sd"]));
                promise_dt = time_from_string(json_to_str(doc["pdd"]));

                if (location != "" and destination != "" and destination != "NSZ") {
                    attempt = true;
                    datum["wbn"] = waybill;
                    datum["loc"] = location;
                    datum["dst"] = destination;
                    datum["con"] = connection;
                    datum["action"] = action;
                    datum["scan_dt"] = json_to_str(doc["cs"]["sd"]);
                    datum["promise_dt"] = json_to_str(doc["pdd"]);

                    MongoWriter mw{"rebar"};
                    mw.init();
                    mw.write("scans", datum);
                }
            }
            catch (std::exception const& exc) {
                std::cout << "Exception occured in transforming kinesis data" << std::endl;
            }

            if (attempt) {
                auto parser = ParserGraph{waybill, promise_dt, solver_ptr, fallback_ptr};
                try {
                    parser.parse_scan(location, destination, connection, Actions::_from_string(action.c_str()), scan_dt);
                }
                catch (std::exception const& exc) {
                    parser.save(false);
                    std::cout << "Exception occurred at parser: " << exc.what() << std::endl;
                }
            }
        }
    }
    catch (std::exception const& exc) {
        std::cout << "Exception occurred at parser: " << exc.what() << std::endl;
    }
}

int main() {
    Queue<std::string> shared_queue;
    std::shared_ptr<Solver> solver_ptr = std::make_shared<Solver>();
    std::shared_ptr<Solver> fallback_ptr = std::make_shared<Solver>("rebar", 2);
    solver_ptr->init();
    fallback_ptr->init();

    Consumer<Queue<std::string> > consumer{"Package.info", shared_queue, Aws::Region::US_EAST_1};

    std::cout << "Starting consumer" << std::endl;
    std::thread threaded(&Consumer<Queue<std::string> >::get_shards, consumer);
    threaded.detach();

    std::cout << "Starting parser" << std::endl;
    // process(solver_ptr, fallback_ptr, std::ref(shared_queue));
    unsigned int parser_count = std::thread::hardware_concurrency() * 4;
    parser_count = 1; //std::thread::hardware_concurrency() * 4;
    std::vector<std::thread> parsers;

    for (unsigned int idx = 0; idx < parser_count; idx++) {
        parsers.push_back(std::thread{std::bind(&process, solver_ptr, fallback_ptr, std::ref(shared_queue))});
    }

    for (unsigned int idx = 0; idx < parser_count; idx++) {
        parsers[idx].join();
    }
    
    return 0;
}
