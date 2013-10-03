#pragma once

#include "type/data.h"
#include "routing/dataraptor.h"


namespace navitia { namespace routing {
///Cherche le premier stop_time partant apres dt sur la journey_pattern au journey_pattern point order
std::pair<const type::StopTime*, uint32_t>
earliest_stop_time(const type::JourneyPatternPoint* jpp,
              const DateTime dt,
              const type::Data &data, bool reconstructing_path = false,
              const type::AccessibiliteParams & accessibilite_params = type::AccessibiliteParams()/*const type::Properties &required_properties=0*/);
///Cherche le premier stop_time partant avant dt sur la journey_pattern au journey_pattern point order
std::pair<const type::StopTime*, uint32_t>
tardiest_stop_time(const type::JourneyPatternPoint* jpp,
              const DateTime dt, const type::Data &data, bool reconstructing_path,
              /*const type::Properties &required_properties=0*/
              const type::AccessibiliteParams & accessibilite_params);

/// Returns the next stop time at given journey pattern point
/// either a vehicle that leaves or that arrives depending on clockwise
std::pair<const type::StopTime*, uint32_t>
best_stop_time(const type::JourneyPatternPoint* jpp,
          const DateTime &dt,
          /*const type::Properties &required_properties*/
          const type::AccessibiliteParams & accessibilite_params,
          const bool clockwise, const type::Data &data, bool reconstructing_path = false);

/// Calcule le gap pour les horaires en fréquences
inline uint32_t compute_gap(uint32_t hour, uint32_t start_time, uint32_t headway_secs, bool clockwise) {
    float tmp = hour-start_time + (hour>start_time ? 0 : DateTimeUtils::SECONDS_PER_DAY);
    int x = clockwise ? std::ceil(tmp/headway_secs) : std::floor(tmp/headway_secs);
    BOOST_ASSERT((clockwise && (x*headway_secs+start_time >= hour)) ||
                 (!clockwise && (x*headway_secs+start_time <= hour)));
    BOOST_ASSERT((clockwise && (x*headway_secs+start_time - hour) <= headway_secs) ||
                 (!clockwise && (hour - (x*headway_secs+start_time)) <= headway_secs));
    return x*headway_secs;
}
}}
