#include <jezik.hpp>

#include <cstring>
#include <utility>

class SocketClosedException : public exception {};

Jezik::Jezik (shared_ptr<tcp::socket> _socket_ptr) : socket_ptr(_socket_ptr) {}

void Jezik::read(unsigned char* buffer) {
    asio::error_code error;
    socket_ptr->read_some(asio::buffer(buffer, sizeof(unsigned char)), error);

    if (error == asio::error::eof) {
        throw SocketClosedException();
    }
    else if (error) {
        throw asio::system_error(error);
    }
}

void Jezik::read(char* buffer, size_t length) {
    if (length == 0) {
        length = sizeof(buffer);
    }
    try {
        asio::error_code error;
        socket_ptr->read_some(asio::buffer(buffer, length), error);

        if (error == asio::error::eof) {
            throw SocketClosedException();
        }
        else if (error) {
            throw asio::system_error(error);
        }
    }
    catch (const exception& exc) {
        cerr << "Error occurred while attempting to read from socket. " << exc.what() << endl;
        throw;
    }
}

void Jezik::do_write(string_view response) {
    socket_ptr->write_some(asio::buffer(to_buffer<unsigned char, 4>(response.length()), 4));
    socket_ptr->write_some(asio::buffer(response.to_string()));
}

ArgumentToken::ArgumentToken(shared_ptr<tcp::socket> socket_ptr) : Jezik(socket_ptr) {}

void ArgumentToken::do_read_body_length() {
    read(body_length);
}

void ArgumentToken::do_read_body() {
    read(data, body_length[0]);
}

void ArgumentToken::do_read() {
    do_read_body_length();
    assert(body_length[0] > 0);
    do_read_body();
}

string_view ArgumentToken::value() const {
    return string_view(data, int(body_length[0]));
}

Argument::Argument(shared_ptr<tcp::socket> socket_ptr) : Jezik(socket_ptr) {}

void Argument::do_read() {
    do_read_argument_type();
    argn.do_read();
    argv.do_read();

    string argt{argument_type, 3};

    string t_value = argv.value().to_string();

    if(argt.compare("INT") == 0) {
        data.first = argn.value();
        data.second = stol(t_value);
    } else

    if(argt.compare("STR") == 0) {
        data.first = argn.value();
        data.second = t_value;
    } else

    if(argt.compare("DBL") == 0) {
        data.first = argn.value();
        data.second = stod(t_value);
    } else {
        throw invalid_argument("Unsupported type for argument " + argt);
    }
}

void Argument::do_read_argument_type() {
    read(argument_type, sizeof(argument_type));
}

pair<string_view, any> Argument::value() const {
    return data;
}

Command::Command(shared_ptr<tcp::socket> socket_ptr) : Jezik(socket_ptr) {}

void Command::do_read() {
    read(mode);
    read(command, sizeof(command));
    read(nargs);

    for (size_t i = 0; i < nargs[0]; i++) {
        Argument arg{socket_ptr};
        arg.do_read();
        kwargs[arg.value().first.to_string()] = arg.value().second;
    }
}

void Command::start(function<json_map(int, string_view, const map<string, any>&) > handler) {
    try {
        while(true) {
            do_read();
            string response = handler(mode[0], string_view(command, 4), kwargs).to_string();
            do_write(response);
        }
    }
    catch (const SocketClosedException& exc) {
        socket_ptr->close();
        return;
    }
    catch (const exception& exc) {
        cerr << "Exception occurred while parsing/executing command: " << exc.what() << endl;
    }
}

Server::Server(asio::io_service& io_service, tcp::endpoint& endpoint, function<json_map(int, string_view, const map<string, any>&)> _handler) : Jezik(make_shared<tcp::socket>(ref(io_service))), acceptor(io_service, endpoint) , handler(_handler) {
    try {
        do_read();
    }
    catch (const exception& exc) {
        cerr << "Exception occurred while listening to socket: " << exc.what() << endl;
        throw;
    }
}

void Server::do_read() {
    acceptor.async_accept(*socket_ptr, [this](const asio::error_code ec) {
        if (!ec)
            make_shared<Command>(socket_ptr)->start(handler);
        do_read();
    });
}
