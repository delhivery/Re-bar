#include <memory>
#include <experimental/any>
#include <experimental/string_view>

#include <asio.hpp>

#include <jeayeson/jeayeson.hpp>

using namespace std;
using experimental::string_view;
using experimental::any;
using asio::ip::tcp;


class Jezik {
    protected:
        shared_ptr<tcp::socket> socket_ptr;

        void read(unsigned char* buffer);
        void read(char* buffer, size_t length=0);

    public:
        Jezik (shared_ptr<tcp::socket> _socket_ptr) : socket_ptr(_socket_ptr) {}
        virtual void do_read() = 0;
        void do_write(string response);
};

class ArgumentToken : public Jezik {
    private:
        char data[256];
        unsigned char body_length[1];

        void do_read_body_length(); 
        void do_read_body();

    public:
        ArgumentToken(shared_ptr<tcp::socket> socket_ptr) : Jezik(socket_ptr) {}
        void do_read();
        string value() const;
};

class Argument : public Jezik {
    private:
        char argument_type[3];
        ArgumentToken argn{socket_ptr}, argv{socket_ptr};
        map<string, any> data;

        void do_read_argument_type();
        void do_read_argument_token();
        string argt();

    public:
        Argument(shared_ptr<tcp::socket> socket_ptr) : Jezik(socket_ptr) {}
        void do_read();
        map<string, any> value() const;
};

class Command : public Jezik, public enable_shared_from_this<Command>  {
    private:
        unsigned char mode[1];
        unsigned char nargs[1];
        char command[3];
        map<string, any> kwargs;

        void run_command();
        string cmd();

    public:
        Command(shared_ptr<tcp::socket> socket_ptr) : Jezik(socket_ptr) {}
        void do_read();
        void start(function<json_map(int, string_view, const map<string, any>&) >);
};

class Server : public Jezik {
    private:
        tcp::acceptor acceptor;
        function<json_map(int, string_view, const map<string, any>&) > handler;

    public:
        void do_read();
        Server(asio::io_service&, tcp::endpoint&, function<json_map(int, string_view, const map<string, any>&) >);
};
