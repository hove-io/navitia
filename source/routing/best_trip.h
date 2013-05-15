#pragma once

#include "type/data.h"
#include "routing/dataraptor.h"


namespace navitia { namespace routing {
///Cherche le premier stop_time partant apres dt sur la journey_pattern au journey_pattern point order
std::pair<type::idx_t, uint32_t>
earliest_stop_time(const type::JourneyPatternPoint & jpp,
              const navitia::type::DateTime &dt,
              const type::Data &data, const type::Properties &required_properties=0);
///Cherche le premier stop_time partant avant dt sur la journey_pattern au journey_pattern point order
std::pair<type::idx_t, uint32_t>
tardiest_stop_time(const type::JourneyPatternPoint & jpp,
              const navitia::type::DateTime &dt, const type::Data &data,
              const type::Properties &required_properties=0);

/// Returns the next stop time at given journey pattern point
/// either a vehicle that leaves or that arrives depending on clockwise
std::pair<type::idx_t, uint32_t>
best_stop_time(const type::JourneyPatternPoint & jpp,
          const navitia::type::DateTime &dt,
          const type::Properties &required_properties,
          const bool clockwise, const type::Data &data);

/// Calcule le gap pour les horaires en fréquences
inline uint32_t compute_gap(uint32_t hour, uint32_t start_time, uint32_t headway_secs) {
    float tmp = hour-start_time + (hour>start_time ? 0 : type::DateTime::SECONDS_PER_DAY);

    return std::ceil(tmp/headway_secs)*headway_secs;
}
}}
