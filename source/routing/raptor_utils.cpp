#include "raptor_utils.h"

namespace navitia { namespace routing {

//boarding_type
//get_type(size_t count, type::idx_t journey_pattern_point, const std::vector<label_vector_t> &labels, const std::vector<vector_idx> &boardings, const navitia::type::Data &data) {


type::idx_t
get_boarding_jpp(size_t count, type::idx_t journey_pattern_point, const std::vector<label_vector_t> &labels, const std::vector<vector_idx> &boardings, const navitia::type::Data &data) {
    auto type = get_type(count, journey_pattern_point, labels, boardings, data);
    if(type == boarding_type::vj) {
        return data.pt_data.stop_times[boardings[count][journey_pattern_point]].journey_pattern_point_idx;
    } else if(type == boarding_type::connection){
        return boardings[count][journey_pattern_point] - data.pt_data.stop_times.size();
    } else {
        return type::invalid_idx;
    }
}


std::pair<type::idx_t, uint32_t>
get_current_stidx_gap(size_t count, type::idx_t journey_pattern_point, const std::vector<label_vector_t> &labels, const std::vector<vector_idx> &boardings,
                      const type::Properties &required_properties, bool clockwise,  const navitia::type::Data &data) {
    if(get_type(count, journey_pattern_point, labels, boardings, data) == boarding_type::vj) {
        type::idx_t vj;
        uint32_t gap;
        const type::JourneyPatternPoint & jpp = data.pt_data.journey_pattern_points[journey_pattern_point];
        std::tie(vj, gap) = best_trip(jpp, labels[count][journey_pattern_point], required_properties, clockwise, data);
        return std::make_pair(data.pt_data.vehicle_journeys[vj].stop_time_list[jpp.order], gap);
    }
    return std::make_pair(type::invalid_idx, std::numeric_limits<uint32_t>::max());
}

}}
