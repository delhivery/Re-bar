#include <cstdlib>
#include <iostream>
#include <thread>
#include <utility>

#include <boost/asio.hpp>

using boost::asio::ip::tcp;

const int max_length = 1024;

void session(tcp::socket sock) {
    try {
        while (true) {
            char data[max_length];

            boost::system::error_code error;
            size_t length = sock.read_some(boost::asio::buffer(data), error);

            if (error == boost::asio::error::eof) {
                break;
            }
            else if (error) {
                throw boost::system::system_error(error);
            }

            boost::asio::write(sock, boost::asio::buffer(data, length));
        }
    }
    catch (std::exception const& exc) {
        std::cerr << "Exception in thread: " << exc.what() << std::endl;
    }
}

void server(boost::asio::io_service& io_service, unsigned short port) {
    tcp::acceptor accept(io_service, tcp::endpoint(tcp::v4(), port));

    while (true) {
        tcp::socket sock(io_service);
        accept.accept(sock);
        std::thread(session, std::move(sock)).detach();
    }
}

int main(int argc, char* argv[]) {
    try {

        if (argc != 2) {
            std::cerr << "Usage: server <port>" << std::endl;
            return 1;
        }

        boost::asio::io_service io_service;
        server(io_service, std::atoi(argv[1]));
    }
    catch (std::exception const& exc) {
        std::cerr << "Exception: " << exc.what() << std::endl;
    }
}
