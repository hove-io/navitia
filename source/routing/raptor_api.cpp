/* Copyright © 2001-2022, Hove and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Hove (www.hove.com).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!

LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

Stay tuned using
twitter @navitia
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "raptor_api.h"

#include "fare/fare.h"
#include "georef/street_network.h"
#include "heat_map.h"
#include "isochrone.h"
#include "type/datetime.h"
#include "type/meta_data.h"
#include "type/pb_converter.h"
#include "type/type_utils.h"
#include "utils/functions.h"
#include "utils/map_find.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/range/algorithm/count.hpp>
#include <boost/range/adaptor/indexed.hpp>

#include <chrono>
#include <string>
#include <unordered_set>
#include <utility>
#include <utility>

namespace navitia {
namespace routing {

// Useful to protect raptor loop
static const uint MAX_NB_RAPTOR_CALL = 100;

bool keep_going(const uint32_t total_nb_journeys,
                const uint32_t nb_try,
                const bool clockwise,
                const DateTime request_date_secs,
                const boost::optional<uint32_t>& min_nb_journeys,
                const boost::optional<DateTime>& timeframe_limit,
                const uint32_t max_transfers) {
    // we allow more call to kraken for simple request
    const uint32_t max_raptor_call = (max_transfers == 0) ? MAX_NB_RAPTOR_CALL * 10 : MAX_NB_RAPTOR_CALL;
    if (nb_try > max_raptor_call) {
        return false;
    }

    auto is_inside = [clockwise](DateTime lhs, DateTime rhs) { return clockwise ? lhs < rhs : lhs > rhs; };

    if (min_nb_journeys) {
        auto need_more_journeys_for_min = total_nb_journeys < *min_nb_journeys;

        if (timeframe_limit) {
            // Case 1: if we don't have enough journeys,
            // we keep searching until the end of time frame
            return need_more_journeys_for_min || is_inside(request_date_secs, *timeframe_limit);
        }

        // Case 2: we return all min_nb_journeys journeys that raptor can find
        return need_more_journeys_for_min;
    }

    if (timeframe_limit) {
        // Case 3: no min_nb_journeys is given, we find all journeys inside of the time frame
        return is_inside(request_date_secs, *timeframe_limit);
    }

    // Case 4: Neither time duration nor min_nb_journeys are given, we call only once Raptor
    return false;
}

/**
 * @brief Process timeframe_limit in raptor referential from request_datetime and timeframe_duration
 */
static boost::optional<DateTime> get_timeframe_limit(const DateTime request_date_secs,
                                                     const bool clockwise,
                                                     const boost::optional<uint32_t>& timeframe_duration) {
    if (!timeframe_duration) {
        return boost::none;
    }

    DateTime limit = 0;  // cannot be negative
    if (clockwise) {
        limit = request_date_secs + *timeframe_duration;
    } else if (request_date_secs > *timeframe_duration) {
        limit = request_date_secs - *timeframe_duration;
    }
    return limit;
}

/**
 * @brief internal function to call raptor in a loop
 */
static std::vector<Path> call_raptor(navitia::PbCreator& pb_creator,
                                     RAPTOR& raptor,
                                     const map_stop_point_duration& departures,
                                     const map_stop_point_duration& destinations,
                                     const std::vector<bt::ptime>& datetimes,
                                     const type::RTLevel rt_level,
                                     const navitia::time_duration& arrival_transfer_penalty,
                                     const navitia::time_duration& walking_transfer_penalty,
                                     const type::AccessibiliteParams& accessibilite_params,
                                     const std::vector<std::string>& forbidden_uri,
                                     const std::vector<std::string>& allowed_ids,
                                     const bool clockwise,
                                     const boost::optional<navitia::time_duration>& direct_path_duration,
                                     const boost::optional<uint32_t>& min_nb_journeys,
                                     const uint32_t nb_direct_path,
                                     const uint32_t max_duration,
                                     const uint32_t max_transfers,
                                     const size_t max_extra_second_pass,
                                     const double night_bus_filter_max_factor,
                                     const int32_t night_bus_filter_base_factor,
                                     const boost::optional<uint32_t>& timeframe_duration,
                                     const boost::optional<boost::posix_time::ptime>& current_datetime) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));

    std::vector<Path> pathes;

    // We loop on datetimes, but in practice there's always only one
    // (It's a deprecated feature to provide multiple datetimes).
    // TODO: remove the vector (and adapt protobuf of request).
    DateTime bound = clockwise ? DateTimeUtils::inf : DateTimeUtils::min;

    for (const auto& datetime : datetimes) {
        // Compute start time and Bound
        DateTime request_date_secs = to_datetime(datetime, raptor.data);

        // timeframe_limit in raptor referential
        boost::optional<DateTime> timeframe_limit =
            get_timeframe_limit(request_date_secs, clockwise, timeframe_duration);

        // Compute Bound
        if (max_duration != DateTimeUtils::inf) {
            if (clockwise) {
                bound = request_date_secs + max_duration;
            } else {
                bound = request_date_secs > max_duration ? request_date_secs - max_duration : 0;
            }
        }

        // Raptor Loop
        JourneySet journeys;
        uint32_t nb_try = 0;
        int total_nb_journeys = 0;

        raptor.set_valid_jp_and_jpp(DateTimeUtils::date(request_date_secs), accessibilite_params, forbidden_uri,
                                    allowed_ids, rt_level);

        do {
            auto raptor_journeys = raptor.compute_all_journeys(
                departures, destinations, request_date_secs, rt_level, arrival_transfer_penalty,
                walking_transfer_penalty, bound, max_transfers, accessibilite_params, clockwise, direct_path_duration,
                max_extra_second_pass, current_datetime);

            LOG4CPLUS_DEBUG(logger, "raptor found " << raptor_journeys.size() << " solutions");

            // Remove direct path
            filter_direct_path(raptor_journeys);

            // filter joureys that are too late.....with the magic formula...
            NightBusFilter::Params params{request_date_secs, clockwise, night_bus_filter_max_factor,
                                          night_bus_filter_base_factor};
            filter_late_journeys(raptor_journeys, params);

            modify_backtracking_journeys(raptor_journeys, departures, destinations, clockwise);

            LOG4CPLUS_DEBUG(logger, "after filtering late journeys: " << raptor_journeys.size() << " solution(s) left");

            if (raptor_journeys.empty()) {
                break;
            }

            // filter the similar journeys
            for (const auto& journey : raptor_journeys) {
                journeys.insert(journey);
            }

            nb_try++;

            total_nb_journeys = journeys.size() + nb_direct_path;

            // Prepare next call for raptor with min_nb_journeys option
            request_date_secs = prepare_next_call_for_raptor(raptor_journeys, clockwise);

        } while (keep_going(total_nb_journeys, nb_try, clockwise, request_date_secs, min_nb_journeys, timeframe_limit,
                            max_transfers));

        // create date time for next
        if (request_date_secs != to_datetime(datetime, raptor.data)) {
            pb_creator.set_next_request_date_time(to_posix_timestamp(request_date_secs, raptor.data));
        }

        auto tmp_pathes = raptor.from_journeys_to_path(journeys);

        LOG4CPLUS_DEBUG(logger, "raptor made " << tmp_pathes.size() << " Path(es)");

        // For one date time
        if (datetimes.size() == 1) {
            pathes = tmp_pathes;
            for (auto& path : pathes) {
                path.request_time = datetime;
            }
        }
        // when we have several date time,
        // we keep that arrival at the earliest / departure at the latest
        else if (!tmp_pathes.empty()) {
            tmp_pathes.back().request_time = datetime;
            pathes.push_back(tmp_pathes.back());
            bound = to_datetime(tmp_pathes.back().items.back().arrival, raptor.data);
        }
        // when we have several date time and we have no result,
        // we return an empty path
        else {
            pathes.emplace_back();
        }
    }

    return pathes;
}

static void add_coord(const type::GeographicalCoord& coord, pbnavitia::Section* pb_section) {
    auto* new_coord = pb_section->add_shape();
    new_coord->set_lon(coord.lon());
    new_coord->set_lat(coord.lat());
}

static void fill_shape(pbnavitia::Section* pb_section, const std::vector<const type::StopTime*>& stop_times) {
    if (stop_times.empty()) {
        return;
    }

    // Adding the coordinates of the first stop point
    type::GeographicalCoord prev_coord = stop_times.front()->stop_point->coord;
    add_coord(prev_coord, pb_section);

    auto prev_order = stop_times.front()->order();
    for (auto it = stop_times.begin() + 1; it != stop_times.end(); ++it) {
        const auto* st = *it;
        const auto cur_order = st->order();

        // As every stop times may not be present (because they can be
        // filtered because of estimated datetime), we can only print
        // the shape if the 2 stop times are consecutive
        if (prev_order + 1 == cur_order) {
            // If the shapes exist, we use them to generate the geometry
            if (st->shape_from_prev != nullptr) {
                for (const auto& cur_coord : *st->shape_from_prev) {
                    if (cur_coord == prev_coord) {
                        continue;
                    }
                    add_coord(cur_coord, pb_section);
                    prev_coord = cur_coord;
                }
                // otherwise, we use the stop points coordinates to draw a line
            } else {
                const auto& sp_coord = st->stop_point->coord;
                if (sp_coord != prev_coord) {
                    add_coord(sp_coord, pb_section);
                    prev_coord = sp_coord;
                }
            }
        }
        prev_order = cur_order;
    }

    // Adding the coordinates of the last stop point
    const type::GeographicalCoord& last_coord = stop_times.back()->stop_point->coord;
    if (last_coord != prev_coord) {
        add_coord(last_coord, pb_section);
    }
}

