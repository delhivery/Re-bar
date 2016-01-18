#include <memory>
#include <experimental/any>

#include <asio.hpp>

using namespace std;
using experimental::any;
using asio::ip::tcp;

class Protocol {
    protected:
        shared_ptr<tcp::socket> socket_ptr;

        void read(unsigned char* buffer);
        void read(char* buffer, size_t length=0);

    public:
        Protocol(shared_ptr<tcp::socket> _socket_ptr) : socket_ptr(_socket_ptr) {}
        virtual void do_read() = 0;
};

class ArgumentToken : public Protocol {
    private:
        char data[256];
        unsigned char body_length[1];

        void do_read_body_length(); 
        void do_read_body();

    public:
        ArgumentToken(shared_ptr<tcp::socket> socket_ptr) : Protocol(socket_ptr) {}
        void do_read();
        string value() const;
};

class Argument : public Protocol {
    private:
        char argument_type[3];
        ArgumentToken argn{socket_ptr}, argv{socket_ptr};
        map<string, any> data;

        void do_read_argument_type();
        void do_read_argument_token();
        string argt();

    public:
        Argument(shared_ptr<tcp::socket> socket_ptr) : Protocol(socket_ptr) {}
        void do_read();
        map<string, any> value() const;
};

class Command : public Protocol, public enable_shared_from_this<Command>  {
    private:
        unsigned char nargs[1];
        char command[3];
        vector<Argument> arguments;

        void run_command();
        string cmd();

    public:
        Command(shared_ptr<tcp::socket> socket_ptr) : Protocol(socket_ptr) {}
        void do_read();
        void start();
};

class Server : public Protocol {
    private:
        tcp::acceptor acceptor;

    public:
        void do_read();

        Server(asio::io_service& io_service, tcp::endpoint& endpoint);
};
