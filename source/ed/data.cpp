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

#include "data.h"

#include "type/datetime.h"
#include "utils/exception.h"
#include "utils/functions.h"
#include "utils/timer.h"

#include <boost/geometry.hpp>
#include <boost/geometry/geometry.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/max_element.hpp>

#include <iostream>

namespace nt = navitia::type;
namespace ed {

void Data::sort() {
#define SORT_AND_INDEX(type_name, collection_name)                     \
    std::sort(collection_name.begin(), collection_name.end(), Less()); \
    std::for_each(collection_name.begin(), collection_name.end(), Indexer<nt::idx_t>());
    ITERATE_NAVITIA_PT_TYPES(SORT_AND_INDEX)

    std::for_each(shapes_from_prev.begin(), shapes_from_prev.end(), Indexer<nt::idx_t>());
    std::sort(stops.begin(), stops.end(), Less());
}

void Data::add_feed_info(const std::string& key, const std::string& value) {
    auto logger = log4cplus::Logger::getInstance("log");
    std::string lower_key = boost::algorithm::to_lower_copy(key);
    if (this->feed_infos.find(lower_key) != this->feed_infos.end()) {
        this->feed_infos[lower_key] = value;
    } else {
        LOG4CPLUS_INFO(logger, "add_feed_info, Key :" << key << " Value :" << value << " not imported.");
    }
}

void Data::normalize_uri() {
    ::ed::normalize_uri(networks);
    ::ed::normalize_uri(companies);
    ::ed::normalize_uri(commercial_modes);
    ::ed::normalize_uri(lines);
    ::ed::normalize_uri(line_groups);
    ::ed::normalize_uri(physical_modes);
    ::ed::normalize_uri(stop_areas);
    ::ed::normalize_uri(stop_points);
    ::ed::normalize_uri(vehicle_journeys);
    ::ed::normalize_uri(validity_patterns);
    ::ed::normalize_uri(calendars);
    ::ed::normalize_uri(access_points);
}

void Data::build_block_id() {
    /// We want to group vehicle journeys by their block_id
    /// Two vehicle_journeys with the same block_id vj1 are consecutive if
    /// the last arrival_time of vj1 <= to the departure_time of vj2
    std::sort(vehicle_journeys.begin(), vehicle_journeys.end(),
              [this](const types::VehicleJourney* vj1, const types::VehicleJourney* vj2) {
                  if (vj1->block_id != vj2->block_id) {
                      return vj1->block_id < vj2->block_id;
                  }
                  auto offset1 = tz_wrapper.tz_handler.get_utc_offset(*vj1->validity_pattern);
                  auto offset2 = tz_wrapper.tz_handler.get_utc_offset(*vj2->validity_pattern);

                  // we don't want to link the splited vjs
                  if (offset1 != offset2) {
                      return offset1 < offset2;
                  }
                  if (vj1->stop_time_list.empty() || vj2->stop_time_list.empty()) {
                      return vj1->stop_time_list.size() < vj2->stop_time_list.size();
                  }
                  return vj1->stop_time_list.front()->departure_time < vj2->stop_time_list.front()->departure_time;
              });

    types::VehicleJourney* prev_vj = nullptr;
    for (auto* vj : vehicle_journeys) {
        if (prev_vj && !prev_vj->block_id.empty() && prev_vj->block_id == vj->block_id) {
            // NOTE: we do nothing for vj with empty stop times, they will be removed in the clean()
            if (!vj->stop_time_list.empty() && !prev_vj->stop_time_list.empty()) {
                // Sanity check
                // If the departure time of the 1st stoptime of vj is greater
                // then the arrivaltime of the last stop time of prev_vj
                // there is a time travel, and we don't like it!
                // This is not supposed to happen
                // @TODO: Add a parameter to avoid too long connection
                // they can be for instance due to bad data
                auto* vj_first_st = vj->stop_time_list.front();
                auto* prev_vj_last_st = prev_vj->stop_time_list.back();
                if (vj_first_st->departure_time >= prev_vj_last_st->arrival_time) {
                    if (vj->joins_on_different_stop_points(*prev_vj)
                        && prev_vj_last_st->departure_time > vj_first_st->arrival_time) {
                        LOG4CPLUS_ERROR(log4cplus::Logger::getInstance("log"),
                                        "Stay-in on different stop points with overlapping stop_times. "
                                        "Stay-in cannot be done between vjs '"
                                            << prev_vj->uri << "' and '" << vj->uri << "'");
                        break;
                    }

                    // we add another check that the vjs are on the same offset (that they are not the from vj split on
                    // different dst)
                    if (tz_wrapper.tz_handler.get_utc_offset(*vj->validity_pattern)
                        == tz_wrapper.tz_handler.get_utc_offset(*prev_vj->validity_pattern)) {
                        prev_vj->next_vj = vj;
                        vj->prev_vj = prev_vj;
                    }
                }
            }
        }
        prev_vj = vj;
    }
}

static std::map<std::string, std::set<std::string>> make_departure_destinations_map(
    const std::vector<types::StopPointConnection*>& stop_point_connections) {
    std::map<std::string, std::set<std::string>> res;
    for (auto conn : stop_point_connections) {
        res[conn->departure->uri].insert(conn->destination->uri);
    }
    return res;
}

static std::map<std::string, std::vector<types::StopPoint*>> make_stop_area_stop_points_map(
    const std::vector<ed::types::StopPoint*>& stop_points) {
    std::map<std::string, std::vector<types::StopPoint*>> res;
    for (auto sp : stop_points) {
        if (sp->stop_area) {
            res[sp->stop_area->uri].push_back(sp);
        }
    }
    return res;
}

types::ValidityPattern* Data::get_or_create_validity_pattern(const types::ValidityPattern& vp) {
    auto find_vp_predicate = [&vp](types::ValidityPattern* vp2) { return vp.days == vp2->days; };
    auto it = boost::find_if(validity_patterns, find_vp_predicate);
    if (it != validity_patterns.end()) {
        return *(it);
    }
    validity_patterns.push_back(new types::ValidityPattern(vp));
    return validity_patterns.back();
}

// Please not that VP is not in the list of validity_patterns
void Data::shift_vp_left(types::ValidityPattern& vp) {
    if (vp.check(0)) {
        // This should be done only once because few lines below we shift
        // every validity_pattern on the right, so every one has its first day
        // deactivated
        auto one_day = boost::gregorian::days(1);
        auto begin_date = meta.production_date.begin() - one_day;
        auto end_date = meta.production_date.end();
        if (end_date - begin_date > boost::gregorian::days(366)) {
            end_date = begin_date + boost::gregorian::days(365);
        }
        meta.production_date = {begin_date, end_date};
        for (auto& vp_ : validity_patterns) {
            // The first day is not active.
            vp_->days <<= 1;
            vp_->beginning_date = begin_date;
        }
        vp.beginning_date = begin_date;
        vp.days <<= 1;

        // we also need to shift the timezone dst period
        tz_wrapper.build_tz(meta.production_date);
    }
    vp.days >>= 1;
}

void Data::shift_stop_times() {
    for (auto vj : vehicle_journeys) {
        if (vj->stop_time_list.empty()) {
            continue;
        }

        // For non-frequency vj, we must have the first stop time in
        // [0; 24:00[ (since they are ordered, every stop time will be
        // greatter than 0)
        // Depending on boarding and alighting duration the start_time will be either the arrival_time or
        // the boarding time.
        //
        // For frequency vj, the start time must be in [0; 24:00[.
        int start_time;
        if (vj->is_frequency()) {
            start_time = vj->start_time;
        } else {
            start_time = vj->earliest_time();
        }

        // number of days to shift for start_time in [0; 24:00[
        int shift = (start_time >= 0 ? 0 : 1) - start_time / int(navitia::DateTimeUtils::SECONDS_PER_DAY);

        if (shift != 0) {
            auto vp = types::ValidityPattern(*vj->validity_pattern);
            switch (shift) {
                case 1:
                    shift_vp_left(vp);
                    break;
                case -1:
                    vp.days <<= 1;
                    break;
                default: {
                    std::string err_msg = "Ed: You have shift vj " + vj->name + " by " + std::to_string(shift) + " days,"
                               " that's more than one day";
                    LOG4CPLUS_FATAL(log4cplus::Logger::getInstance("log"), err_msg);
                    throw navitia::exception(err_msg);
                }
            }
            vj->validity_pattern = get_or_create_validity_pattern(vp);
            for (auto st : vj->stop_time_list) {
                st->shift_times(shift);
            }
        }

        if (vj->is_frequency()) {
            // shifting start_time in [0; 24:00[
            vj->start_time += shift * int(navitia::DateTimeUtils::SECONDS_PER_DAY);
            // vj->end_time must be gretter than 0 (but it can be lesser than start_time)
            while (vj->end_time < 0) {
                vj->end_time += int(navitia::DateTimeUtils::SECONDS_PER_DAY);
            }
        }
    }
}

static bool compare(const std::pair<ed::types::StopArea*, size_t>& p1,
                    const std::pair<ed::types::StopArea*, size_t>& p2) {
    return p1.second < p2.second;
}

void Data::build_route_destination() {
    std::unordered_map<ed::types::Route*, std::map<ed::types::StopArea*, size_t>> destinations;
    for (const auto* vj : vehicle_journeys) {
        if (!vj->route || vj->stop_time_list.empty()) {
            continue;
        }
        if (vj->route->destination) {
            continue;
        }  // we have a destination, don't create one
        if (!vj->stop_time_list.back()->stop_point || !vj->stop_time_list.back()->stop_point->stop_area) {
            continue;
        }
        ++destinations[vj->route][vj->stop_time_list.back()->stop_point->stop_area];
    }
    for (const auto& map_route : destinations) {
        if (map_route.second.empty()) {
            continue;
        }  // we never know
        const auto max = boost::max_element(map_route.second, compare);
        map_route.first->destination = max->first;
    }
}

void Data::complete() {
    build_block_id();
    auto start = boost::posix_time::microsec_clock::local_time();
    build_shape_from_prev();
    LOG4CPLUS_INFO(log4cplus::Logger::getInstance("log"),
                   "build_shape_from_prev took "
                       << (boost::posix_time::microsec_clock::local_time() - start).total_milliseconds() << " ms");
    pick_up_drop_of_on_borders();

    build_grid_validity_pattern();
    build_associated_calendar();

    shift_stop_times();
    finalize_frequency();

    ::ed::normalize_uri(routes);

    // set StopPoint from old zonal ODT to is_zonal
    for (const auto* vj : vehicle_journeys) {
        using nt::VehicleJourneyType;
        if (navitia::contains({VehicleJourneyType::adress_to_stop_point, VehicleJourneyType::odt_point_to_point},
                              vj->vehicle_journey_type)) {
            for (const auto* st : vj->stop_time_list) {
                st->stop_point->is_zonal = true;
            }
        }
    }

    if (!stop_point_connections.empty()) {
        LOG4CPLUS_INFO(log4cplus::Logger::getInstance("log"),
                       "Some connections exists in the input data. I'll not generate any new connections.");
        return;
    }
    // generates default connections inside each stop area
    LOG4CPLUS_INFO(
        log4cplus::Logger::getInstance("log"),
        "No connection exists in the input data. I'll generate connections between all stop points in a stop area.");
    auto connections = make_departure_destinations_map(stop_point_connections);
    const auto sa_sps = make_stop_area_stop_points_map(stop_points);
    const auto connections_size = stop_point_connections.size();
    for (const types::StopArea* sa : stop_areas) {
        const auto& sps = find_or_default(sa->uri, sa_sps);
        for (const auto& sp1 : sps) {
            for (const auto& sp2 : sps) {
                // if the connection exists, do nothing
                if (find_or_default(sp1->uri, connections).count(sp2->uri) != 0) {
                    continue;
                }

                const int conn_dur_itself = 0;

                const double distance = sp1->coord.distance_to(sp2->coord) * 1.414;  // distance * sqrt(2), in meters
                const double duration_double = distance / 1.1;  // meters / (meters/seconds) = seconds
                const int conn_dur_other = static_cast<int>(duration_double);
                const int min_waiting_dur = 120;
                stop_point_connections.push_back(new types::StopPointConnection());
                auto new_conn = stop_point_connections.back();
                new_conn->departure = sp1;
                new_conn->destination = sp2;
                new_conn->connection_kind = types::ConnectionType::StopArea;
                new_conn->display_duration = sp1->uri == sp2->uri ? conn_dur_itself : conn_dur_other;
                new_conn->duration = new_conn->display_duration + min_waiting_dur;
                new_conn->uri = sp1->uri + "=>" + sp2->uri;
                connections[new_conn->departure->uri].insert(new_conn->destination->uri);
            }
        }
    }
    LOG4CPLUS_INFO(log4cplus::Logger::getInstance("log"),
                   stop_point_connections.size() - connections_size << " connections added");
}

void Data::clean() {
    auto logger = log4cplus::Logger::getInstance("log");
    std::set<std::string> toErase;
    int erase_emptiness = 0, erase_no_circulation = 0, erase_invalid_stoptimes = 0;

    for (auto* vj : vehicle_journeys) {
        if (vj_to_erase.count(vj)) {
            toErase.insert(vj->uri);
            continue;
        }
        if (vj->stop_time_list.empty()) {
            toErase.insert(vj->uri);
            ++erase_emptiness;
            continue;
        }
        if (vj->validity_pattern->days.none() && vj->adapted_validity_pattern->days.none()) {
            toErase.insert(vj->uri);
            ++erase_no_circulation;
            continue;
        }
        // we check that no stop times are negatives
        const auto st_is_invalid = [](const types::StopTime* st) {
            return st->departure_time < 0 || st->arrival_time < 0 || st->boarding_time < 0 || st->alighting_time < 0;
        };
        if (std::any_of(vj->stop_time_list.begin(), vj->stop_time_list.end(), st_is_invalid)) {
            toErase.insert(vj->uri);
            ++erase_invalid_stoptimes;
        }
    }

    std::vector<size_t> erasest;

    for (int i = stops.size() - 1; i >= 0; --i) {
        auto it = toErase.find(stops[i]->vehicle_journey->uri);
        if (it != toErase.end()) {
            erasest.push_back(i);
        }
    }

    // For each stop_time to remove, we delete and put the last element of the vector in it's place
    // We avoid resizing the vector until completition for performance reasons.
    size_t num_elements = stops.size();
    for (size_t to_erase : erasest) {
        remove_reference_to_object(stops[to_erase]);
        delete stops[to_erase];
        stops[to_erase] = stops[num_elements - 1];
        num_elements--;
    }

    stops.resize(num_elements);

    erasest.clear();
    for (int i = vehicle_journeys.size() - 1; i >= 0; --i) {
        auto it = toErase.find(vehicle_journeys[i]->uri);
        if (it != toErase.end()) {
            erasest.push_back(i);
        }
    }

    // The same but now with vehicle_journey's
    num_elements = vehicle_journeys.size();
    for (size_t to_erase : erasest) {
        auto vj = vehicle_journeys[to_erase];
        if (vj->next_vj) {
            vj->next_vj->prev_vj = nullptr;
        }
        if (vj->prev_vj) {
            vj->prev_vj->next_vj = nullptr;
        }
        // we need to remove the vj from the meta vj
        auto& metavj = meta_vj_map[vj->meta_vj_name];
        bool found = false;
        for (auto it = metavj.theoric_vj.begin(); it != metavj.theoric_vj.end(); ++it) {
            if (*it == vj) {
                metavj.theoric_vj.erase(it);
                found = true;
                break;
            }
        }
        if (!found) {
            throw navitia::exception("construction problem, impossible to find the vj " + vj->uri + " in the meta vj "
                                     + vj->meta_vj_name);
        }
        if (metavj.theoric_vj.empty()) {
            // we remove the meta vj
            meta_vj_map.erase(vj->meta_vj_name);
        }

        remove_reference_to_object(vj);
        delete vj;
        vehicle_journeys[to_erase] = vehicle_journeys[num_elements - 1];
        num_elements--;
    }
    vehicle_journeys.resize(num_elements);

    if (erase_emptiness || erase_no_circulation || erase_invalid_stoptimes) {
        LOG4CPLUS_INFO(logger, "Data::clean(): "
                                   << erase_emptiness << " because they do not contain any clean stop_times, "
                                   << erase_no_circulation << " because they are never valid "
                                   << " and " << erase_invalid_stoptimes << " because the stop times were negatives");
    }
    // Delete duplicate connections
    // Connections are sorted by departure,destination
    auto sort_function = [](types::StopPointConnection* spc1, types::StopPointConnection* spc2) {
        return spc1->uri < spc2->uri || (spc1->uri == spc2->uri && spc1 < spc2);
    };

    auto unique_function = [](types::StopPointConnection* spc1, types::StopPointConnection* spc2) {
        return spc1->uri == spc2->uri;
    };

    std::sort(stop_point_connections.begin(), stop_point_connections.end(), sort_function);
    num_elements = stop_point_connections.size();
    auto it_end = std::unique(stop_point_connections.begin(), stop_point_connections.end(), unique_function);
    //@TODO : Attention, it's leaking, it should succeed in erasing objects
    // Ce qu'il y a dans la fin du vecteur apres unique n'est pas garanti, on ne peut pas itérer sur la suite pour
    // effacer
    stop_point_connections.resize(std::distance(stop_point_connections.begin(), it_end));
    LOG4CPLUS_INFO(logger, num_elements - stop_point_connections.size()
                               << " stop point connections deleted because of duplicate connections");
}

using LineStringIter = nt::LineString::const_iterator;
using LineStringIterPair = std::pair<LineStringIter, LineStringIter>;

// Returns the nearest segment or point from coord under the form of a
// pair of iterators {it1, it2}.  If it is a segment, it1 + 1 == it2.
// If it is a point, it1 == it2.  If line is empty, it1 == it2 == line.end()
static LineStringIterPair get_nearest(const nt::GeographicalCoord& coord, const nt::LineString& line) {
    if (line.empty()) {
        return {line.end(), line.end()};
    }
    auto nearest = std::make_pair(line.begin(), line.begin());
    auto nearest_dist = coord.project(*nearest.first, *nearest.second).second;
    for (auto it1 = line.begin(), it2 = it1 + 1; it2 != line.end(); ++it1, ++it2) {
        auto projection = coord.project(*it1, *it2);
        if (nearest_dist <= projection.second) {
            continue;
        }

        nearest_dist = projection.second;
        if (projection.first == *it1) {
            nearest = {it1, it1};
        } else if (projection.first == *it2) {
            nearest = {it2, it2};
        } else {
            nearest = {it1, it2};
        }
    }
    return nearest;
}

// Returns nearest projected point in a path
static nt::GeographicalCoord get_nearest_projection(const nt::GeographicalCoord& coord, const nt::LineString& line) {
    if (line.empty()) {
        return coord;
    }
    nt::GeographicalCoord projected_point = *line.begin();
    auto nearest_dist = float(coord.distance_to(projected_point));
    for (auto it1 = line.begin(), it2 = it1 + 1; it2 != line.end(); ++it1, ++it2) {
        auto projection = coord.project(*it1, *it2);
        if (nearest_dist <= projection.second) {
            continue;
        }

        nearest_dist = projection.second;
        projected_point = projection.first;
    }
    return projected_point;
}

static size_t abs_distance(LineStringIterPair pair) {
    return pair.first < pair.second ? pair.second - pair.first : pair.first - pair.second;
}

static LineStringIterPair get_smallest_range(const LineStringIterPair& p1, const LineStringIterPair& p2) {
    using P = LineStringIterPair;
    P res = {p1.first, p2.first};
    for (P p : {P(p1.first, p2.second), P(p1.second, p2.first), P(p1.second, p2.second)}) {
        if (abs_distance(res) > abs_distance(p)) {
            res = p;
        }
    }
    return res;
}

nt::LineString create_shape(const nt::GeographicalCoord& from,
                            const nt::GeographicalCoord& to,
                            const nt::LineString& shape,
                            const double simplify_tolerance) {
    const auto nearest_from = get_nearest(from, shape);
    const auto nearest_to = get_nearest(to, shape);
    const auto p_from = get_nearest_projection(from, shape);
    const auto p_to = get_nearest_projection(to, shape);

    if (nearest_from == nearest_to) {
        return {from, to};
    }

    nt::LineString res;
    const auto range = get_smallest_range(nearest_from, nearest_to);

    res.push_back(p_from);
    if (range.first < range.second) {
        for (auto it = range.first; it <= range.second; ++it) {
            res.push_back(*it);
        }
    } else {
        for (auto it = range.first; it >= range.second; --it) {
            res.push_back(*it);
        }
    }
    res.push_back(p_to);

    // simplification
    nt::LineString simplified;
    boost::geometry::simplify(res, simplified, simplify_tolerance);

    return simplified;
}

void Data::build_shape_from_prev() {
    std::map<std::string, std::shared_ptr<types::Shape>> shape_cache;
    for (types::VehicleJourney* vj : vehicle_journeys) {
        const types::StopPoint* prev_stop_point = nullptr;
        for (types::StopTime* stop_time : vj->stop_time_list) {
            if (prev_stop_point) {
                const auto& shape = find_or_default(vj->shape_id, shapes);
                if (!shape.empty()) {
                    std::string key =
                        (boost::format("%s|%s|%s") % vj->shape_id % prev_stop_point->uri % stop_time->stop_point->uri)
                            .str();

                    // we keep in cache the resulting geometry, so we don't have to re compute it later
                    if (shape_cache.find(key) == shape_cache.end()) {
                        auto s = std::make_shared<types::Shape>(create_shape(
                            prev_stop_point->coord, stop_time->stop_point->coord, shape.front(), simplify_tolerance));
                        this->shapes_from_prev.push_back(s);
                        shape_cache[key] = s;
                    }
                    stop_time->shape_from_prev = shape_cache[key];
                }
            }
            prev_stop_point = stop_time->stop_point;
        }
    }
}

void Data::pick_up_drop_of_on_borders() {
    for (auto* vj : vehicle_journeys) {
        if (vj->stop_time_list.empty()) {
            continue;
        }

        /*
         * The first arrival and last departure of a vehicle-journey don't make sense as they can't be used.
         * So let's forbid them
         *
         * This is true unless the vehicle-journey has a stay-in on different stop points as described below
         */

        auto* first_st = vj->stop_time_list.front();
        auto* last_st = vj->stop_time_list.back();

        /*
         * In the example below we are assuming that VJ:1 and VJ:2 are sharing the same block_id.
         *
         * The stay-in section is allowed with 2 configurations:
         *  - when two VJ are joined on 2 different stop points with consecutive stop times (example 1)
         *  - when two VJ share the same stop point with similar stop times (example 2)
         *
         *      Example 1:
         *      ----------
         *                                            (1)  (2)
         *              out          in   out         in   out         in   out         in
         *               X    SP1    |    ▲    SP2    |    ▲    SP3    |    ▲   SP4     X
         *               X           ▼    |           ▼    |           |    |           X
         *  (prev) VJ:1   08:00-09:00      10:00-11:00     |           ▼    |           X
         *  (next) VJ:2                                     12:00-13:00      14:00-15:00
         *                                |---------- Stay In ---------|
         *
         *       Example 1 is the only case were we allow specific pick-up and drop-off
         *
         *
         *      Example 2:
         *      ----------
         *              out          in   out         in
         *               X    SP1    |    ▲    SP2    X
         *               X           ▼    |           X
         *  (prev) VJ:1   08:00-09:00      10:00-11:00
         *  (next) VJ:2                    10:00-11:00      14:00-15:00
         *                                X           ▲    |           X
         *                                X           |    ▼   SP3     X
         *                                out         in   out         in
         *                                |- Stay-In -|
         */

        if (!vj->next_vj or vj->ends_with_stayin_on_same_stop_point()) {
            last_st->pick_up_allowed = false;
        }

        if (!vj->prev_vj or vj->starts_with_stayin_on_same_stop_point()) {
            first_st->drop_off_allowed = false;
        }
    }
}

static types::ValidityPattern get_union_validity_pattern(const types::MetaVehicleJourney* meta_vj) {
    types::ValidityPattern validity;

    for (auto* vj : meta_vj->theoric_vj) {
        if (validity.beginning_date.is_not_a_date()) {
            validity.beginning_date = vj->validity_pattern->beginning_date;
        } else {
            if (validity.beginning_date != vj->validity_pattern->beginning_date) {
                throw navitia::exception("the beginning date of the meta_vj are not all the same");
            }
        }
        validity.days |= vj->validity_pattern->days;
    }
    return validity;
}

using list_cal_bitset = std::vector<std::pair<const types::Calendar*, types::ValidityPattern::year_bitset>>;

list_cal_bitset Data::find_matching_calendar(const std::string& name,
                                             const types::ValidityPattern& validity_pattern,
                                             const std::vector<types::Calendar*>& calendar_list,
                                             double relative_threshold) {
    list_cal_bitset res;
    // for the moment we keep lot's of trace, but they will be removed after a while
    auto log = log4cplus::Logger::getInstance("kraken::type::Data::Calendar");
    LOG4CPLUS_TRACE(log, "meta vj " << name << " :" << validity_pattern.days.to_string());

    for (const auto calendar : calendar_list) {
        // sometimes a calendar can be empty (for example if it's validity period does not
        // intersect the data's validity period)
        // we do not filter those calendar since it's a user input, but we do not match them
        if (!calendar->validity_pattern.days.any()) {
            continue;
        }
        auto diff = get_difference(calendar->validity_pattern.days, validity_pattern.days);
        size_t nb_diff = diff.count();

        LOG4CPLUS_TRACE(log, "cal " << calendar->uri << " :" << calendar->validity_pattern.days.to_string());

        // we associate the calendar to the vj if the diff are below a relative threshold
        // compared to the number of active days in the calendar
        size_t threshold = std::round(relative_threshold * calendar->validity_pattern.days.count());
        LOG4CPLUS_TRACE(log, "**** diff: " << nb_diff << " and threshold: " << threshold
                                           << (nb_diff <= threshold ? ", we keep it!!" : ""));

        if (nb_diff > threshold) {
            continue;
        }
        res.push_back({calendar, diff});
    }

    return res;
}

void Data::build_grid_validity_pattern() {
    for (types::Calendar* cal : this->calendars) {
        cal->validity_pattern.beginning_date = meta.production_date.begin();
        cal->build_validity_pattern(meta.production_date);
    }
}

void Data::build_associated_calendar() {
    auto log = log4cplus::Logger::getInstance("kraken::type::Data");
    std::multimap<types::ValidityPattern, types::AssociatedCalendar*> associated_vp;
    size_t nb_not_matched_vj(0);
    size_t nb_matched(0);
    size_t nb_ignored(0);

    for (auto& meta_vj_pair : meta_vj_map) {
        auto& meta_vj = meta_vj_pair.second;

        assert(!meta_vj.theoric_vj.empty());

        if (!meta_vj.associated_calendars.empty()) {
            LOG4CPLUS_TRACE(log, "The meta_vj " << meta_vj_pair.first << " was already linked to a grid_calendar");
            nb_ignored++;
            continue;
        }

        // we check the theoric vj of a meta vj
        // because we start from the postulate that the theoric VJs are the same VJ
        // split because of dst (day saving time)
        // because of that we try to match the calendar with the union of all theoric vj validity pattern
        types::ValidityPattern meta_vj_validity_pattern = get_union_validity_pattern(&meta_vj);

        // some check can be done on any theoric vj, we do them on the first
        auto* first_vj = meta_vj.theoric_vj.front();

        // some check can be done on any theoric vj, we do them on the first
        std::vector<types::Calendar*> calendar_list;

        for (types::Calendar* calendar : this->calendars) {
            for (types::Line* line : calendar->line_list) {
                if (line->uri == first_vj->route->line->uri) {
                    calendar_list.push_back(calendar);
                }
            }
        }

        if (calendar_list.empty()) {
            LOG4CPLUS_TRACE(log, "the line of the vj " << first_vj->uri << " is associated to no grid_calendar");
            nb_not_matched_vj++;
            continue;
        }

        // we check if we already computed the associated val for this validity pattern
        // since a validity pattern can be shared by many vj
        auto it = associated_vp.find(meta_vj_validity_pattern);
        if (it != associated_vp.end()) {
            for (; it->first == meta_vj_validity_pattern; ++it) {
                meta_vj.associated_calendars.insert({it->second->calendar->uri, it->second});
            }
            continue;
        }

        auto close_cal = find_matching_calendar(meta_vj_pair.first, meta_vj_validity_pattern, calendar_list);

        if (close_cal.empty()) {
            LOG4CPLUS_TRACE(log, "the meta vj " << meta_vj_pair.first << " has been attached to no grid_calendar");
            nb_not_matched_vj++;
            continue;
        }
        nb_matched++;

        std::stringstream cal_uri;
        for (auto cal_bit_set : close_cal) {
            auto associated_calendar = new types::AssociatedCalendar();
            associated_calendar->idx = this->associated_calendars.size();
            this->associated_calendars.push_back(associated_calendar);

            associated_calendar->calendar = cal_bit_set.first;
            // we need to create the associated exceptions
            for (size_t i = 0; i < cal_bit_set.second.size(); ++i) {
                if (!cal_bit_set.second[i]) {
                    continue;  // cal_bit_set.second is the resulting differences, so 0 means no exception
                }
                navitia::type::ExceptionDate ex;
                ex.date = meta_vj_validity_pattern.beginning_date + boost::gregorian::days(i);
                // if the vj is active this day it's an addition, else a removal
                ex.type = (meta_vj_validity_pattern.days[i] ? navitia::type::ExceptionDate::ExceptionType::add
                                                            : navitia::type::ExceptionDate::ExceptionType::sub);
                associated_calendar->exceptions.push_back(ex);
            }

            meta_vj.associated_calendars.insert({associated_calendar->calendar->uri, associated_calendar});
            associated_vp.insert({meta_vj_validity_pattern, associated_calendar});
            cal_uri << associated_calendar->calendar->uri << " ";
        }

        LOG4CPLUS_DEBUG(log, "the meta vj " << meta_vj_pair.first << " has been attached to " << cal_uri.str());
    }

    LOG4CPLUS_INFO(log, nb_matched << " vehicle journeys have been matched to at least one grid_calendar");
    if (nb_not_matched_vj) {
        LOG4CPLUS_WARN(log, "no grid_calendar found for " << nb_not_matched_vj << " vehicle journey");
    }
    if (nb_ignored) {
        LOG4CPLUS_INFO(log, nb_ignored << " vehicle journey were already linked and therefore ignored");
    }
}

// For frequency based trips, make arrival and departure time relative from first stop.
void Data::finalize_frequency() {
    for (auto* vj : this->vehicle_journeys) {
        if (!vj->stop_time_list.empty() && vj->stop_time_list.front()->is_frequency) {
            int begin = vj->earliest_time();
            if (begin == 0) {
                continue;  // Time is already relative to 0
            }
            for (auto* st : vj->stop_time_list) {
                st->arrival_time -= begin;
                st->departure_time -= begin;
                st->alighting_time -= begin;
                st->boarding_time -= begin;
            }
        }
    }
}

/*
 * we need the list of dst periods over the years of the validity period
 *
 *                                  validity period
 *                              [-----------------------]
 *                        2013                                  2014
 *       <------------------------------------><-------------------------------------->
 *[           non DST   )[  DST    )[        non DST     )[   DST     )[     non DST          )
 *
 * We thus create a partition of the time with all period with the UTC offset
 *
 *       [    +7h       )[  +8h    )[       +7h          )[   +8h     )[      +7h     )
 */
std::vector<EdTZWrapper::PeriodWithUtcShift> EdTZWrapper::get_dst_periods(
    const boost::gregorian::date_period& validity_period) const {
    if (validity_period.is_null()) {
        return {};
    }
    std::vector<int> years;
    // we want to have all the overlapping year
    // so add each time 1 year and continue till the first day of the year is after the end of the period (cf
    // gtfs_parser_test.boost_periods)
    for (boost::gregorian::year_iterator y_it(validity_period.begin());
         boost::gregorian::date((*y_it).year(), 1, 1) < validity_period.end(); ++y_it) {
        years.push_back((*y_it).year());
    }

    BOOST_ASSERT(!years.empty());
    bool is_dst_start_smaller = (boost_timezone->dst_local_start_time(years.back()).date()
                                 < boost_timezone->dst_local_end_time(years.back()).date());
    auto utc_jan_offset = boost_timezone->base_utc_offset();
    auto utc_june_offset = boost_timezone->base_utc_offset() + boost_timezone->dst_offset();
    if (!is_dst_start_smaller) {
        utc_jan_offset = boost_timezone->base_utc_offset() + boost_timezone->dst_offset();
        utc_june_offset = boost_timezone->base_utc_offset();
    }

    std::vector<PeriodWithUtcShift> res;
    for (int year : years) {
        auto saving_start_time = boost_timezone->dst_local_start_time(year).date();
        auto saving_end_time = boost_timezone->dst_local_end_time(year).date();

        if (!is_dst_start_smaller) {
            saving_start_time = boost_timezone->dst_local_end_time(year).date();
            saving_end_time = boost_timezone->dst_local_start_time(year).date();
        }

        if (!res.empty()) {
            // if res is not empty we add the additional period without the dst
            // from the previous end date to the beggining of the dst next year
            res.push_back({{res.back().period.end(), saving_start_time}, utc_jan_offset});

        } else {
            // for the first elt, we add a non dst
            auto first_day_of_year = boost::gregorian::date(year, 1, 1);
            if (boost_timezone->dst_local_start_time(year).date() != first_day_of_year) {
                res.push_back({{first_day_of_year, saving_start_time}, utc_jan_offset});
            }
        }

        res.push_back({{saving_start_time, saving_end_time}, utc_june_offset});
    }

    // we add the last non DST period
    res.push_back({{res.back().period.end(), boost::gregorian::date(years.back() + 1, 1, 1)}, utc_jan_offset});

    // we want the shift in seconds, and it is in minute in the tzdb
    for (auto& p_shift : res) {
        p_shift.utc_shift *= 60;
    }
    return res;
}

void EdTZWrapper::build_tz(const boost::gregorian::date_period& validity_period) {
    const auto& splited_production_period = split_over_dst(validity_period);
    tz_handler = nt::TimeZoneHandler(tz_name, validity_period.begin(), splited_production_period);
}

nt::TimeZoneHandler::dst_periods EdTZWrapper::split_over_dst(
    const boost::gregorian::date_period& validity_period) const {
    nt::TimeZoneHandler::dst_periods res;

    if (!boost_timezone) {
        LOG4CPLUS_FATAL(log4cplus::Logger::getInstance("log"), "no timezone available, cannot compute dst split");
        return res;
    }

    boost::posix_time::time_duration utc_offset = boost_timezone->base_utc_offset();

    if (!boost_timezone->has_dst()) {
        // no dst -> easy way out, no split, we just have to take the utc offset into account
        res[utc_offset.total_seconds()].push_back(validity_period);
        return res;
    }

    std::vector<PeriodWithUtcShift> dst_periods = get_dst_periods(validity_period);

    // we now compute all intersection between periods
    // to use again the example of get_dst_periods:
    //                                      validity period
    //                                  [----------------------------]
    //                            2013                                  2014
    //           <------------------------------------><-------------------------------------->
    //    [           non DST   )[  DST    )[        non DST     )[   DST     )[     non DST          )
    //
    // we create the periods:
    //
    //                                  [+8)[       +7h          )[+8h)
    //
    // all period_with_utc_shift are grouped by dst offsets.
    // ie in the previous example there are 2 period_with_utc_shift:
    //                        1/        [+8)                      [+8h)
    //                        2/            [       +7h          )
    for (const auto& dst_period : dst_periods) {
        if (!validity_period.intersects(dst_period.period)) {
            // no intersection, we don't consider it
            continue;
        }
        auto intersec = validity_period.intersection(dst_period.period);

        res[dst_period.utc_shift].push_back(intersec);
    }

    return res;
}

Georef::~Georef() {
    for (auto itm : this->nodes) {
        delete itm.second;
    }
    for (auto itm : this->edges) {
        delete itm.second;
    }
    for (auto itm : this->ways) {
        delete itm.second;
    }
    for (auto itm : this->admins) {
        delete itm.second;
    }
    for (auto itm : this->poi_types) {
        delete itm.second;
    }
}

}  // namespace ed
