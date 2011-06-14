#pragma once
#include <google/protobuf/message.h>
#include <memory>




struct Context {
    enum Service{
        UNKNOWN,
        BAD_RESPONSE,
        PTREF
    };

    std::unique_ptr<google::protobuf::Message> pb;
    Service service;
    std::string str;



    Context() : service(UNKNOWN){}
};
