#include "disruption.h"

namespace navitia { namespace disruption {

ntw_ln disruptions_list(const std::vector<type::idx_t>& line_idx, const type::PT_Data &d,
                        const boost::posix_time::time_period action_period, const boost::posix_time::ptime now){
    ntw_ln response;

    for(auto idx : line_idx){
        const auto *line = d.lines[idx];
        for(const auto message : line->get_applicable_messages(now, action_period)){

            auto find_predicate = [&](pair_ntw_ln ntw_list) {
                return ntw_list.first->idx == line->network->idx;
            };
            auto it = std::find_if(response.begin(), response.end(), find_predicate);
            if(it == response.end()){
                response.push_back(std::make_pair(line->network, std::vector<const navitia::type::Line*> {line}));
            }else{
                    it->second.push_back(line);
                }
            }
    }
    return response;
}
}}