static void set_length(pbnavitia::Section* pb_section) {
    type::GeographicalCoord prev_geo;
    if (pb_section->shape_size() > 0) {
        const auto& first_coord = pb_section->shape(0);
        prev_geo = type::GeographicalCoord(first_coord.lon(), first_coord.lat());
    }
    double length = 0;
    for (int i = 1; i < pb_section->shape_size(); ++i) {
        const auto& cur_coord = pb_section->shape(i);
        const auto cur_geo = type::GeographicalCoord(cur_coord.lon(), cur_coord.lat());
        length += prev_geo.distance_to(cur_geo);
        prev_geo = cur_geo;
    }
    pb_section->set_length(length);
}

static void _update_max_severity(boost::optional<type::disruption::Effect>& worst_disruption,
                                 type::disruption::Effect new_val) {
    // the effect are sorted, the first one is the worst one
    if (!worst_disruption || static_cast<size_t>(new_val) < static_cast<size_t>(*worst_disruption)) {
        worst_disruption = new_val;
    }
}

template <typename T>
void _update_max_impact_severity(boost::optional<type::disruption::Effect>& max,
                                 const T& pb_obj,
                                 const PbCreator& pb_creator) {
    for (const auto& impact_uri : pb_obj.impact_uris()) {
        const auto* impact = pb_creator.get_impact(impact_uri);
        if (!impact) {
            continue;
        }
        _update_max_severity(max, impact->severity->effect);
    }
}

static void compute_most_serious_disruption(pbnavitia::Journey* pb_journey, const PbCreator& pb_creator) {
    boost::optional<type::disruption::Effect> max_severity = boost::none;

    for (const auto& section : pb_journey->sections()) {
        if (section.type() != pbnavitia::PUBLIC_TRANSPORT) {
            continue;
        }
        _update_max_impact_severity(max_severity, section.pt_display_informations(), pb_creator);

        _update_max_impact_severity(max_severity, section.origin().stop_point(), pb_creator);
        _update_max_impact_severity(max_severity, section.origin().stop_point().stop_area(), pb_creator);

        _update_max_impact_severity(max_severity, section.destination().stop_point(), pb_creator);
        _update_max_impact_severity(max_severity, section.destination().stop_point().stop_area(), pb_creator);
    }

    if (max_severity) {
        pb_journey->set_most_serious_disruption_effect(type::disruption::to_string(*max_severity));
    }
}

static void fill_section(PbCreator& pb_creator,
                         pbnavitia::Section* pb_section,
                         const type::VehicleJourney* vj,
                         const std::vector<const type::StopTime*>& stop_times) {
    if (vj->has_boarding()) {
        pb_section->set_type(pbnavitia::boarding);
        return;
    }
    if (vj->has_landing()) {
        pb_section->set_type(pbnavitia::landing);
        return;
    }
    auto* vj_pt_display_information = pb_section->mutable_pt_display_informations();

    const auto& vj_stoptimes = navitia::VjStopTimes(vj, stop_times);
    pb_creator.fill(&vj_stoptimes, vj_pt_display_information, 1);

    fill_shape(pb_section, stop_times);
    set_length(pb_section);
    pb_creator.fill_co2_emission(pb_section, vj);
}

static void co2_emission_aggregator(pbnavitia::Journey* pb_journey) {
    double co2_emission = 0.;
    bool to_add = false;
    if (!pb_journey->sections().empty()) {
        for (int idx_section = 0; idx_section < pb_journey->sections().size(); ++idx_section) {
            if (pb_journey->sections(idx_section).has_co2_emission()) {
                co2_emission += pb_journey->sections(idx_section).co2_emission().value();
                to_add = true;
            }
        }
    }
    if (to_add) {
        pbnavitia::Co2Emission* pb_co2_emission = pb_journey->mutable_co2_emission();
        pb_co2_emission->set_unit("gEC");
        pb_co2_emission->set_value(co2_emission);
    }
}

static void compute_metadata(pbnavitia::Journey* pb_journey) {
    uint32_t total_walking_duration = 0;
    uint32_t total_car_duration = 0;
    uint32_t total_bike_duration = 0;
    uint32_t total_ridesharing_duration = 0;
    uint32_t total_walking_distance = 0;
    uint32_t total_car_distance = 0;
    uint32_t total_bike_distance = 0;
    uint32_t total_ridesharing_distance = 0;

    for (const auto& section : pb_journey->sections()) {
        if (section.type() == pbnavitia::STREET_NETWORK || section.type() == pbnavitia::CROW_FLY) {
            switch (section.street_network().mode()) {
                case pbnavitia::StreetNetworkMode::Walking:
                    total_walking_duration += section.duration();
                    total_walking_distance += section.length();
                    break;
                case pbnavitia::StreetNetworkMode::Car:
                case pbnavitia::StreetNetworkMode::CarNoPark:
                case pbnavitia::StreetNetworkMode::Taxi:
                    total_car_duration += section.duration();
                    total_car_distance += section.length();
                    break;
                case pbnavitia::StreetNetworkMode::Bike:
                case pbnavitia::StreetNetworkMode::Bss:
                    total_bike_duration += section.duration();
                    total_bike_distance += section.length();
                    break;
                case pbnavitia::StreetNetworkMode::Ridesharing:
                    total_ridesharing_duration += section.duration();
                    total_ridesharing_distance += section.length();
                    break;
            }
        } else if (section.type() == pbnavitia::TRANSFER && section.transfer_type() == pbnavitia::walking) {
            total_walking_duration += section.duration();
        }
    }

    const auto ts_departure = pb_journey->sections(0).begin_date_time();
    const auto ts_arrival = pb_journey->sections(pb_journey->sections_size() - 1).end_date_time();
    pbnavitia::Durations* durations = pb_journey->mutable_durations();
    durations->set_walking(total_walking_duration);
    durations->set_bike(total_bike_duration);
    durations->set_car(total_car_duration);
    durations->set_taxi(0);
    durations->set_ridesharing(total_ridesharing_duration);
    durations->set_total(ts_arrival - ts_departure);

    pbnavitia::Distances* distances = pb_journey->mutable_distances();
    distances->set_walking(total_walking_distance);
    distances->set_bike(total_bike_distance);
    distances->set_car(total_car_distance);
    distances->set_taxi(0);
    distances->set_ridesharing(total_ridesharing_distance);
}

static georef::Path get_direct_path(georef::StreetNetwork& worker,
                                    const type::EntryPoint& origin,
                                    const type::EntryPoint& destination) {
    if (!origin.streetnetwork_params.enable_direct_path) {  //(direct path use only origin mode)
        return georef::Path();
    }
    return worker.get_direct_path(origin, destination);
}

void add_direct_path(PbCreator& pb_creator,
                     const georef::Path& path,
                     const type::EntryPoint& origin,
                     const type::EntryPoint& destination,
                     const std::vector<bt::ptime>& datetimes,
                     const bool clockwise) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    if (!path.path_items.empty()) {
        pb_creator.set_response_type(pbnavitia::ITINERARY_FOUND);
        LOG4CPLUS_DEBUG(logger, "direct path of " << path.duration << "s found!");

        // for each date time we add a direct street journey
        for (const auto& datetime : datetimes) {
            pbnavitia::Journey* pb_journey = pb_creator.add_journeys();
            pb_journey->set_requested_date_time(navitia::to_posix_timestamp(datetime));

            bt::ptime departure;
            if (clockwise) {
                departure = datetime;
            } else {
                departure = datetime - bt::seconds(path.duration.total_seconds());
            }
            pb_creator.fill_street_sections(origin, path, pb_journey, departure);

            const auto ts_departure = pb_journey->sections(0).begin_date_time();
            const auto ts_arrival = pb_journey->sections(pb_journey->sections_size() - 1).end_date_time();
            pb_journey->set_departure_date_time(ts_departure);
            pb_journey->set_arrival_date_time(ts_arrival);
            pb_journey->set_duration(ts_arrival - ts_departure);
            // We add coherence with the origin of the request
            auto origin_pb = pb_journey->mutable_sections(0)->mutable_origin();
            origin_pb->Clear();
            pb_creator.fill(&origin, origin_pb, 2);
            // We add coherence with the destination object of the request
            auto destination_pb = pb_journey->mutable_sections(pb_journey->sections_size() - 1)->mutable_destination();
            destination_pb->Clear();
            pb_creator.fill(&destination, destination_pb, 2);
            co2_emission_aggregator(pb_journey);
            compute_metadata(pb_journey);
        }
    }
}

/**
 * Compute base passage from amended passage, knowing amended and base stop-times
 */
static bt::ptime get_base_dt(const nt::StopTime* st_orig,
                             const nt::StopTime* st_base,
                             const bt::ptime& dt_orig,
                             bool is_departure) {
    if (st_orig == nullptr || st_base == nullptr) {
        return bt::not_a_date_time;
    }
    // compute the "base validity_pattern" day of dt_orig:
    // * shift back if amended-vj is shifted compared to base-vj
    // * shift back if the hour in stop-time of amended-vj is >24h
    auto validity_pattern_dt_day = dt_orig.date();
    validity_pattern_dt_day -= boost::gregorian::days(st_orig->vehicle_journey->shift);
    auto hour_of_day_orig = (is_departure ? st_orig->departure_time : st_orig->arrival_time);
    validity_pattern_dt_day -= boost::gregorian::days(hour_of_day_orig / DateTimeUtils::SECONDS_PER_DAY);
    // from the "base validity_pattern" day, we simply have to apply stop_time from base_vj (st_base)
    auto hour_of_day_base = (is_departure ? st_base->departure_time : st_base->arrival_time);
    return {validity_pattern_dt_day, boost::posix_time::seconds(hour_of_day_base)};
}

