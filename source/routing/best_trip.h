#pragma once

#include "type/data.h"
#include "routing/dataraptor.h"


namespace navitia { namespace routing {
///Cherche le premier trip partant apres dt sur la journey_pattern au journey_pattern point order
std::pair<type::idx_t, uint32_t> 
earliest_trip(const type::JourneyPatternPoint & jpp,
              const navitia::type::DateTime &dt,
              const type::Data &data, const type::Properties &required_properties=0);
///Cherche le premier trip partant avant dt sur la journey_pattern au journey_pattern point order
std::pair<type::idx_t, uint32_t> 
tardiest_trip(const type::JourneyPatternPoint & jpp,
              const navitia::type::DateTime &dt, const type::Data &data,
              const type::Properties &required_properties=0);


std::pair<type::idx_t, uint32_t>
best_trip(const type::JourneyPatternPoint & jpp,
          const navitia::type::DateTime &dt,
          const type::Properties &required_properties,
          const bool clockwise, const type::Data &data);

/// Calcule le gap pour les horaires en fréquences
inline uint32_t compute_gap(uint32_t hour, uint32_t start_time, uint32_t headway_secs) {
    float tmp = hour-start_time + (hour>start_time ? 0 : type::DateTime::SECONDS_PER_DAY);

    return std::ceil(tmp/headway_secs)*headway_secs;
}
}}
