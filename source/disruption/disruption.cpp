#include "disruption.h"

namespace navitia { namespace disruption {

network_line disruptions_list(const std::vector<type::idx_t>& line_idx, const type::PT_Data &d,
                        const boost::posix_time::time_period action_period, const boost::posix_time::ptime now){
    network_line response;

    for(auto idx : line_idx){
        const auto *line = d.lines[idx];
        auto find_predicate = [&](pair_network_line ntw_list) {
            return ntw_list.first->idx == line->network->idx;
        };
        if (line->has_applicable_message(now, action_period)){
            auto it = std::find_if(response.begin(), response.end(), find_predicate);
            if(it == response.end()){
                response.push_back(std::make_pair(line->network, std::vector<const navitia::type::Line*> {line}));
            }else{
                    it->second.push_back(line);
                }
        }else{
            if (line->network->has_applicable_message(now, action_period)){
                auto it = std::find_if(response.begin(), response.end(), find_predicate);
                if(it == response.end()){
                    response.push_back(std::make_pair(line->network, std::vector<const navitia::type::Line*> {}));
                }
            }
        }
    }
    return response;
}
}}
