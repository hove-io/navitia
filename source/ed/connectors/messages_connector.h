#pragma once
#include <string>
#include <map>
#include "type/message.h"

namespace navitia{namespace type{
    struct Data;
}}

namespace ed{ namespace connectors{

struct MessageLoaderConfig{
    std::string connection_string;
};

std::map<std::string, boost::shared_ptr<navitia::type::Message>> load_messages(
        const MessageLoaderConfig& conf,
        const boost::posix_time::ptime& current_time);

void apply_messages(navitia::type::Data& data);

}}//connectors
