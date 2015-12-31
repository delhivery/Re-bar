#include <iostream>
#include <marge.hpp>
#include <rebar.hpp>

using namespace std;

int main() {
    std::shared_ptr<Solver> solver_ptr = std::make_shared<Solver>();
    solver_ptr->init();
    try {
        while(true) {
            string waybill, location, destination, connection, action;
            double scan_dt, promise_dt;
            std::cout << "Waybill: ";
            std::cin >> waybill;
            std::cout << "Location: ";
            std::cin >> location;
            std::cout << "Destination: ";
            std::cin >> destination;
            std::cout << "Connection: ";
            std::cin >> connection;
            std::cout << "Action: ";
            std::cin >> action;
            std::cout << "Scan date: ";
            std::cin >> scan_dt;
            std::cout << "Promise date: ";
            std::cin >> promise_dt;
            std::cout << std::endl;

            auto parser = ParserGraph{waybill, solver_ptr};
            try {
                parser.parse_scan(location, destination, connection, Actions::_from_string(action.c_str()), scan_dt, promise_dt);
                // parser.show();
            }
            catch (std::exception const& exc) {
                parser.save(false);
                std::cout << "Exception occurred: " << exc.what() << std::endl;
                throw exc;
            }
        };
    }
    catch (std::exception const& exc) {
        std::cout << "Exception occurred: " << exc.what() << std::endl;
    }
    return 0;
}
