#include <iostream>
#include <marge.hpp>
#include <rebar.hpp>

using namespace std;

int main() {
    Solver solver;
    solver.init();
    std::shared_ptr<Solver> solver_ptr = std::make_shared<Solver>(solver);

    while(true) {
        string waybill, location, destination, connection, action;
        double scan_dt, promise_dt;

        cin >> waybill >> location >> destination >> connection >> action >> scan_dt >> promise_dt;

        auto parser = ParserGraph{waybill, solver_ptr};
        parser.parse_scan(location, destination, connection, Actions::_from_string(action.c_str()), scan_dt, promise_dt);
    };
    return 0;
}