static bt::ptime handle_pt_sections(pbnavitia::Journey* pb_journey,
                                    PbCreator& pb_creator,
                                    const navitia::routing::Path& path,
                                    const uint32_t depth) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    pb_journey->set_nb_transfers(path.nb_changes);
    pb_journey->set_requested_date_time(navitia::to_posix_timestamp(path.request_time));
    bt::ptime arrival_time;

    pb_journey->set_sn_dur(path.sn_dur.total_seconds());
    pb_journey->set_transfer_dur(path.transfer_dur.total_seconds());
    pb_journey->set_min_waiting_dur(path.min_waiting_dur.total_seconds());
    pb_journey->set_nb_vj_extentions(path.nb_vj_extentions);
    pb_journey->set_nb_sections(path.nb_sections);

    size_t item_idx(0);
    boost::optional<navitia::type::ValidityPattern> vp;

    for (auto path_i = path.items.begin(); path_i < path.items.end(); ++path_i) {
        const auto& item = *path_i;

        pbnavitia::Section* pb_section = pb_journey->add_sections();
        pb_section->set_id(pb_creator.register_section(pb_journey, item_idx++));

        if (item.type == ItemType::public_transport) {
            pb_section->set_type(pbnavitia::PUBLIC_TRANSPORT);
            bt::ptime departure_ptime, arrival_ptime;
            type::VehicleJourney const* const vj = item.get_vj();

            if (!vp) {
                vp = *vj->base_validity_pattern();
            } else {
                assert(vp->beginning_date == vj->base_validity_pattern()->beginning_date);
                vp->days &= vj->base_validity_pattern()->days;
            }

            const size_t nb_sps = item.stop_points.size();
            for (size_t i = 0; i < nb_sps; ++i) {
                if (vj->has_boarding() || vj->has_landing()) {
                    continue;
                }
                // skipping estimated stop points
                if (i != 0 && i != nb_sps - 1 && item.stop_times[i]->date_time_estimated()) {
                    continue;
                }

                // do not add a stop_time with pick_up_allowed = false and drop_off_allowed = false
                // these stop_times may also be related to 'deleted' or 'deleted for detour' stop_point
                if (!item.stop_times[i]->pick_up_allowed() && !item.stop_times[i]->drop_off_allowed()) {
                    continue;
                }

                pbnavitia::StopDateTime* stop_time = pb_section->add_stop_date_times();
                auto arr_time = navitia::to_posix_timestamp(item.arrivals[i]);
                stop_time->set_arrival_date_time(arr_time);
                auto dep_time = navitia::to_posix_timestamp(item.departures[i]);
                stop_time->set_departure_date_time(dep_time);

                auto base_st = item.stop_times[i]->get_base_stop_time();
                if (base_st != nullptr) {
                    auto base_dep_dt = get_base_dt(item.stop_times[i], base_st, item.departures[i], true);
                    stop_time->set_base_departure_date_time(navitia::to_posix_timestamp(base_dep_dt));
                    auto base_arr_dt = get_base_dt(item.stop_times[i], base_st, item.arrivals[i], false);
                    stop_time->set_base_arrival_date_time(navitia::to_posix_timestamp(base_arr_dt));
                }

                const auto p_deptime = item.departures[i];
                const auto p_arrtime = item.arrivals[i];
                pb_creator.action_period = bt::time_period(p_deptime, p_arrtime + bt::seconds(1));
                pb_creator.fill(item.stop_points[i], stop_time->mutable_stop_point(), int(depth) - 1);
                pb_creator.fill(item.stop_times[i], stop_time, 1);

                // L'heure de départ du véhicule au premier stop point
                if (departure_ptime.is_not_a_date_time()) {
                    departure_ptime = p_deptime;
                }
                // L'heure d'arrivée au dernier stop point
                arrival_ptime = p_arrtime;
            }
            if (!item.stop_points.empty()) {
                // some time there is only one stop points, typically in case of "extension of services"
                // if the passenger as to board on the last stop_point of a VJ (yes, it's possible...)
                // in this case we want to display this only point as the departure and the destination of this section

                pb_creator.action_period = bt::time_period(item.departures[0], item.arrivals[0] + bt::seconds(1));
                pb_creator.fill(item.stop_points.front(), pb_section->mutable_origin(), 1);
                pb_creator.fill(item.stop_points.back(), pb_section->mutable_destination(), 1);
            }
            pb_creator.action_period = bt::time_period(departure_ptime, arrival_ptime + bt::seconds(1));
            fill_section(pb_creator, pb_section, vj, item.stop_times);

            // setting additional_informations
            const bool has_datetime_estimated =
                !item.stop_times.empty()
                && (item.stop_times.front()->date_time_estimated() || item.stop_times.back()->date_time_estimated());
            const bool has_odt =
                !item.stop_times.empty() && (item.stop_times.front()->odt() || item.stop_times.back()->odt());
            const bool is_zonal =
                !item.stop_points.empty() && (item.stop_points.front()->is_zonal || item.stop_points.back()->is_zonal);
            pb_creator.fill_additional_informations(pb_section->mutable_additional_informations(),
                                                    has_datetime_estimated, has_odt, is_zonal);

            // If this section has estimated stop times,
            // if the previous section is a waiting section, we also
            // want to set it to estimated.
            if (has_datetime_estimated && pb_journey->sections_size() >= 2) {
                auto previous_section = pb_journey->mutable_sections(pb_journey->sections_size() - 2);
                if (previous_section->type() == pbnavitia::WAITING) {
                    previous_section->add_additional_informations(pbnavitia::HAS_DATETIME_ESTIMATED);
                }
            }
        } else {
            pb_section->set_type(pbnavitia::TRANSFER);
            switch (item.type) {
                case ItemType::stay_in: {
                    pb_section->set_transfer_type(pbnavitia::stay_in);
                    // We "stay in" the precedent section, this one is only a transfer
                    int section_idx = pb_journey->sections_size() - 2;
                    if (section_idx >= 0 && pb_journey->sections(section_idx).type() == pbnavitia::PUBLIC_TRANSPORT) {
                        auto* prec_section = pb_journey->mutable_sections(section_idx);
                        prec_section->add_additional_informations(pbnavitia::STAY_IN);
                    }
                    break;
                }
                case ItemType::waiting:
                    pb_section->set_type(pbnavitia::WAITING);
                    break;
                case ItemType::boarding:
                    pb_section->set_type(pbnavitia::boarding);
                    break;
                case ItemType::alighting:
                    pb_section->set_type(pbnavitia::ALIGHTING);
                    break;
                default:
                    pb_section->set_transfer_type(pbnavitia::walking);
                    break;
            }
            // For a waiting section, if the previous public transport section,
            // has estimated datetime we need to set it has estimated too.
            const auto& sections = pb_journey->sections();
            for (auto it = sections.rbegin(); it != sections.rend(); ++it) {
                if (it->type() != pbnavitia::PUBLIC_TRANSPORT) {
                    continue;
                }
                if (boost::count(it->additional_informations(), pbnavitia::HAS_DATETIME_ESTIMATED)) {
                    pb_section->add_additional_informations(pbnavitia::HAS_DATETIME_ESTIMATED);
                }
                break;
            }

            pb_creator.action_period = bt::time_period(item.departure, item.arrival + bt::seconds(1));
            const auto origin_sp = item.stop_points.front();
            const auto destination_sp = item.stop_points.back();
            pb_creator.fill(origin_sp, pb_section->mutable_origin(), 1);
            pb_creator.fill(destination_sp, pb_section->mutable_destination(), 1);
            pb_section->set_length(origin_sp->coord.distance_to(destination_sp->coord));
        }
        uint64_t dep_time, arr_time;
        if (item.stop_points.size() == 1 && item.type == ItemType::public_transport) {
            // after a stay it's possible to have only one point,
            // so we take the arrival hour as arrival and departure
            dep_time = navitia::to_posix_timestamp(item.arrival);
            arr_time = navitia::to_posix_timestamp(item.arrival);
        } else {
            dep_time = navitia::to_posix_timestamp(item.departure);
            arr_time = navitia::to_posix_timestamp(item.arrival);
        }
        pb_section->set_begin_date_time(dep_time);
        pb_section->set_end_date_time(arr_time);

        if (item.type == ItemType::public_transport) {
            auto base_dep_st = item.stop_times.front()->get_base_stop_time();
            if (base_dep_st != nullptr) {
                auto base_dep_dt = get_base_dt(item.stop_times.front(), base_dep_st, item.departure, true);
                pb_section->set_base_begin_date_time(navitia::to_posix_timestamp(base_dep_dt));
            }
            auto base_arr_st = item.stop_times.back()->get_base_stop_time();
            if (base_arr_st != nullptr) {
                auto base_arr_dt = get_base_dt(item.stop_times.back(), base_arr_st, item.arrival, false);
                pb_section->set_base_end_date_time(navitia::to_posix_timestamp(base_arr_dt));
            }
            pb_section->set_realtime_level(
                to_pb_realtime_level(item.stop_times.front()->vehicle_journey->realtime_level));
        }

        arrival_time = item.arrival;
        pb_section->set_duration(arr_time - dep_time);
    }

    if (vp) {
        const auto& vect_p = vptranslator::translate(*vp);
        pb_creator.fill(vect_p, pb_journey->mutable_calendars(), 0);
    }

    compute_most_serious_disruption(pb_journey, pb_creator);

    // fare computation, done at the end for the journey to be complete
    auto before_fare = std::chrono::system_clock::now();
    auto fare = pb_creator.data->fare->compute_fare(path);
    auto after_fare = std::chrono::system_clock::now();
    LOG4CPLUS_DEBUG(
        logger,
        "Fare duration : " << std::chrono::duration_cast<std::chrono::milliseconds>(after_fare - before_fare).count());

    try {
        pb_creator.fill_fare_section(pb_journey, fare);
    } catch (const navitia::exception& e) {
        pb_journey->clear_fare();
        LOG4CPLUS_WARN(logger, "Unable to compute fare, error : " << e.what());
    }
    return arrival_time;
}

