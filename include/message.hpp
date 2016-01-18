#ifndef MESSAGE_HPP_INCLUDED
#define MESSAGE_HPP_INCLUDED

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <experimental/string_view>

using namespace std;
using namespace std::experimental;


class Message {
    public:
        enum { header_length = 4};
        enum { max_body_length = 512};

    private:
        char data_[header_length + max_body_length];
        size_t body_length_;

    public:
        Message() : body_length_(0) {}

        string_view data() const {
            return data_;
        }

        char* data() {
            return data_;
        }

        size_t length() const {
            return header_length + body_length_;
        }

        string_view body() const {
            return data_ + header_length;
        }

        char* body() {
            return data_ + header_length;
        }

        size_t body_length() const {
            return body_length_;
        }

        void body_length(size_t new_length) {
            body_length_ = new_length > max_body_length ? max_body_length : new_length;
        }

        bool decode_header() {
            char header[header_length + 1] = "";
            strncat(header, data_, header_length);
            body_length_ = atoi(header);

            if (body_length_ > max_body_length) {
                body_length_ = 0;
                return false;
            }

            return true;
        }

        void encode_header() {
            char header[header_length] = "";
            sprintf(header, "%4d", static_cast<int>(body_length_));
            memcpy(data_, header, header_length);
        }
};

#endif
