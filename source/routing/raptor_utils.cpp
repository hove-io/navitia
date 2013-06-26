#include "raptor_utils.h"

namespace navitia { namespace routing {

//boarding_type
//get_type(size_t count, type::idx_t journey_pattern_point, const std::vector<label_vector_t> &labels, const std::vector<vector_idx> &boardings, const navitia::type::Data &data) {


const type::JourneyPatternPoint*
get_boarding_jpp(size_t count, type::idx_t journey_pattern_point_idx, const std::vector<std::vector<const type::JourneyPatternPoint*> > &boardings) {
    return boardings[count][journey_pattern_point_idx];
}


std::pair<const type::StopTime*, uint32_t>
get_current_stidx_gap(size_t count, type::idx_t journey_pattern_point, const std::vector<label_vector_t> &labels, const std::vector<std::vector<boarding_type> > &boarding_types,
                      const type::Properties &required_properties, bool clockwise,  const navitia::type::Data &data) {
    if(get_type(count, journey_pattern_point, boarding_types, data) == boarding_type::vj) {
        const type::JourneyPatternPoint* jpp = data.pt_data.journey_pattern_points[journey_pattern_point];
        return best_stop_time(jpp, labels[count][journey_pattern_point], required_properties, clockwise, data, true);
    }
    return std::make_pair(nullptr, std::numeric_limits<uint32_t>::max());
}

}}
