/** @file jezik.hpp
 * @brief Defines the protocol and utility functions for the TCP server
 */
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
 * @brief Utility function to convert an unsigned integer to a buffer
 */
template <typename T, size_t N> array<T, N> to_buffer(unsigned int value) {
    array<T, N> buffer;
    size_t t_size = sizeof(T) * CHAR_BIT;

    for (int i = N; i > 0; i--) {
        buffer[i] = (value >> (t_size * (i - 1))) & 0xFF;
    }
    return buffer;
}

/**
 * @brief A class to implement basic read write and structure for TCP Messaging.
 */
class Jezik {
    protected:
        /**
         * @brief Pointer to a socket against which a connection has been made
         */
        shared_ptr<tcp::socket> socket_ptr;

        /**
         * @brief Read data from socket to a buffer.
         * @details Specifically used to read a single byte of data representing integral values in the range of 0-255.
         * @param[in,out] : A buffer to which data has to be read
         */
        void read(unsigned char*);

        /**
         * Read data from socket to a buffer with optional size
         * @param[in,out] : A buffer to which data has to be read
         * @param[in] : An optional length indicating the size of data to be read(defaults to length of entire buffer).
         */
        void read(char*, size_t = 0);

    public:
        /*
         * Default constructs the protocol handler
         * @param[in] : Pointer to socket against which connection has been established
         */
        Jezik (shared_ptr<tcp::socket>);

        /**
         * Pure virtual virtual responsible for appropriate calls to read() depending on buffers it needs to populate
         */
        virtual void do_read() = 0;

        /**
         * Writes to the tcp socket
         * @param[in] : Data to be written to socket
         */
        void do_write(string_view);
};

/**
 * @brief Class representing an argument token.
 * @details Each argument token consists of 2 parts
 * - A 1 byte header (unsigned char) specifying the size of the body
 * - A 1-256 length of body characters specifying the value of this token
 */
class ArgumentToken : public Jezik {
    private:
        /**
         * @brief Buffer to hold the body of token
         */
        char data[256];

        /**
         * @brief Buffer to hold the length of the body of token
         */
        unsigned char body_length[1];

        /**
         * @brief Reads the body length of the token
         */
        void do_read_body_length(); 

        /**
         * @brief Reads the body of the token 
         */
        void do_read_body();

    public:
        /**
         * @brief Default constructs a message handler
         * @param[in] : Pointer to socket against which connection has been established
         */
        ArgumentToken(shared_ptr<tcp::socket>);

        /**
         * @brief Reads argument token from socket
         */
        void do_read();

        /*
         * @brief Fetches the value of ArgumentToken
         * @return The body of the ArgumentToken
         */
        string_view value() const;
};

/**
 * @brief Class to implement the Argument Messaging Protocol.
 * @details Implement an argument messaging protocol to represent a named argument(i.e. arg_name = arg_value). Each argument consists of a
 * - 3 byte keyword specifying the argument value type (INT, STR, DBL)
 * - An argument token as defined by ArgumentToken specifying the argument name
 * - An argument token as defined by ArgumentToken specifying the argument value
 */
class Argument : public Jezik {
    private:
        /**
         * @brief A buffer to hold the argument type
         */
        char argument_type[3];

        /**
         * @brief ArgumentToken to hold the argument name
         */
        ArgumentToken argn{socket_ptr};

        /**
         * @brief ArgumentToken to hold the argument value
         */
        ArgumentToken argv{socket_ptr};

        /**
         * @brief A map exposing the named argument as a key, value pair
         */
        pair<string_view, any> data;

        /**
         * @brief Read the argument type from socket
         */
        void do_read_argument_type();

        /**
         * @brief Read an ArgumentToken from the socket
         */
        void do_read_argument_token();

        /**
         * @brief Fetch the argument type
         * @return A string representing the argument type
         */
        string_view argt();

    public:
        /**
         * @brief Default constructs an Argument
         * @param[in] : Pointer to socket against which connection has been established
         */
        Argument(shared_ptr<tcp::socket>);

        /**
         * @brief Read named argument from socket
         */
        void do_read();

        /**
         * @brief Fetch the named argument
         * @return Named argument as a pair of key, value
         */
        pair<string_view, any> value() const;
};

/**
 * @brief Class to implement a command in the messaging protocol over TCP
 * @details A command consists of the following components
 * - Mode (Integer[0-1])
 * - Nargs (Integer[0-255])
 * - Command (string{0-4})
 * - Arguments (Map of named arguments to be used against command)
 */
class Command : public Jezik, public enable_shared_from_this<Command>  {
    private:
        /**
         * @brief Buffer to store mode
         */
        unsigned char mode[1];

        /**
         * @brief Buffer to store number of arguments
         */
        unsigned char nargs[1];

        /**
         * @brief Buffer to store command
         */
        char command[4];

        /**
         * @brief A map holding named arguments as key: value
         */
        map<string, any> kwargs;

        /**
         * @brief Executes a successfully read command
         */
        void run_command();

        /**
         * @brief Fetch the parsed command
         * @return String representation of the command
         */
        string_view cmd();

    public:

        /**
         * @brief Default constructs an instance of command to read from an accepted tcp socket
         * @param[in] : Pointer to socket against which connection has been established
         */
        Command(tcp::socket);

        /**
         * @brief Implementaion of the virtual function to read the command from the socket
         */
        void do_read();

        /**
         * @brief Wrapper function to make appropriate underlying calls to parse command from socket, execute it and write appropriate response to socket
         * @param[in] : A functor which takes the command as an input and executes it.
         */
        void start(function<json_map(int, string_view, const map<string, any>&) >);
};

/**
 * @brief Allow reading command in a separate thread
 */
void do_read(shared_ptr<Command>, function<json_map(int, string_view, const map<string, any>&) >);

/**
 * @brief Class implementing the server responsible for accepting TCP connections.
 */
class Server {
    private:
        /**
         * @brief An acceptor to bind a socket to an endpoint
         */
        tcp::acceptor acceptor;

        tcp::socket socket;

        /**
         * @brief A functor which takes a Command as input and passes it on to an appropriate solver
         */
        function<json_map(int, string_view, const map<string, any>&) > handler;

    public:
        /**
         * @brief Implementation of virtual function to accept a TCP connection, pass it on to a parser and listen for further connections.
         */
        void do_accept();

        /**
         * @brief Default constructs a server
         * @param[in] : An io_service responsible for underlying network socket
         * @param[in] : Endpoint where the underlying socket binds to
         * @param[in] : A functor which takes a Command as input and passes it on to an appropriate solver
         */
        Server(asio::io_service&, tcp::endpoint&, function<json_map(int, string_view, const map<string, any>&) >);
};
