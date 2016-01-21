#include <memory>
#include <experimental/any>
#include <experimental/string_view>

#include <asio.hpp>

#include <jeayeson/jeayeson.hpp>

using namespace std;
using experimental::string_view;
using experimental::any;
using asio::ip::tcp;

/**
 * Utility function to convert an unsigned integer to a buffer
 */
template <typename T, size_t N> array<T, N> to_buffer(unsigned int value) {
    array<T, N> buffer;
    size_t t_size = sizeof(T) * CHAR_BIT;

    for (int i = N; i > 0; i--) {
        buffer[N - i] = (value >> (t_size * (i - 1))) & 0xFF;
    }
    return buffer;
}

/**
 * A class to implement basic read write and structure for TCP Messaging.
 */
class Jezik {
    protected:
        /**
         * Pointer to a socket against which a connection has been made
         */
        shared_ptr<tcp::socket> socket_ptr;

        /**
         * Read data from socket to a buffer.
         * Specifically used to read a single byte of data representing integral values in the range of 0-255.
         * @param[in,out] buffer: An unsigned buffer of char to which data has to be read
         */
        void read(unsigned char* buffer);

        /**
         * Read data from socket to a buffer with optional size
         * @param[in,out] buffer: A buffer of char to which data has to be read
         * @param[in] length: An optional length of type size_t indicating the size of data to be read(defaults to length of entire buffer).
         */
        void read(char* buffer, size_t length=0);

    public:
        /*
         * Default constructs the protocol handler
         * @param[in] _socket_ptr: Shared pointer to a asio::tcp::socket across which a connection has been accepted
         */
        Jezik (shared_ptr<tcp::socket> _socket_ptr) : socket_ptr(_socket_ptr) {}

        /**
         * Pure virtual virtual responsible for appropriate calls to read() depending on buffers it needs to populate
         */
        virtual void do_read() = 0;

        /**
         * Writes a response to the tcp socket
         * @param[in] response: String response to be written to socket
         */
        void do_write(string_view response);
};

/**
 * Class representing an argument token. Each argument token consists of 2 parts
 * - A 1 byte header (unsigned char) specifying the size of the body
 * - A 1-256 length of body characters specifying the value of this token
 */
class ArgumentToken : public Jezik {
    private:
        char data[256];
        unsigned char body_length[1];

        /**
         * Reads the header of the message to find out body length
         */
        void do_read_body_length(); 

        /**
         * Reads the body of the message
         */
        void do_read_body();

    public:
        /**
         * Default constructs a message handler
         * @param[in] _socket_ptr: Shared pointer to asio::tcp::socket
         */
        ArgumentToken(shared_ptr<tcp::socket> socket_ptr) : Jezik(socket_ptr) {}

        /**
         * Implementation of virtual function to read argument token from socket
         */
        void do_read();

        /*
         * Returns the body of the message as read from the socket
         */
        string_view value() const;
};

/* Class to implement the Argument Messaging Protocol to represent a named argument i.e. arg_name = arg_value.
 * Each argument consists of a
 * - 3 byte keyword specifying the argument value type (INT, STR, DBL)
 * - An argument token as defined by ArgumentToken specifying the argument name
 * - An argument token as defined by ArgumentToken specifying the argument value
 */
class Argument : public Jezik {
    private:
        /**
         * A char buffer to hold the argument type
         */
        char argument_type[3];

        /**
         * Argument tokens to hold the argument name and argument value
         */
        ArgumentToken argn{socket_ptr}, argv{socket_ptr};

        /**
         * A map exposing the named argument as a key, value pair
         */
        pair<string_view, any> data;

        /**
         * Read the argument type from socket
         */
        void do_read_argument_type();

        /**
         * Read an argument token from the socket
         */
        void do_read_argument_token();

        /**
         * Returns the argument type as a string_view
         */
        string_view argt();

    public:
        /**
         * Default constructs an object representing a named argument
         */
        Argument(shared_ptr<tcp::socket> socket_ptr) : Jezik(socket_ptr) {}

        /**
         * Implementation of virtual function to read named argument from socket
         */
        void do_read();

        /**
         * Returns the named argument as a pair of key, value
         */
        pair<string_view, any> value() const;
};

/**
 * Class to implement a command in the messaging protocol over TCP
 * A command consists of the following components
 * - Mode (Integer[0-1])
 * - Nargs (Integer[0-255])
 * - Command (string{0-4})
 * - Arguments (Map of named arguments to be used against command)
 */
class Command : public Jezik, public enable_shared_from_this<Command>  {
    private:
        /**
         * Unsigned char buffer of size 1 to store mode
         */
        unsigned char mode[1];

        /**
         * Unsigned char buffer of size 1 to store number of arguments
         */
        unsigned char nargs[1];

        /**
         * Unsigned char buffer of size 4 to store command
         */
        char command[4];

        /**
         * A map holding named arguments as key: value
         */
        map<string, any> kwargs;

        /**
         * Execute a successfully read command
         */
        void run_command();

        /**
         * Returns the parsed command as a string
         */
        string_view cmd();

    public:

        /**
         * Default constructs an instance of command to read from an accepted tcp socket
         * @param[in] _socket_ptr: Shared pointer to asio::tcp::socket
         */
        Command(shared_ptr<tcp::socket> socket_ptr) : Jezik(socket_ptr) {}

        /**
         * Implementaion of the virtual function to read the command from the tcp socket
         */
        void do_read();

        /**
         * Wrapper function to make appropriate underlying calls to parse command from socket, execute it and write appropriate response to socket
         * @param[in] handler: A functor which takes the command as an input and executes it.
         */
        void start(function<json_map(int, string_view, const map<string, any>&) >);
};

/**
 * Class implementing the server responsible for accepting TCP connections.
 */
class Server : public Jezik {
    private:
        tcp::acceptor acceptor;
        function<json_map(int, string_view, const map<string, any>&) > handler;

    public:
        /**
         * Implementation of virtual function to accept a TCP connection, pass it on to a parser and listen for further connections.
         */
        void do_read();

        /**
         * Default constructs a server
         * @param[in] io_service: An asio::io_service responsible for underlying network socket
         * @param[in] endpoint: TCP endpoint where the underlying socket binds to
         * @param[in] handler: A functor which is responsible to execute appropriate command
         */
        Server(asio::io_service&, tcp::endpoint&, function<json_map(int, string_view, const map<string, any>&) >);
};