static boost::optional<time_duration> get_duration_to_stop_point(const navitia::type::StopPoint* stop_point,
                                                                 const navitia::georef::PathFinder& path_finder) {
    boost::optional<time_duration> duration_to_entry_pt;
    const auto sp_idx = SpIdx(*stop_point);

    utils::make_map_find(path_finder.distance_to_entry_point, sp_idx).if_found([&](const time_duration& duration) {
        duration_to_entry_pt = boost::make_optional(duration);
    });

    return duration_to_entry_pt;
}

void make_pathes(PbCreator& pb_creator,
                 const std::vector<navitia::routing::Path>& paths,
                 georef::StreetNetwork& worker,
                 const georef::Path& direct_path,
                 const type::EntryPoint& origin,
                 const type::EntryPoint& destination,
                 const std::vector<bt::ptime>& datetimes,
                 const bool clockwise,
                 const uint32_t free_radius_from,
                 const uint32_t free_radius_to,
                 const uint32_t depth) {
    pb_creator.set_response_type(pbnavitia::ITINERARY_FOUND);

    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));

    for (const Path& path : paths) {
        bt::ptime arrival_time = bt::pos_infin;
        if (path.items.empty()) {
            continue;
        }
        pbnavitia::Journey* pb_journey = pb_creator.add_journeys();

        /*
         * For the first section, we can distinguish 4 cases
         * 1) We start from an area(stop_area or admin), we will add a crow fly section from the centroid of the area
         *    to the origin stop point of the first section only if the stop point belongs to this area.
         *
         * 2) we start from an area but the chosen stop point doesn't belong to this area, for example we want to start
         * from a city, but the pt part of the journey start in another city, in this case
         * we add a street network section from the centroid of this area to the departure of the first pt_section
         *
         * 3) We start from a ponctual place (everything but stop_area or admin)
         * We add a street network section from this place to the departure of the first pt_section
         *
         * If the uri of the origin point and the uri of the departure of the first section are the
         * same we do nothing
         *
         * 4) 'taxi like' odt. For some case, we don't want a walking section to the the odt stop point
         *     (since the stop point has been artificially created on the data side)
         *     we want a odt section from the departure asked by the user to the destination of the odt)
         **/
        const bool journey_begin_with_address_odt =
            !path.items.front().stop_points.empty() && path.items.front().stop_points.front()->is_zonal;
        const bool journey_end_with_address_odt =
            !path.items.back().stop_points.empty() && path.items.back().stop_points.back()->is_zonal;

        if (journey_begin_with_address_odt) {
            // no crow fly section, but we'll have to update the start of the journey
            // first is zonal ODT, we do nothing, there is no walking
        } else if (!path.items.empty() && !path.items.front().stop_points.empty()) {
            const auto& departure_stop_point = path.items.front().stop_points.front();
            georef::Path sn_departure_path = worker.get_path(departure_stop_point->idx);

            auto duration_to_departure = get_duration_to_stop_point(departure_stop_point, worker.departure_path_finder);

            if (is_same_stop_point(origin, *departure_stop_point)) {
                // nothing in this case
            } else if (use_crow_fly(origin, *departure_stop_point, sn_departure_path, *pb_creator.data,
                                    free_radius_from, duration_to_departure)) {
                type::EntryPoint destination_tmp(type::Type_e::StopPoint, departure_stop_point->uri);
                destination_tmp.coordinates = departure_stop_point->coord;
                pb_creator.action_period = bt::time_period(path.items.front().departures.front(),
                                                           path.items.front().departures.front() + bt::seconds(1));
                time_duration crow_fly_duration = duration_to_departure ? *duration_to_departure : time_duration();
                auto seconds_to_departure = pt::seconds(crow_fly_duration.to_posix().total_seconds());
                auto departure_time = path.items.front().departures.front() - seconds_to_departure;
                pb_creator.fill_crowfly_section(origin, destination_tmp, crow_fly_duration,
                                                worker.departure_path_finder.mode, departure_time, pb_journey);
            } else if (!sn_departure_path.path_items.empty()) {
                // because of projection problem, the walking path might not join
                // exactly the routing one
                nt::GeographicalCoord routing_first_coord = departure_stop_point->coord;
                if (sn_departure_path.path_items.back().coordinates.back() != routing_first_coord) {
                    // if it's the case, we artificially add the missing segment
                    sn_departure_path.path_items.back().coordinates.push_back(routing_first_coord);
                }

                // todo: better way to round this: fractionnal second are evil
                const auto walking_time = sn_departure_path.duration;
                auto departure_time =
                    path.items.front().departure - pt::seconds(walking_time.to_posix().total_seconds());
                pb_creator.fill_street_sections(origin, sn_departure_path, pb_journey, departure_time);
                if (pb_journey->sections_size() > 0) {
                    auto first_section = pb_journey->mutable_sections(0);
                    auto last_section = pb_journey->mutable_sections(pb_journey->sections_size() - 1);
                    pb_creator.action_period =
                        bt::time_period(navitia::from_posix_timestamp(first_section->begin_date_time()),
                                        navitia::from_posix_timestamp(last_section->end_date_time() + bt::seconds(1)));
                    // We add coherence with the origin of the request
                    pb_creator.fill(&origin, first_section->mutable_origin(), 2);
                    // We add coherence with the first pt section
                    last_section->mutable_destination()->Clear();
                    pb_creator.fill(departure_stop_point, last_section->mutable_destination(), 2);
                }
            }
        }
        arrival_time = handle_pt_sections(pb_journey, pb_creator, path, depth);
        // for 'taxi like' odt, we want to start from the address, not the 1 stop point
        if (journey_begin_with_address_odt) {
            auto* section = pb_journey->mutable_sections(0);
            section->mutable_origin()->Clear();
            pb_creator.action_period =
                bt::time_period(navitia::from_posix_timestamp(section->begin_date_time()), bt::seconds(1));
            pb_creator.fill(&origin, section->mutable_origin(), 1);
        }

        if (journey_end_with_address_odt) {
            // last is 'taxi like' ODT, there is no walking nor crow fly section,
            // but we have to update the end of the journey
            auto* section = pb_journey->mutable_sections(pb_journey->sections_size() - 1);
            section->mutable_destination()->Clear();
            // TODO: the period can probably be better (-1 min shift)
            pb_creator.action_period =
                bt::time_period(navitia::from_posix_timestamp(section->end_date_time()), bt::seconds(1));
            pb_creator.fill(&destination, section->mutable_destination(), 1);
        } else if (!path.items.empty() && !path.items.back().stop_points.empty()) {
            const auto arrival_stop_point = path.items.back().stop_points.back();
            georef::Path sn_arrival_path = worker.get_path(arrival_stop_point->idx, true);

            auto duration_to_arrival = get_duration_to_stop_point(arrival_stop_point, worker.arrival_path_finder);

            if (is_same_stop_point(destination, *arrival_stop_point)) {
                // nothing in this case
            } else if (use_crow_fly(destination, *arrival_stop_point, sn_arrival_path, *pb_creator.data, free_radius_to,
                                    duration_to_arrival)) {
                type::EntryPoint origin_tmp(type::Type_e::StopPoint, arrival_stop_point->uri);
                auto dt = path.items.back().arrivals.back();
                origin_tmp.coordinates = arrival_stop_point->coord;
                pb_creator.action_period = bt::time_period(dt, bt::seconds(1));
                time_duration crow_fly_duration = duration_to_arrival ? *duration_to_arrival : time_duration();
                arrival_time = arrival_time + pt::seconds(crow_fly_duration.to_posix().total_seconds());
                pb_creator.fill_crowfly_section(origin_tmp, destination, crow_fly_duration,
                                                worker.arrival_path_finder.mode, dt, pb_journey);
            }
            // for stop areas, we don't want to display the fallback section if start
            // from one of the stop area's stop point
            else if (!sn_arrival_path.path_items.empty()) {
                // add a junction between the routing path and the walking one if needed
                nt::GeographicalCoord routing_last_coord = arrival_stop_point->coord;
                if (sn_arrival_path.path_items.front().coordinates.front() != routing_last_coord) {
                    sn_arrival_path.path_items.front().coordinates.push_front(routing_last_coord);
                }

                auto begin_section_time = arrival_time;
                int nb_section = pb_journey->mutable_sections()->size();
                pb_creator.fill_street_sections(destination, sn_arrival_path, pb_journey, begin_section_time);
                arrival_time = arrival_time + pt::seconds(sn_arrival_path.duration.to_posix().total_seconds());
                if (pb_journey->mutable_sections()->size() > nb_section) {
                    // We add coherence between the destination of the PT part of the journey
                    // and the origin of the street network part
                    auto section = pb_journey->mutable_sections(nb_section);
                    pb_creator.action_period =
                        bt::time_period(navitia::from_posix_timestamp(section->begin_date_time()),
                                        navitia::from_posix_timestamp(section->end_date_time() + 1));
                }

                // We add coherence with the destination object of the request
                auto section = pb_journey->mutable_sections(pb_journey->mutable_sections()->size() - 1);
                pb_creator.action_period = bt::time_period(navitia::from_posix_timestamp(section->begin_date_time()),
                                                           navitia::from_posix_timestamp(section->end_date_time() + 1));
                pb_creator.fill(&destination, section->mutable_destination(), 2);
            }
        }

        const auto departure = pb_journey->sections(0).begin_date_time();
        const auto arrival = pb_journey->sections(pb_journey->sections_size() - 1).end_date_time();
        pb_journey->set_departure_date_time(departure);
        pb_journey->set_arrival_date_time(arrival);
        pb_journey->set_duration(arrival - departure);
        co2_emission_aggregator(pb_journey);
        compute_metadata(pb_journey);
    }

    add_direct_path(pb_creator, direct_path, origin, destination, datetimes, clockwise);
}

