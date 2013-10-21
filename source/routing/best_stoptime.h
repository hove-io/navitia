#pragma once

#include "type/data.h"
#include "routing/dataraptor.h"


namespace navitia { namespace routing {
///Cherche le premier stop_time partant apres dt sur la journey_pattern au journey_pattern point order
///. Renvoie le stop time et la première heure de départ après dt
std::pair<const type::StopTime*, uint32_t>
earliest_stop_time(const type::JourneyPatternPoint* jpp,
              const DateTime dt,
              const type::Data &data, bool reconstructing_path = false,
              const type::AccessibiliteParams & accessibilite_params = type::AccessibiliteParams()/*const type::Properties &required_properties=0*/);
///Cherche le premier stop_time partant avant dt sur la journey_pattern au journey_pattern point order
/// Renvoie la première heure d'arrivée avant dt
std::pair<const type::StopTime*, uint32_t>
tardiest_stop_time(const type::JourneyPatternPoint* jpp,
              const DateTime dt, const type::Data &data, bool reconstructing_path,
              const type::AccessibiliteParams & accessibilite_params);

/// Returns the next stop time at given journey pattern point
/// either a vehicle that leaves or that arrives depending on clockwise
std::pair<const type::StopTime*, uint32_t>
best_stop_time(const type::JourneyPatternPoint* jpp,
          const DateTime dt,
          /*const type::Properties &required_properties*/
          const type::AccessibiliteParams & accessibilite_params,
          const bool clockwise, const type::Data &data, bool reconstructing_path = false);

/// Pour les horaires en frequences

inline u_int32_t f_arrival_time(uint32_t hour, const type::StopTime* st) {
    const u_int32_t lower_bound = st->start_time;
    const u_int32_t higher_bound = st->end_time - (st->departure_time - st->arrival_time);
    // If higher bound is overmidnight the hour can be either in [lower_bound;midnight] or in
    // [midnight;higher_bound]
    if((higher_bound < DateTimeUtils::SECONDS_PER_DAY && hour>=lower_bound && hour<=higher_bound) ||
            (higher_bound > DateTimeUtils::SECONDS_PER_DAY && (
                (hour>=lower_bound && hour <= DateTimeUtils::SECONDS_PER_DAY)
                 || (hour + DateTimeUtils::SECONDS_PER_DAY) <= higher_bound) )) {
        if(hour < lower_bound)
            hour += DateTimeUtils::SECONDS_PER_DAY;
        const uint32_t x = std::floor(double(hour - lower_bound) / double(st->headway_secs));
        BOOST_ASSERT(x*st->headway_secs+lower_bound <= hour);
        BOOST_ASSERT(((hour - (x*st->headway_secs+lower_bound))%DateTimeUtils::SECONDS_PER_DAY) <= st->headway_secs);
        return lower_bound + x * st->headway_secs;
    } else {
        return higher_bound;
    }
}


inline u_int32_t f_departure_time(uint32_t hour, const type::StopTime* st) {
    const u_int32_t lower_bound = st->start_time + (st->departure_time - st->arrival_time);
    const u_int32_t higher_bound = st->end_time;
    // If higher bound is overmidnight the hour can be either in [lower_bound;midnight] or in
    // [midnight;higher_bound]
    if((higher_bound < DateTimeUtils::SECONDS_PER_DAY && hour>=lower_bound && hour<=higher_bound) ||
            (higher_bound > DateTimeUtils::SECONDS_PER_DAY && (
                (hour>=lower_bound && hour <= DateTimeUtils::SECONDS_PER_DAY)
                 || (hour + DateTimeUtils::SECONDS_PER_DAY) <= higher_bound) )) {
        if(hour < lower_bound)
            hour += DateTimeUtils::SECONDS_PER_DAY;
        const uint32_t x = std::ceil(double(hour - lower_bound) / double(st->headway_secs));
        BOOST_ASSERT((x*st->headway_secs+lower_bound) >= hour);
        BOOST_ASSERT((((x*st->headway_secs+lower_bound) - hour)%DateTimeUtils::SECONDS_PER_DAY) <= st->headway_secs);
        return lower_bound + x * st->headway_secs;
    } else {
        return lower_bound;
    }
}

inline uint32_t compute_gap(const uint32_t hour, const uint32_t start_time, const uint32_t end_time, const  uint32_t headway_secs, const bool clockwise) {
    if((hour>=start_time && hour <= end_time)
            || (end_time>DateTimeUtils::SECONDS_PER_DAY && ((end_time-DateTimeUtils::SECONDS_PER_DAY)>=hour))) {
        const double tmp = double(hour - start_time) / double(headway_secs);
        const uint32_t x = (clockwise ? std::ceil(tmp) : std::floor(tmp));
        BOOST_ASSERT((clockwise && (x*headway_secs+start_time >= hour)) ||
                     (!clockwise && (x*headway_secs+start_time <= hour)));
        BOOST_ASSERT((clockwise && (((x*headway_secs+start_time) - hour)%DateTimeUtils::SECONDS_PER_DAY) <= headway_secs) ||
                     (!clockwise && ((hour - (x*headway_secs+start_time))%DateTimeUtils::SECONDS_PER_DAY) <= headway_secs));
        return x * headway_secs;
    } else {
        return 0;
    }
}
}}
