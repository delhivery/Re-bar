#include <iostream>
#include <marge.hpp>
#include <rebar.hpp>

using namespace std;

int main() {
    std::shared_ptr<Solver> solver_ptr = std::make_shared<Solver>();
    solver_ptr->init();

    while(true) {
        string waybill, location, destination, connection, action;
        double scan_dt, promise_dt;
        std::cout << "Waybill: ";
        cin >> waybill;
        std::cout << "Location: ";
        cin >> location;
        std::cout << "Destination: ";
        cin >> destination;
        std::cout << "Connection: ";
        cin >> connection;
        std::cout << "Action: ";
        cin >> action;
        cout << "Scan date: ";
        cin >> scan_dt;
        cout << "Promise date: ";
        cin >> promise_dt;

        auto parser = ParserGraph{waybill, solver_ptr};
        parser.parse_scan(location, destination, connection, Actions::_from_string(action.c_str()), scan_dt, promise_dt);
    };
    return 0;
}
