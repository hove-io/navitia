#include "disruption_api.h"
#include "type/pb_converter.h"
#include "ptreferential/ptreferential.h"

namespace navitia { namespace disruption_api {

pbnavitia::Response disruptions(const navitia::type::Data &d, const std::string &str_dt,
                                const uint32_t depth,
                                uint32_t ,
                                uint32_t , const std::string &filter){
    pbnavitia::Response pb_response;
    boost::posix_time::ptime now ;
    std::vector<navitia::type::idx_t> line_idx;
    try{
        now = boost::posix_time::from_iso_string(str_dt);
    }catch(boost::bad_lexical_cast){
           fill_pb_error(pbnavitia::Error::unable_to_parse, "Unable to parse Datetime", pb_response.mutable_error());
           return pb_response;
       }

    boost::posix_time::time_period action_period(now, now);
    const std::vector<std::string> forbidden_uris;
    try{
        line_idx = ptref::make_query(type::Type_e::Line, filter, forbidden_uris, d);
    } catch(const ptref::parsing_error &parse_error) {
        fill_pb_error(pbnavitia::Error::unable_to_parse, "Unable to parse Datetime" + parse_error.more, pb_response.mutable_error());
        return pb_response;
    } catch(const ptref::ptref_error &ptref_error){
        fill_pb_error(pbnavitia::Error::bad_filter, "ptref : "  + ptref_error.more, pb_response.mutable_error());
        return pb_response;
    }

    navitia::disruption::network_line list = navitia::disruption::disruptions_list(line_idx, d.pt_data, action_period, now);
    for(auto pair_network_line : list){
        pbnavitia::Disruption* pb_disruption = pb_response.add_disruptions();
        pbnavitia::Network* pb_network = pb_disruption->mutable_network();
        navitia::fill_pb_object(pair_network_line.first, d, pb_network, depth, now, action_period);
        for(auto line : pair_network_line.second){
            pbnavitia::Line* pb_line = pb_disruption->add_lines();
            navitia::fill_pb_object(line, d, pb_line, depth-1, now, action_period);
        }
    }
    auto pagination = pb_response.mutable_pagination();
    pagination->set_totalresult(list.size());
    pagination->set_startpage(0);
    pagination->set_itemsperpage(list.size());
    pagination->set_itemsonpage(pb_response.disruptions_size());

    if (pb_response.disruptions_size() == 0) {
        fill_pb_error(pbnavitia::Error::no_solution, "no solution found for this disruption",
        pb_response.mutable_error());
        pb_response.set_response_type(pbnavitia::NO_SOLUTION);
    }
    return pb_response;
}
}
}
