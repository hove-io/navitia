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
    navitia::type::DateTime arrival, upper_bound;
    float walking_time, ratio;

    Departure_Type() : rpidx(type::invalid_idx), count(0), walking_time(0), ratio(std::numeric_limits<float>::min()) {}
};

std::vector<Departure_Type>
getDepartures(const std::vector<std::pair<type::idx_t, double> > &departs, const std::vector<std::pair<type::idx_t, double> > &destinations, bool clockwise, const float walking_speed,
              const std::vector<label_vector_t> &labels, const std::vector<std::vector<const type::JourneyPatternPoint *> > &boardings,
              const std::vector<std::vector<boarding_type> > &boarding_types, const type::AccessibiliteParams & accessibilite_params/*const type::Properties &required_properties*/, const type::Data &data);

std::vector<Departure_Type>
getDepartures(const std::vector<std::pair<type::idx_t, double> > &departs, const navitia::type::DateTime &dep, bool clockwise, const float walking_speed, const type::Data & data);

std::vector<Departure_Type>
getWalkingSolutions(bool clockwise, const std::vector<std::pair<type::idx_t, double> > &departs, const std::vector<std::pair<type::idx_t, double> > &destinations, Departure_Type best,
                    const float walking_speed, const std::vector<label_vector_t> &labels, const std::vector<std::vector<const type::JourneyPatternPoint*> > &boardings,
                    const std::vector<std::vector<boarding_type> > &boarding_types,const type::Data &data);

std::vector<Departure_Type>
getParetoFront(bool clockwise, const std::vector<std::pair<type::idx_t, double> > &departs, const std::vector<std::pair<type::idx_t, double> > &destinations, const float walking_speed,
               const std::vector<label_vector_t> &labels, const std::vector<std::vector<const type::JourneyPatternPoint*> >&boardings, const std::vector<std::vector<boarding_type> > &boarding_types,
               const type::AccessibiliteParams & accessibilite_params/*const type::Properties &required_properties*/, const type::Data &data);

std::pair<type::idx_t, navitia::type::DateTime>
getFinalRpidAndDate(int count, type::idx_t rpid, bool clockwise, const std::vector<label_vector_t> &labels,
                    const std::vector<std::vector<const type::JourneyPatternPoint *> >&boardings, const std::vector<std::vector<boarding_type> >&boarding_types, const type::Data &data);

float
getWalkingTime(int count, type::idx_t rpid, const std::vector<std::pair<type::idx_t, double> > &departs, const std::vector<std::pair<type::idx_t, double> > &destinations,
               bool clockwise, const std::vector<label_vector_t> &, const std::vector<std::vector<const type::JourneyPatternPoint *> >&boardings,
               const std::vector<std::vector<boarding_type> > &boarding_types, const type::Data &data);

}}
