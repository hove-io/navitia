#include "disruption_api.h"
#include "type/pb_converter.h"
namespace navitia { namespace disruption_api {

pbnavitia::Response disruptions(const navitia::type::Data &d,
                                const boost::posix_time::ptime now,
                                uint32_t depth){
    pbnavitia::Response pb_response;
    boost::posix_time::time_period action_period(now, now);
    navitia::disruption::ntw_ln list = navitia::disruption::disruptions_list(d.pt_data, action_period, now);

    for(auto pair : list){
        pbnavitia::Disruption* pb_disruption = pb_response.add_disruptions();
        pbnavitia::Network* pb_network = pb_disruption->mutable_network();
        navitia::fill_pb_object(pair.first, d, pb_network, depth, now,action_period);
        for(auto line : pair.second){
            pbnavitia::Line* pb_line = pb_disruption->add_lines();
            navitia::fill_pb_object(line, d, pb_line, depth-1, now,action_period);
        }
    }
    return pb_response;
}
}
}
