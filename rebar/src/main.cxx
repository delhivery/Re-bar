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

int main() {
    assert(sizeof(char) * CHAR_BIT == 8);
    asio::io_service io_service;
    asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), 9000);

    Weld<BaseGraph> welder;
    welder.add_solver(make_shared<Pareto>());
    welder.add_solver(make_shared<Optimal>(true));

    Server server{io_service, endpoint, welder};

    io_service.run();
    return 0;
}
