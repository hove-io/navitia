#pragma once
#include <google/protobuf/message.h>

struct Context {
    google::protobuf::Message* pb;

    Context() : pb(NULL){}
};
