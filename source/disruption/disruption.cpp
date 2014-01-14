#include "disruption.h"
#include "ptreferential/ptreferential.h"

namespace navitia { namespace disruption {

network_line_list disruptions_list(const std::string& filter,
                        const std::vector<std::string>& forbidden_uris,
                        const type::Data &d,
                        const boost::posix_time::time_period action_period,
                        const boost::posix_time::ptime now){
    network_line_list response;
    auto line_list = ptref::make_query(type::Type_e::Line, filter,
            forbidden_uris, d);

    for(auto idx : line_list){
        const auto* line = d.pt_data.lines[idx];
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