static void add_pt_pathes(PbCreator& pb_creator,
                          const std::vector<navitia::routing::Path>& paths,
                          const uint32_t depth) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    for (const Path& path : paths) {
        // TODO: what do we want to do in this case?
        if (path.items.empty()) {
            continue;
        }
        bt::ptime departure_time = path.items.front().departures.front();
        pbnavitia::Journey* pb_journey = pb_creator.add_journeys();
        bt::ptime arrival_time = handle_pt_sections(pb_journey, pb_creator, path, depth);

        pb_journey->set_departure_date_time(navitia::to_posix_timestamp(departure_time));
        pb_journey->set_arrival_date_time(navitia::to_posix_timestamp(arrival_time));
        pb_journey->set_duration((arrival_time - departure_time).total_seconds());

        co2_emission_aggregator(pb_journey);
        compute_metadata(pb_journey);
    }
}

static void make_pt_pathes(PbCreator& pb_creator,
                           const std::vector<navitia::routing::Path>& paths,
                           const uint32_t depth) {
    pb_creator.set_response_type(pbnavitia::ITINERARY_FOUND);
    add_pt_pathes(pb_creator, paths, depth);
}

static void add_isochrone_response(RAPTOR& raptor,
                                   const type::EntryPoint& origin,
                                   PbCreator& pb_creator,
                                   const std::vector<type::StopPoint*> stop_points,
                                   bool clockwise,
                                   DateTime init_dt,
                                   DateTime bound,
                                   int max_duration) {
    pbnavitia::PtObject pb_origin;
    pb_creator.fill(&origin, &pb_origin, 0);
    for (const type::StopPoint* sp : stop_points) {
        SpIdx sp_idx(*sp);
        const auto best_lbl = raptor.best_labels[sp_idx].dt_pt;
        if ((clockwise && best_lbl < bound) || (!clockwise && best_lbl > bound)) {
            int round = raptor.best_round(sp_idx);
            const auto& best_round_label = raptor.labels[round][sp_idx];

            if (round == -1 || !is_dt_initialized(best_round_label.dt_pt)) {
                continue;
            }

            // get absolute value
            int duration = best_lbl > init_dt ? best_lbl - init_dt : init_dt - best_lbl;

            if (duration > max_duration) {
                continue;
            }
            auto pb_journey = pb_creator.add_journeys();

            uint64_t departure;
            uint64_t arrival;
            // Note: since there is no 2nd pass for the isochrone, the departure dt
            // is the requested dt (or the arrival dt for non clockwise)
            if (clockwise) {
                departure = to_posix_timestamp(init_dt, raptor.data);
                arrival = to_posix_timestamp(best_lbl, raptor.data);
            } else {
                departure = to_posix_timestamp(best_lbl, raptor.data);
                arrival = to_posix_timestamp(init_dt, raptor.data);
            }
            const auto str_requested = to_posix_timestamp(init_dt, raptor.data);
            pb_journey->set_arrival_date_time(arrival);
            pb_journey->set_departure_date_time(departure);
            pb_journey->set_requested_date_time(str_requested);
            pb_journey->set_duration(duration);
            pb_journey->set_nb_transfers(round - 1);
            pb_creator.action_period = bt::time_period(navitia::to_posix_time(best_lbl - duration, raptor.data),
                                                       navitia::to_posix_time(best_lbl, raptor.data) + bt::seconds(1));
            if (clockwise) {
                *pb_journey->mutable_origin() = pb_origin;
                pb_creator.fill(sp, pb_journey->mutable_destination(), 0);
            } else {
                pb_creator.fill(sp, pb_journey->mutable_origin(), 0);
                *pb_journey->mutable_destination() = pb_origin;
            }
        }
    }
}

namespace {
template <typename T>
const std::vector<georef::Admin*>& get_admins(const std::string& uri,
                                              const std::unordered_map<std::string, T*>& obj_map) {
    if (auto obj = find_or_default(uri, obj_map)) {
        return obj->admin_list;
    }
    static const std::vector<georef::Admin*> empty_list;
    return empty_list;
}
}  // namespace

static std::vector<georef::Admin*> find_admins(const type::EntryPoint& ep, const type::Data& data) {
    if (ep.type == type::Type_e::StopArea) {
        return get_admins(ep.uri, data.pt_data->stop_areas_map);
    }
    if (ep.type == type::Type_e::StopPoint) {
        return get_admins(ep.uri, data.pt_data->stop_points_map);
    }
    if (ep.type == type::Type_e::Admin) {
        auto it_admin = data.geo_ref->admin_map.find(ep.uri);
        if (it_admin == data.geo_ref->admin_map.end()) {
            return {};
        }
        const auto admin = data.geo_ref->admins[it_admin->second];
        return {admin};
    }
    if (ep.type == type::Type_e::POI) {
        auto it_poi = data.geo_ref->poi_map.find(ep.uri);
        if (it_poi == data.geo_ref->poi_map.end()) {
            return {};
        }
        return it_poi->second->admin_list;
    }
    // else we look for the coordinate's admin
    return data.geo_ref->find_admins(ep.coordinates);
}

static void add_free_stop_point(const type::StopPoint* stop_point,
                                georef::PathFinder& pf,
                                routing::map_stop_point_duration& results) {
    const SpIdx sp_idx{*stop_point};
    utils::make_map_find(results, sp_idx).if_not_found([&]() {
        pf.distance_to_entry_point[sp_idx] = {};
        results[sp_idx] = {};
    });
}

boost::optional<routing::map_stop_point_duration> get_stop_points(const type::EntryPoint& ep,
                                                                  const type::Data& data,
                                                                  georef::StreetNetwork& worker,
                                                                  const uint32_t free_radius,
                                                                  bool use_second) {
    routing::map_stop_point_duration result;
    georef::PathFinder& concerned_path_finder = use_second ? worker.arrival_path_finder : worker.departure_path_finder;
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    LOG4CPLUS_TRACE(logger, "Searching nearest stop_point's from entry point : [" << ep.coordinates.lat() << ","
                                                                                  << ep.coordinates.lon() << "]");

    switch (ep.type) {
        case type::Type_e::StopArea: {
            utils::make_map_find(data.pt_data->stop_areas_map, ep.uri).if_found([&](const type::StopArea* sa) {
                for (auto stop_point : sa->stop_point_list) {
                    add_free_stop_point(stop_point, concerned_path_finder, result);
                }
            });
            break;
        }
        case type::Type_e::StopPoint: {
            utils::make_map_find(data.pt_data->stop_points_map, ep.uri)
                .if_found([&](const type::StopPoint* stop_point) {
                    add_free_stop_point(stop_point, concerned_path_finder, result);
                });
            break;
        }
        case type::Type_e::Admin: {
            // For an admin, we want to leave from it's main stop areas if we have some,
            // else we'll leave from the center of the admin
            auto it_admin = data.geo_ref->admin_map.find(ep.uri);
            if (it_admin == data.geo_ref->admin_map.end()) {
                LOG4CPLUS_WARN(logger, "impossible to find admin " << ep.uri);
                return result;
            }
            const auto admin = data.geo_ref->admins[it_admin->second];

            for (auto stop_area : admin->main_stop_areas) {
                for (auto stop_point : stop_area->stop_point_list) {
                    add_free_stop_point(stop_point, concerned_path_finder, result);
                }
            }
            break;
        }
        case type::Type_e::Address:
        case type::Type_e::Coord:
        case type::Type_e::POI:
            break;
        default:
            return boost::none;
    }

    // TODO ODT NTFSv0.3: remove that when we stop to support NTFSv0.1
    // we need to check if the admin has zone odt
    const auto& admins = find_admins(ep, data);
    for (const auto* admin : admins) {
        for (const auto* odt_admin_stop_point : admin->odt_stop_points) {
            add_free_stop_point(odt_admin_stop_point, concerned_path_finder, result);
        }
    }

    // checking for zonal stop points
    const auto& zonal_sps = data.pt_data->get_stop_points_by_area(ep.coordinates);
    for (const auto* stop_point : zonal_sps) {
        add_free_stop_point(stop_point, concerned_path_finder, result);
    }

    // Filtering with free radius (free_radius in meters)
    // We set the SP time duration to 0, if SP are inside
    free_radius_filter(result, concerned_path_finder, ep, data, free_radius);

    // We add the center of the admin, and look for the stop points around
    auto nearest = worker.find_nearest_stop_points(ep.streetnetwork_params.max_duration,
                                                   data.pt_data->stop_point_proximity_list, use_second);
    for (const auto& elt : nearest) {
        const SpIdx sp_idx{elt.first};
        if (result.find(sp_idx) == result.end()) {
            result[sp_idx] = elt.second;
        }
    }
    LOG4CPLUS_DEBUG(logger, result.size() << " sp found for admin");

    return result;
}

