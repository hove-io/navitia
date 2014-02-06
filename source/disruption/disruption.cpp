#include "disruption.h"
#include "ptreferential/ptreferential.h"

namespace navitia { namespace disruption {

type::idx_t Disruption::find_or_create(const type::Network* network){
    auto find_predicate = [&](disrupt network_disrupt) {
        return network->idx == network_disrupt.network_idx;
    };
    auto it = std::find_if(this->disrupts.begin(),
                           this->disrupts.end(),
                           find_predicate);
    if(it == this->disrupts.end()){
        disrupt dist;
        dist.network_idx = network->idx;
        dist.idx = this->disrupts.size();
        this->disrupts.push_back(dist);
        return dist.idx;
    }
    return it->idx;
}

void Disruption::add_stop_areas(const std::vector<type::idx_t>& network_idx,
                      const std::string& filter,
                      const std::vector<std::string>& forbidden_uris,
                      const type::Data &d,
                      const boost::posix_time::time_period action_period,
                      const boost::posix_time::ptime now){

    for(auto idx : network_idx){
        const auto* network = d.pt_data.networks[idx];
        std::string new_filter = "network.uri=" + network->uri;
        if(!filter.empty()){
            new_filter  += " and " + filter;
        }
        std::vector<type::idx_t> line_list;

       try{
            line_list = ptref::make_query(type::Type_e::StopArea, new_filter,
                    forbidden_uris, d);
        } catch(const ptref::parsing_error &parse_error) {
            LOG4CPLUS_WARN(logger, "Disruption::add_stop_areas : Unable to parse filter "
                                + parse_error.more);
        } catch(const ptref::ptref_error &ptref_error){
            LOG4CPLUS_WARN(logger, "Disruption::add_stop_areas : ptref : "  + ptref_error.more);
        }
        for(auto stop_area_idx : line_list){
            const auto* stop_area = d.pt_data.stop_areas[stop_area_idx];
            if (stop_area->has_applicable_message(now, action_period)){
                disrupt& dist = this->disrupts[this->find_or_create(network)];
                auto find_predicate = [&](type::idx_t idx ) {
                    return stop_area->idx == idx;
                };
                auto it = std::find_if(dist.stop_area_idx.begin(),
                                       dist.stop_area_idx.end(),
                                       find_predicate);
                if(it == dist.stop_area_idx.end()){
                    dist.stop_area_idx.push_back(stop_area->idx);
                }
            }
        }
    }
}

void Disruption::add_networks(const std::vector<type::idx_t>& network_idx,
                      const type::Data &d,
                      const boost::posix_time::time_period action_period,
                      const boost::posix_time::ptime now){

    for(auto idx : network_idx){
        const auto* network = d.pt_data.networks[idx];
        if (network->has_applicable_message(now, action_period)){
            this->disrupts[this->find_or_create(network)];
        }
    }
}

void Disruption::add_lines(const std::string& filter,
                      const std::vector<std::string>& forbidden_uris,
                      const type::Data &d,
                      const boost::posix_time::time_period action_period,
                      const boost::posix_time::ptime now){

    std::vector<type::idx_t> line_list;
    try{
        line_list  = ptref::make_query(type::Type_e::Line, filter,
                forbidden_uris, d);
    } catch(const ptref::parsing_error &parse_error) {
        LOG4CPLUS_WARN(logger, "Disruption::add_lines : Unable to parse filter " + parse_error.more);
    } catch(const ptref::ptref_error &ptref_error){
        LOG4CPLUS_WARN(logger, "Disruption::add_lines : ptref : "  + ptref_error.more);
    }
    for(auto idx : line_list){
        const auto* line = d.pt_data.lines[idx];
        if (line->has_applicable_message(now, action_period)){
            disrupt& dist = this->disrupts[this->find_or_create(line->network)];
            auto find_predicate = [&](type::idx_t idx ) {
                return line->idx == idx;
            };
            auto it = std::find_if(dist.line_idx.begin(),
                                   dist.line_idx.end(),
                                   find_predicate);
            if(it == dist.line_idx.end()){
                dist.line_idx.push_back(line->idx);
            }
        }
    }
}

void Disruption::disruptions_list(const std::string& filter,
                        const std::vector<std::string>& forbidden_uris,
                        const type::Data &d,
                        const boost::posix_time::time_period action_period,
                        const boost::posix_time::ptime now){

    std::vector<type::idx_t> network_idx = ptref::make_query(type::Type_e::Network, filter,
                                                                                      forbidden_uris, d);
    add_networks(network_idx, d, action_period, now);
    add_lines(filter, forbidden_uris, d, action_period, now);
    add_stop_areas(network_idx, filter, forbidden_uris, d, action_period, now);
}
const std::vector<disrupt>& Disruption::get_disrupts() const{
    return this->disrupts;
}

size_t Disruption::get_disrupts_size(){
    return this->disrupts.size();
}

}}
