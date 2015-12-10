#include <zmq.hpp>
#include <thread>


class ZMQWrapper {
    private:
        zmq::context_t context;
    public:
        ZMQWrapper() {
            context = zmq::context_t{int(std::thread::hardware_concurrency())};
        }

        void listen() {
            zmq::socket_t socket{context, ZMQ_PUB};
            socket.bind("tcp://*:5555");

            while(true) {
                zmq::message_t request;
                socket.recv(&request);
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                zmq::message_t reply{5};
                memcpy(reply.data(), "Hello", 5);
                socket.send(reply);
            }
        }
};