static routing::map_stop_point_duration make_map_stop_point_duration(
    const type::EntryPoints& entryPointList,
    const std::unordered_map<std::string, type::StopPoint*>& raptor_stop_points_map) {
    routing::map_stop_point_duration results;
    for (const auto& entryPoint : entryPointList) {
        utils::make_map_find(raptor_stop_points_map, entryPoint.uri)
            .if_found(
                [&](const type::StopPoint* sp) { results[SpIdx{*sp}] = navitia::seconds(entryPoint.access_duration); })
            .if_not_found([&]() {
                // for now we throw, maybe we should ignore them
                throw navitia::recoverable_exception("stop_point " + entryPoint.uri + " not found");
            });
    }
    return results;
}

static const boost::optional<routing::map_stop_point_duration> get_stop_points_if_not_already_done(
    const navitia::type::EntryPoint& center,
    const navitia::type::Data& data,
    navitia::georef::StreetNetwork& worker,
    const boost::optional<const type::EntryPoints&>& stop_points) {
    // If stop_points have already been computed, we don't need to do it here again.
    if (stop_points) {
        return make_map_stop_point_duration(*stop_points, data.pt_data->stop_points_map);
    }

    worker.init(center);
    return get_stop_points(center, data, worker);
}

void free_radius_filter(routing::map_stop_point_duration& sp_list,
                        georef::PathFinder& path_finder,
                        const type::EntryPoint& ep,
                        const type::Data& data,
                        const uint32_t free_radius) {
    if (free_radius > 0) {
        auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));

        // Find stop point list with a free radius constraint
        LOG4CPLUS_DEBUG(logger, "filtering with free radius (" << free_radius << " meters)");
        auto excluded_elements = data.pt_data->stop_point_proximity_list.find_within(ep.coordinates, free_radius);
        LOG4CPLUS_DEBUG(logger, "find " << excluded_elements.size() << " stop points in free radius");

        // For each excluded stop point
        for (const auto& excluded_sp : excluded_elements) {
            const SpIdx sp_idx{excluded_sp.first};
            sp_list[sp_idx] = {};
            path_finder.distance_to_entry_point[sp_idx] = {};
            LOG4CPLUS_TRACE(logger, "free radius, sp idx : " << sp_idx << " , duration is set to 0");
        }
    }
}

DateTime prepare_next_call_for_raptor(const RAPTOR::Journeys& journeys, const bool clockwise) {
    if (clockwise) {
        // among the journeys with the earliest arrival time, we compute the latest departure time
        DateTime earliest_arrival = DateTimeUtils::inf;
        DateTime latest_departure = DateTimeUtils::min;
        for (const auto& journey : journeys) {
            if (journey.arrival_dt < earliest_arrival) {
                earliest_arrival = journey.arrival_dt;
                latest_departure = journey.departure_dt;
            } else if (journey.arrival_dt == earliest_arrival && journey.departure_dt > latest_departure) {
                latest_departure = journey.departure_dt;
            }
        }

        return latest_departure + 1;
    }

    else {
        // among the journeys with the latest departure time, we compute the earliest arrival time
        DateTime earliest_arrival = DateTimeUtils::inf;
        DateTime latest_departure = DateTimeUtils::min;
        for (const auto& journey : journeys) {
            if (journey.departure_dt > latest_departure) {
                latest_departure = journey.departure_dt;
                earliest_arrival = journey.arrival_dt;
            } else if (journey.departure_dt == latest_departure && journey.arrival_dt < earliest_arrival) {
                earliest_arrival = journey.arrival_dt;
            }
        }

        return earliest_arrival - 1;
    }
}

static std::vector<bt::ptime> parse_datetimes(const RAPTOR& raptor,
                                              const std::vector<uint64_t>& timestamps,
                                              navitia::PbCreator& pb_creator,
                                              bool clockwise) {
    std::vector<bt::ptime> datetimes;

    for (uint64_t datetime : timestamps) {
        bt::ptime ptime = bt::from_time_t(datetime);
        if (!raptor.data.meta->production_date.contains(ptime.date())) {
            pb_creator.fill_pb_error(pbnavitia::Error::date_out_of_bounds, pbnavitia::DATE_OUT_OF_BOUNDS,
                                     "date is not in data production period");
        }
        datetimes.push_back(ptime);
    }
    if (clockwise) {
        std::sort(datetimes.begin(), datetimes.end(), [](bt::ptime dt1, bt::ptime dt2) { return dt1 > dt2; });
    } else {
        std::sort(datetimes.begin(), datetimes.end());
    }

    return datetimes;
}

void make_pt_response(navitia::PbCreator& pb_creator,
                      RAPTOR& raptor,
                      const type::EntryPoints& origins,
                      const type::EntryPoints& destinations,
                      const uint64_t timestamp,
                      const bool clockwise,
                      const type::AccessibiliteParams& accessibilite_params,
                      const std::vector<std::string>& forbidden,
                      const std::vector<std::string>& allowed,
                      const type::RTLevel rt_level,
                      const navitia::time_duration& arrival_transfer_penalty,
                      const navitia::time_duration& walking_transfer_penalty,
                      const uint32_t max_duration,
                      const uint32_t max_transfers,
                      const uint32_t max_extra_second_pass,
                      const boost::optional<navitia::time_duration>& direct_path_duration,
                      const boost::optional<uint32_t>& min_nb_journeys,
                      const double night_bus_filter_max_factor,
                      const int32_t night_bus_filter_base_factor,
                      const boost::optional<DateTime>& timeframe_duration,
                      const uint32_t depth,
                      const boost::optional<boost::posix_time::ptime>& current_datetime) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));

    // Create datetime
    auto datetimes = parse_datetimes(raptor, {timestamp}, pb_creator, clockwise);
    if (pb_creator.has_error() || pb_creator.has_response_type(pbnavitia::DATE_OUT_OF_BOUNDS)) {
        return;
    }

    // Get stop points for departure and destination
    auto departures = make_map_stop_point_duration(origins, raptor.data.pt_data->stop_points_map);
    auto arrivals = make_map_stop_point_duration(destinations, raptor.data.pt_data->stop_points_map);

    // Call Raptor loop
    const auto pathes =
        call_raptor(pb_creator, raptor, departures, arrivals, datetimes, rt_level, arrival_transfer_penalty,
                    walking_transfer_penalty, accessibilite_params, forbidden, allowed, clockwise, direct_path_duration,
                    min_nb_journeys,
                    // nb_direct_path = 0 for distributed if direct_path_duration is none
                    direct_path_duration ? 1 : 0, max_duration, max_transfers, max_extra_second_pass,
                    night_bus_filter_max_factor, night_bus_filter_base_factor, timeframe_duration, current_datetime);

    // Create pb response
    make_pt_pathes(pb_creator, pathes, depth);

    // Add error field
    if (pb_creator.empty_journeys()) {
        pb_creator.fill_pb_error(pbnavitia::Error::no_solution, pbnavitia::NO_SOLUTION,
                                 "no solution found for this journey");
    }
}

void filter_direct_path(RAPTOR::Journeys& journeys) {
    journeys.remove_if([](const Journey& j) { return !j.is_pt(); });
}

/**
    Check if a journey is way later than another journey

    Then, we check for each journey the difference between the
    requested datetime and the arrival datetime (the other way around
    for non clockwise)

    requested dt
    *
                    |=============>
                          journey2

                                            |=============>
                                                 journey1

    |-----------------------------|
       journey2 pseudo duration

    |------------------------------------------------------|
            journey1 pseudo duration
 */
bool is_way_later(const Journey& j1, const Journey& j2, const NightBusFilter::Params& params) {
    auto& requested_dt = params.requested_datetime;
    auto& clockwise = params.clockwise;

    auto get_pseudo_duration = [&](const Journey& j) {
        auto dt = clockwise ? j.arrival_dt : j.departure_dt;
        return std::abs(double(dt) - double(requested_dt));
    };

    auto j1_pseudo_duration = get_pseudo_duration(j1);
    auto j2_pseudo_duration = get_pseudo_duration(j2);

    auto max_pseudo_duration = j2_pseudo_duration * params.max_factor + params.base_factor;

    return j1_pseudo_duration > max_pseudo_duration;
}

void filter_late_journeys(RAPTOR::Journeys& journeys, const NightBusFilter::Params& params) {
    if (journeys.empty()) {
        return;
    }

    const auto& best = get_pseudo_best_journey(journeys, params.clockwise);

    auto it = journeys.begin();
    while (it != journeys.end()) {
        const auto& journey = *it;
        if (best != journey && is_way_later(journey, best, params)) {
            it = journeys.erase(it);
            continue;
        }

        ++it;
    }
}

