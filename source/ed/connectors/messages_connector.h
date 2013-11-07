#pragma once
#include <string>
#include <map>
#include "type/message.h"

namespace navitia{namespace type{
    struct Data;
}}

namespace ed{ namespace connectors{

struct RealtimeLoaderConfig{
    std::string connection_string;
    uint32_t shift_days;

    RealtimeLoaderConfig(const std::string& connectionstring, const uint32_t shiftdays) : connection_string(connectionstring), shift_days(shiftdays){}
};

std::map<std::string, boost::shared_ptr<navitia::type::Message>> load_messages(
        const RealtimeLoaderConfig& conf,
        const boost::posix_time::ptime& current_time);

void apply_messages(navitia::type::Data& data);

std::vector<navitia::type::AtPerturbation> load_at_perturbations(
        const RealtimeLoaderConfig& conf,
        const boost::posix_time::ptime& current_time);

}}//connectors
