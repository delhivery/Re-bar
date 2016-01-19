#include <jezik/jezik.hpp>

#include <cstring>
#include <iostream>

void Jezik::read(unsigned char* buffer) {
    socket_ptr->read_some(asio::buffer(buffer, sizeof(unsigned char)));
}

void Jezik::read(char* buffer, size_t length) {
    if (length == 0) {
        length = sizeof(buffer);
    }
    try {
        socket_ptr->read_some(asio::buffer(buffer, length));
    }
    catch (const exception& exc) {
        cerr << "Error occurred while attempting to read from socket. " << exc.what() << endl;
        throw exc;
    }
}

void Jezik::do_write(string response) {
    response = to_string(response.length()) + response;
    socket_ptr->write_some(asio::buffer(response, response.length()));
    socket_ptr->close();
}

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

string ArgumentToken::value() const {
    return string(data, int(body_length[0]));
}

void Argument::do_read() {
    do_read_argument_type();
    argn.do_read();
    argv.do_read();
}

void Argument::do_read_argument_type() {
    read(argument_type, sizeof(argument_type));
}

map<string, any> Argument::value() const {
    map<string, any> dict;
    string key = argn.value();
    string argt{argument_type, 3};

    if(argt.compare("INT") == 0) {
        dict[key] = stoi(argv.value());
    } else

    if(argt.compare("STR") == 0) {
        dict[key] = argv.value();
    } else

    if(argt.compare("DBL") == 0) {
        dict[key] = stod(argv.value());
    }
    return dict;
}

void Command::do_read() {
    read(mode);
    read(command, sizeof(command));
    read(nargs);

    for (size_t i = 0; i < nargs[0]; i++) {
        Argument arg{socket_ptr};
        arg.do_read();

        for (const auto& value: arg.value()) {
            kwargs[value.first] = value.second;
        }
    }
}

void Command::start(function<json_map(int, string_view, const map<string, any>&) > handler) {
    do_read();
    string response = handler(mode[0], string_view(command, 3), kwargs).to_string();
    do_write(response);
}

Server::Server(asio::io_service& io_service, tcp::endpoint& endpoint, function<json_map(int, string_view, const map<string, any>&)> _handler) : Jezik(make_shared<tcp::socket>(ref(io_service))), acceptor(io_service, endpoint) , handler(_handler) {
    try {
        do_read();
    }
    catch (const exception& exc) {
        cerr << "Exception occurred while listening to socket: " << exc.what() << endl;
        throw exc;
    }
}

void Server::do_read() {
    acceptor.async_accept(*socket_ptr, [this](error_code ec) {
        if (!ec)
            make_shared<Command>(socket_ptr)->start(handler);
        do_read();
    });
}
