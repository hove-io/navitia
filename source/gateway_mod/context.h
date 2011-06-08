#pragma once
#include <google/protobuf/message.h>
#include <memory>




struct Context {
    enum Service{
        UNKNOWN,
        PTREF
    };

    std::unique_ptr<google::protobuf::Message> pb;
    Service service;



    Context() : service(UNKNOWN){}
};
