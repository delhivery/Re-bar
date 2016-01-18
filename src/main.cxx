#include <cassert>
#include <climits>

#include <protocol.hpp>

int main() {
    assert(sizeof(char) * CHAR_BIT == 8);
    asio::io_service io_service;
    asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), 9000);
    Server server{io_service, endpoint};
    io_service.run();
    return 0;
}