// - if `clockwise` : remove everything in the section *after* the *first* occurence of `stop_area_uri`, unless this
// stop is served by `vj_to_skip`
//
// returns `true` if the section has been modified
bool shorten_section_clockwise(navitia::routing::Journey::Section& section,
                               const std::string& last_stop_area_uri,
                               const navitia::type::VehicleJourney* last_vj,
                               const map_stop_point_duration& fallbacks) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));

    // because of stay-ins, we may have several vj in one section, we have to scan the stop times of all vjs
    auto order = section.get_in_st->order();
    for (const auto* vj = section.get_in_st->vehicle_journey; vj; (vj = vj->next_vj, order = type::RankStopTime(0))) {
        if (vj == last_vj) {
            return false;
        }

        if (section.get_in_dt < section.get_in_st->boarding_time) {
            LOG4CPLUS_ERROR(logger, "Error : section.get_in_dt < section.get_in_st->boarding_time");
            return false;
        }
        // determine midnigth of the day at which the vj is used
        // WARNING : this base_dt may need to be increased in case there is some stay-ins vehicle
        // with stop_times that are past midnight (not handled yet)
        navitia::DateTime base_dt = section.get_in_dt - section.get_in_st->boarding_time;
        for (const auto& st :
             boost::make_iterator_range(vj->stop_time_list.begin() + order.val, vj->stop_time_list.end())) {
            // if the end of original section is reached: no shortening on that section
            if (&st == section.get_out_st) {
                return false;
            }
            // check stop_area is the same and it's possible to use stop_point to get out of PT
            if (st.stop_point->stop_area->uri == last_stop_area_uri
                && navitia::contains(fallbacks, routing::SpIdx{st.stop_point->idx}) && st.drop_off_allowed()) {
                section.get_out_st = &st;
                section.get_out_dt = base_dt + st.alighting_time;

                return true;
            }
        }
    }
    LOG4CPLUS_ERROR(logger, "Error occurred when modifying backtracking journeys clockwise");
    return false;
}

// - if `! clockwise` : remove everything in the section *before* the *last* occurence of `stop_area_uri`, unless this
// stop is served by `vj_to_skip`
//
// returns `true` if the section has been modified
bool shorten_section_anticlockwise(navitia::routing::Journey::Section& section,
                                   const std::string& first_stop_area_uri,
                                   const navitia::type::VehicleJourney* first_vj,
                                   const map_stop_point_duration& fallbacks) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));

    // because of stay-ins, we may have several vj in one section, we have to scan the stop times of all vjs
    auto order = type::RankStopTime(section.get_out_st->vehicle_journey->stop_time_list.size() - 1
                                    - section.get_out_st->order().val);
    for (const auto* vj = section.get_out_st->vehicle_journey; vj; (vj = vj->prev_vj, order = type::RankStopTime(0))) {
        if (vj == first_vj) {
            return false;
        }

        if (section.get_in_dt < section.get_in_st->boarding_time) {
            LOG4CPLUS_ERROR(logger, "Error : section.get_in_dt < section.get_in_st->boarding_time");
            return false;
        }
        // determine midnigth of the day at which the vj is used
        // WARNING : this base_dt may need to be increased in case there is some stay-ins vehicle
        // with stop_times that are past midnight (not handled yet)
        navitia::DateTime base_dt = section.get_in_dt - section.get_in_st->boarding_time;
        for (const auto& st :
             boost::make_iterator_range(vj->stop_time_list.rbegin() + order.val, vj->stop_time_list.rend())) {
            // if the start of original section is reached: no shortening on that section
            if (&st == section.get_in_st) {
                return false;
            }
            // check stop_area is the same and it's possible to use stop_point to get in PT
            if (st.stop_point->stop_area->uri == first_stop_area_uri
                && navitia::contains(fallbacks, routing::SpIdx{st.stop_point->idx}) && st.pick_up_allowed()) {
                section.get_in_st = &st;
                section.get_in_dt = base_dt + st.boarding_time;

                return true;
            }
        }
    }

    LOG4CPLUS_ERROR(logger, "Error occurred when modifying backtracking journeys anticlockwise");
    return false;
}

void modify_backtracking_journeys(RAPTOR::Journeys& journeys,
                                  const map_stop_point_duration& departures,
                                  const map_stop_point_duration& destinations,
                                  const bool clockwise) {
    for (auto& journey : journeys) {
        if (clockwise) {
            const auto& last_section = journey.sections.rbegin();
            const auto& last_stop_area_uri = last_section->get_out_st->stop_point->stop_area->uri;
            const auto* last_vj = last_section->get_out_st->vehicle_journey;

            size_t section_idx = 0;
            // browse sections and modify any section that allows shortening
            for (; section_idx < journey.sections.size(); section_idx++) {
                if (shorten_section_clockwise(journey.sections[section_idx], last_stop_area_uri, last_vj,
                                              destinations)) {
                    break;
                }
            }
            // remove useless sections after the one allowing shortening
            if (section_idx + 1 < journey.sections.size()) {
                journey.sections.erase(journey.sections.begin() + section_idx + 1, journey.sections.end());
            }
        } else {
            const auto& first_section = journey.sections.begin();
            const auto& first_stop_area_uri = first_section->get_in_st->stop_point->stop_area->uri;
            const auto* first_vj = first_section->get_in_st->vehicle_journey;

            int section_idx = journey.sections.size() - 1;
            // browse sections and modify any section that allows shortening
            for (; section_idx >= 0; section_idx--) {
                if (shorten_section_anticlockwise(journey.sections[section_idx], first_stop_area_uri, first_vj,
                                                  departures)) {
                    break;
                }
            }
            // remove useless sections before the one allowing shortening
            if (section_idx > 0) {
                journey.sections.erase(journey.sections.begin(), journey.sections.begin() + section_idx);
            }
        }
    }
}

void make_response(navitia::PbCreator& pb_creator,
                   RAPTOR& raptor,
                   const type::EntryPoint& origin,
                   const type::EntryPoint& destination,
                   const std::vector<uint64_t>& timestamps,
                   const bool clockwise,
                   const type::AccessibiliteParams& accessibilite_params,
                   const std::vector<std::string>& forbidden,
                   const std::vector<std::string>& allowed,
                   georef::StreetNetwork& worker,
                   const type::RTLevel rt_level,
                   const navitia::time_duration& arrival_transfer_penalty,
                   const navitia::time_duration& walking_transfer_penalty,
                   const uint32_t max_duration,
                   const uint32_t max_transfers,
                   const uint32_t max_extra_second_pass,
                   const uint32_t free_radius_from,
                   const uint32_t free_radius_to,
                   const boost::optional<uint32_t>& min_nb_journeys,
                   const double night_bus_filter_max_factor,
                   const int32_t night_bus_filter_base_factor,
                   const boost::optional<uint32_t>& timeframe_duration,
                   const uint32_t depth,
                   const boost::optional<boost::posix_time::ptime>& current_datetime) {
    // Create datetime
    auto datetimes = parse_datetimes(raptor, timestamps, pb_creator, clockwise);
    if (pb_creator.has_error() || pb_creator.has_response_type(pbnavitia::DATE_OUT_OF_BOUNDS)) {
        return;
    }

    // Initialize street network
    worker.init(origin, {destination});

    // Get stop points for departure and destination
    const auto departures = get_stop_points(origin, raptor.data, worker, free_radius_from);
    const auto destinations = get_stop_points(destination, raptor.data, worker, free_radius_to, true);

    // case 1 : departure no exist
    if (!departures) {
        pb_creator.fill_pb_error(pbnavitia::Error::unknown_object, "The entry point: " + origin.uri + " is not valid");
        return;
    }

    // case 2 : destination no exist
    if (!destinations) {
        pb_creator.fill_pb_error(pbnavitia::Error::unknown_object,
                                 "The entry point: " + destination.uri + " is not valid");
        return;
    }

    // case 3 : departure or destination are emtpy
    if (departures->empty() || destinations->empty()) {
        make_pathes(pb_creator, std::vector<Path>(), worker, get_direct_path(worker, origin, destination), origin,
                    destination, datetimes, clockwise, depth);

        if (pb_creator.empty_journeys()) {
            if (departures->empty() && destinations->empty()) {
                pb_creator.fill_pb_error(pbnavitia::Error::no_origin_nor_destination,
                                         pbnavitia::NO_ORIGIN_NOR_DESTINATION_POINT,
                                         "Public transport is not reachable from origin nor destination");
            } else if (departures->empty()) {
                pb_creator.fill_pb_error(pbnavitia::Error::no_origin, pbnavitia::NO_ORIGIN_POINT,
                                         "Public transport is not reachable from origin");
            } else if (destinations->empty()) {
                pb_creator.fill_pb_error(pbnavitia::Error::no_destination, pbnavitia::NO_DESTINATION_POINT,
                                         "Public transport is not reachable from destination");
            }
        }
        return;
    }

    // classical case : We have departures and destinations

    // Compute direct path
    using OptTimeDur = boost::optional<navitia::time_duration>;
    const auto direct_path = get_direct_path(worker, origin, destination);
    const OptTimeDur direct_path_dur =
        direct_path.path_items.empty() ? OptTimeDur()
                                       : OptTimeDur(direct_path.duration / origin.streetnetwork_params.speed_factor);
    const uint32_t nb_direct_path = direct_path.path_items.empty() ? 0 : 1;

    // Call Raptor loop
    const auto pathes =
        call_raptor(pb_creator, raptor, *departures, *destinations, datetimes, rt_level, arrival_transfer_penalty,
                    walking_transfer_penalty, accessibilite_params, forbidden, allowed, clockwise, direct_path_dur,
                    min_nb_journeys, nb_direct_path, max_duration, max_transfers, max_extra_second_pass,
                    night_bus_filter_max_factor, night_bus_filter_base_factor, timeframe_duration, current_datetime);

    // Create pb response
    make_pathes(pb_creator, pathes, worker, direct_path, origin, destination, datetimes, clockwise, free_radius_from,
                free_radius_to, depth);

    // Add error field
    if (pb_creator.empty_journeys()) {
        pb_creator.fill_pb_error(pbnavitia::Error::no_solution, pbnavitia::NO_SOLUTION,
                                 "no solution found for this journey");
    }
}

