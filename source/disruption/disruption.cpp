#include "disruption.h"

namespace navitia { namespace disruption {

ntw_ln disruptions_list(const type::PT_Data &d, const boost::posix_time::time_period action_period, const boost::posix_time::ptime now){
    ntw_ln response;
    for (const navitia::type::Network* network : d.networks){
        std::vector<const navitia::type::Line*> temp;
        for(const navitia::type::Line* line: network->line_list){
            for(const auto message : line->get_applicable_messages(now, action_period)){
                temp.push_back(line);
                break;
            }
        }
        if(temp.size() > 0){
            response.push_back(std::make_pair(network, temp));
        }
    }
    return response;
}
}}
