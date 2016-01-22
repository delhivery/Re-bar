#include <cassert>
#include <climits>

#include "optimal.hpp"
#include "pareto.hpp"
#include "jezik.hpp"
#include "weld.hpp"

/*template <typename T> map<string, function<json_map(shared_ptr<T>, const map<string, any>&)> > Weld<T>::welder = {
    {"ADDV", T::addv},
    {"ADDE", T::adde},
    {"LOOK", T::look},
    {"FIND", T::find}
};*/

const string_view DEFAULT_HOST{"127.0.0.1"};
const short int DEFAULT_PORT = 9000;

int main(int argc, char* argv[]) {
    string host{DEFAULT_HOST};
    short int port = DEFAULT_PORT;

    if (argc == 2) {
        port = atoi(argv[1]);
    } else

    if (argc == 3) {
        host = static_cast<string>(argv[1]);
        port = atoi(argv[2]);
    }

    assert(sizeof(char) * CHAR_BIT == 8);
    asio::io_service io_service;
    asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port);

    Weld<BaseGraph> welder;
    welder.add_solver(make_shared<Pareto>());
    welder.add_solver(make_shared<Optimal>(true));

    Server server{io_service, endpoint, welder};

    io_service.run();
    return 0;
}
