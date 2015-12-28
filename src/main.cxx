#include <iostream>
#include <iomanip>
#include <string>

#include <marge.hpp>
#include <utils.hpp>

using namespace std;

int main() {
    string origin, destination;
    double departure, promise_dt;

    cin >> origin >> destination >> departure >> promise_dt;
    Solver solver;
    solver.init();

    cout << "Center,";
    cout << "Cost,";
    cout << "Arrival,";
    cout << "Connection,";
    cout << "Departure,";
    cout << "Inbound By,";
    cout << "Inbound Time,";
    cout << "Bag By,",
    cout << "Bagging Time,";
    cout << "Start Outbound By,";
    cout << "Outbound duration," << std::endl;

    try {
        double departure_start = departure - get_time(departure);
        double arrival = get_time(departure);
        auto center = origin;
        double cost = 0;
        double inbound = 0;

        for(auto solution: solver.solve(origin, destination, get_time(departure), promise_dt)) {
            auto connection = solution.connection;
            double d_time = departure_start + arrival + connection.wait_time(arrival) + connection._t_agg_proc + connection._t_out_proc;

            cout << std::setiosflags(ios::fixed) << std::setprecision(0);
            cout << center << ",";
            cout << cost << ",";
            cout << departure_start + arrival << ",";
            cout << solution.connection.name << ",";
            cout << d_time << ",";
            cout << departure_start + arrival + inbound << ",";
            cout << inbound << ",";
            cout << d_time - connection._t_out_proc << ",";
            cout << connection._t_agg_proc << ",";
            cout << d_time << ",";
            cout << connection._t_out_proc << std::endl;
            center = solution.destination;
            cost = solution.cost;
            arrival = solution.arrival;
            inbound = connection._t_inb_proc;
        }

        cout << center << ",";
        cout << cost << ",";
        cout << departure_start + arrival << ",";
        cout << "" << ",";
        cout << "" << ",";
        cout << departure_start + arrival + inbound << ",";
        cout << "" << ",";
        cout << "" << ",";
        cout << "" << ",";
        cout << "" << ",";
        cout << "" << "," << std::endl;
    }
    catch (std::exception& e) {
        cout << e.what() << endl;
    }

    cout << std::endl;

    return 0;
}