struct IsochroneCommon {
    bool clockwise{};
    type::GeographicalCoord coord_origin;
    map_stop_point_duration departures;
    DateTime init_dt{};
    type::EntryPoint center;
    DateTime bound{};
    bt::ptime datetime;
    IsochroneCommon() = default;
    IsochroneCommon(const bool clockwise,
                    const type::GeographicalCoord& coord_origin,
                    map_stop_point_duration departures,
                    const DateTime init_dt,
                    type::EntryPoint center,
                    const DateTime bound,
                    bt::ptime datetime)
        : clockwise(clockwise),
          coord_origin(coord_origin),
          departures(std::move(departures)),
          init_dt(init_dt),
          center(std::move(center)),
          bound(bound),
          datetime(datetime) {}
};

static const boost::optional<IsochroneCommon> make_isochrone_common(
    RAPTOR& raptor,
    const type::EntryPoint& center,
    const uint64_t departure_datetime,
    const double max_duration,
    const uint32_t max_transfers,
    const type::AccessibiliteParams& accessibilite_params,
    const std::vector<std::string>& forbidden,
    const std::vector<std::string>& allowed,
    bool clockwise,
    const nt::RTLevel rt_level,
    georef::StreetNetwork& worker,
    PbCreator& pb_creator,
    const boost::optional<const type::EntryPoints&>& stop_points) {
    const auto tmp_datetime = parse_datetimes(raptor, {departure_datetime}, pb_creator, clockwise);
    if (pb_creator.has_error() || tmp_datetime.empty() || pb_creator.has_response_type(pbnavitia::DATE_OUT_OF_BOUNDS)) {
        return boost::optional<IsochroneCommon>{};
    }

    const auto departures = get_stop_points_if_not_already_done(center, raptor.data, worker, stop_points);
    if (!departures) {
        pb_creator.fill_pb_error(pbnavitia::Error::unknown_object, "The entry point: " + center.uri + " is not valid");
        return boost::optional<IsochroneCommon>{};
    }

    const auto datetime = tmp_datetime.front();
    const auto init_dt = to_datetime(datetime, raptor.data);
    const auto bound = build_bound(clockwise, max_duration, init_dt);
    raptor.isochrone(*departures, init_dt, bound, max_transfers, accessibilite_params, forbidden, allowed, clockwise,
                     rt_level);
    return IsochroneCommon(clockwise, center.coordinates, *departures, init_dt, center, bound, datetime);
}

void make_isochrone(navitia::PbCreator& pb_creator,
                    RAPTOR& raptor,
                    const type::EntryPoint& center,
                    const uint64_t datetime_timestamp,
                    const bool clockwise,
                    const type::AccessibiliteParams& accessibilite_params,
                    const std::vector<std::string>& forbidden,
                    const std::vector<std::string>& allowed,
                    georef::StreetNetwork& worker,
                    const type::RTLevel rt_level,
                    const int max_duration,
                    const uint32_t max_transfers,
                    const boost::optional<const type::EntryPoints&>& stop_points) {
    const auto isochrone_common =
        make_isochrone_common(raptor, center, datetime_timestamp, max_duration, max_transfers, accessibilite_params,
                              forbidden, allowed, clockwise, rt_level, worker, pb_creator, stop_points);

    if (!isochrone_common) {
        return;
    }

    add_isochrone_response(raptor, center, pb_creator, raptor.data.pt_data->stop_points, clockwise,
                           isochrone_common->init_dt, isochrone_common->bound, max_duration);
    pb_creator.sort_journeys();
    if (pb_creator.empty_journeys()) {
        pb_creator.fill_pb_error(pbnavitia::Error::no_solution, pbnavitia::NO_SOLUTION,
                                 "no solution found for this isochrone");
    }
}

static void print_coord(std::stringstream& ss, const type::GeographicalCoord coord) {
    ss << std::setprecision(15) << "[" << coord.lon() << "," << coord.lat() << "]";
}

static void print_polygon(std::stringstream& os, const type::Polygon& polygon) {
    os << "[[";
    separated_by_coma(os, print_coord, polygon.outer());
    os << "]";
    for (const auto& inner : polygon.inners()) {
        os << ",[";
        separated_by_coma(os, print_coord, inner);
        os << "]";
    }
    os << "]";
}

template <typename T>
static void add_common_isochrone(PbCreator& pb_creator,
                                 type::EntryPoint center,
                                 bool clockwise,
                                 bt::ptime datetime,
                                 T pb) {
    if (clockwise) {
        pb_creator.fill(&center, pb->mutable_origin(), 2);
    } else {
        pb_creator.fill(&center, pb->mutable_destination(), 2);
    }
    pb->set_requested_date_time(navitia::to_posix_timestamp(datetime));
}

static void add_graphical_isochrone(const type::MultiPolygon& shape,
                                    const int& min_duration,
                                    const int& max_duration,
                                    PbCreator& pb_creator,
                                    type::EntryPoint center,
                                    bool clockwise,
                                    bt::ptime datetime,
                                    const type::Data& d,
                                    DateTime min_date_time,
                                    DateTime max_date_time) {
    std::stringstream geojson;
    geojson << R"({"type":"MultiPolygon","coordinates":[)";
    separated_by_coma(geojson, print_polygon, shape);
    geojson << "]}";
    auto pb_isochrone = pb_creator.add_graphical_isochrones();
    pb_isochrone->mutable_geojson();
    pb_isochrone->set_geojson(geojson.str());
    pb_isochrone->set_min_duration(min_duration);
    pb_isochrone->set_max_duration(max_duration);
    pb_isochrone->set_min_date_time(navitia::to_posix_timestamp(min_date_time, d));
    pb_isochrone->set_max_date_time(navitia::to_posix_timestamp(max_date_time, d));
    add_common_isochrone(pb_creator, std::move(center), clockwise, datetime, pb_isochrone);
}

static void add_heat_map(const std::string& heat_map,
                         PbCreator& pb_creator,
                         type::EntryPoint center,
                         bool clockwise,
                         bt::ptime datetime) {
    auto pb_heat_map = pb_creator.add_heat_maps();
    pb_heat_map->mutable_heat_matrix();
    pb_heat_map->set_heat_matrix(heat_map);
    add_common_isochrone(pb_creator, std::move(center), clockwise, datetime, pb_heat_map);
}

static DateTime make_isochrone_date(const DateTime& init_dt, const DateTime& offset, const bool clockwise) {
    return init_dt + (clockwise ? 1 : -1) * offset;
}

void make_graphical_isochrone(navitia::PbCreator& pb_creator,
                              RAPTOR& raptor,
                              const type::EntryPoint& center,
                              const uint64_t departure_datetime,
                              const std::vector<DateTime>& boundary_duration,
                              const uint32_t max_transfers,
                              const type::AccessibiliteParams& accessibilite_params,
                              const std::vector<std::string>& forbidden,
                              const std::vector<std::string>& allowed,
                              const bool clockwise,
                              const nt::RTLevel rt_level,
                              georef::StreetNetwork& worker,
                              const double& speed,
                              const boost::optional<const type::EntryPoints&>& stop_points) {
    const auto isochrone_common = make_isochrone_common(raptor, center, departure_datetime, boundary_duration[0],
                                                        max_transfers, accessibilite_params, forbidden, allowed,
                                                        clockwise, rt_level, worker, pb_creator, stop_points);

    if (!isochrone_common) {
        return;
    }

    std::vector<Isochrone> isochrone =
        build_isochrones(raptor, isochrone_common->clockwise, isochrone_common->coord_origin,
                         isochrone_common->departures, speed, boundary_duration, isochrone_common->init_dt);
    for (const auto& iso : isochrone) {
        auto min_date_time = make_isochrone_date(isochrone_common->init_dt, iso.min_duration, clockwise);
        auto max_date_time = make_isochrone_date(isochrone_common->init_dt, iso.max_duration, clockwise);
        add_graphical_isochrone(iso.shape, iso.min_duration, iso.max_duration, pb_creator, center, clockwise,
                                isochrone_common->datetime, raptor.data, min_date_time, max_date_time);
    }
}

void make_heat_map(navitia::PbCreator& pb_creator,
                   RAPTOR& raptor,
                   const type::EntryPoint& center,
                   const uint64_t departure_datetime,
                   const DateTime max_duration,
                   const uint32_t max_transfers,
                   const type::AccessibiliteParams& accessibilite_params,
                   const std::vector<std::string>& forbidden,
                   const std::vector<std::string>& allowed,
                   const bool clockwise,
                   const nt::RTLevel rt_level,
                   georef::StreetNetwork& worker,
                   const double& end_speed,
                   const navitia::type::Mode_e end_mode,
                   const uint32_t resolution,
                   const boost::optional<const type::EntryPoints&>& stop_points) {
    const auto isochrone_common =
        make_isochrone_common(raptor, center, departure_datetime, max_duration, max_transfers, accessibilite_params,
                              forbidden, allowed, clockwise, rt_level, worker, pb_creator, stop_points);
    if (!isochrone_common) {
        return;
    }

    if (worker.geo_ref.nb_vertex_by_mode == 0) {
        // if we have no street network we cannot compute a heatmap
        pb_creator.fill_pb_error(pbnavitia::Error::no_solution, pbnavitia::NO_SOLUTION,
                                 "no street network data, impossible to compute a heat_map");
        return;
    }

    auto heat_map = build_raster_isochrone(worker.geo_ref, end_speed, end_mode, isochrone_common->init_dt, raptor,
                                           isochrone_common->coord_origin, max_duration, clockwise,
                                           isochrone_common->bound, resolution);
    add_heat_map(heat_map, pb_creator, center, clockwise, isochrone_common->datetime);
}

}  // namespace routing
}  // namespace navitia
