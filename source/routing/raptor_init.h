#pragma once
#include "type/type.h"
#include "type/data.h"
#include "routing.h"
#include "raptor_utils.h"
#include "best_stoptime.h"

namespace navitia { namespace routing {

struct Departure_Type {
    type::idx_t rpidx;
    uint32_t count;
    DateTime arrival, upper_bound;
    float ratio;
    boost::posix_time::time_duration walking_time = {};

    Departure_Type() : rpidx(type::invalid_idx), count(0),
                       arrival(DateTimeUtils::inf), upper_bound(DateTimeUtils::inf),
                       ratio(std::numeric_limits<float>::min()) {}

};

std::vector<Departure_Type>
getDepartures(const std::vector<std::pair<type::idx_t, boost::posix_time::time_duration> > &departs, const std::vector<std::pair<type::idx_t, boost::posix_time::time_duration> > &destinations, bool clockwise,
              const std::vector<label_vector_t> &labels, const type::AccessibiliteParams & accessibilite_params, const type::Data &data, bool disruption_active);

std::vector<Departure_Type>
getDepartures(const std::vector<std::pair<type::idx_t, boost::posix_time::time_duration> > &departs, const DateTime &dep, bool clockwise, const type::Data & data, bool disruption_active);

std::vector<Departure_Type>
getWalkingSolutions(bool clockwise, const std::vector<std::pair<type::idx_t, boost::posix_time::time_duration> > &departs, const std::vector<std::pair<type::idx_t, boost::posix_time::time_duration> > &destinations, Departure_Type best,
                    const std::vector<label_vector_t> &labels, const type::Data &data);

std::vector<Departure_Type>
getParetoFront(bool clockwise, const std::vector<std::pair<type::idx_t, boost::posix_time::time_duration> > &departs, const std::vector<std::pair<type::idx_t, boost::posix_time::time_duration> > &destinations,
               const std::vector<label_vector_t> &labels,
               const type::AccessibiliteParams & accessibilite_params, const type::Data &data, bool disruption_active);

std::pair<type::idx_t, DateTime>
getFinalRpidAndDate(int count, type::idx_t jpp_idx, bool clockwise, const std::vector<label_vector_t> &labels);

boost::posix_time::time_duration
getWalkingTime(int count, type::idx_t rpid, const std::vector<std::pair<type::idx_t, boost::posix_time::time_duration> > &departs, const std::vector<std::pair<type::idx_t, boost::posix_time::time_duration> > &destinations,
               bool clockwise, const std::vector<label_vector_t> &labels, const type::Data &data);

}}
