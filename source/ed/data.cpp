/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
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
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "data.h"
#include <iostream>
#include "ptreferential/where.h"
#include "utils/timer.h"
#include "utils/functions.h"
#include "utils/exception.h"
#include <boost/geometry.hpp>
#include <boost/geometry/geometry.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include "type/datetime.h"



namespace nt = navitia::type;
namespace ed{

void Data::sort(){
#define SORT_AND_INDEX(type_name, collection_name) std::sort(collection_name.begin(), collection_name.end(), Less());\
    std::for_each(collection_name.begin(), collection_name.end(), Indexer<nt::idx_t>());
    ITERATE_NAVITIA_PT_TYPES(SORT_AND_INDEX)

    std::sort(stops.begin(), stops.end(), Less());
}

void Data::normalize_uri(){
    ::ed::normalize_uri(networks);
    ::ed::normalize_uri(companies);
    ::ed::normalize_uri(commercial_modes);
    ::ed::normalize_uri(lines);
    ::ed::normalize_uri(physical_modes);
    ::ed::normalize_uri(stop_areas);
    ::ed::normalize_uri(stop_points);
    ::ed::normalize_uri(vehicle_journeys);
    ::ed::normalize_uri(validity_patterns);
    ::ed::normalize_uri(calendars);
}

void Data::build_block_id() {
    /// We want to group vehicle journeys by their block_id
    /// Two vehicle_journeys with the same block_id vj1 are consecutive if
    /// the last arrival_time of vj1 <= to the departure_time of vj2
    std::sort(vehicle_journeys.begin(), vehicle_journeys.end(),
              [](const types::VehicleJourney* vj1, const types::VehicleJourney* vj2) {
        if(vj1->block_id != vj2->block_id) {
            return vj1->block_id < vj2->block_id;
        } else if (vj1->utc_to_local_offset != vj2->utc_to_local_offset) {
            // we don't want to link the splited vjs
            return vj1->utc_to_local_offset < vj2->utc_to_local_offset;
        } else {
            return vj1->stop_time_list.back()->departure_time <=
                    vj2->stop_time_list.front()->departure_time;
        }
    }
    );

    types::VehicleJourney* prev_vj = nullptr;
    for(auto* vj : vehicle_journeys) {
        if(prev_vj && prev_vj->block_id != "" &&
           prev_vj->block_id == vj->block_id){
            //NOTE: we do nothing for vj with empty stop times, they will be removed in the clean()
            if (! vj->stop_time_list.empty() && ! prev_vj->stop_time_list.empty()) {

                // Sanity check
                // If the departure time of the 1st stoptime of vj is greater
                // then the arrivaltime of the last stop time of prev_vj
                // there is a time travel, and we don't like it!
                // This is not supposed to happen
                // @TODO: Add a parameter to avoid too long connection
                // they can be for instance due to bad data
                if(vj->stop_time_list.front()->departure_time >= prev_vj->stop_time_list.back()->arrival_time) {

                    //we add another check that the vjs are on the same offset (that they are not the from vj split on different dst)
                    if (vj->utc_to_local_offset == prev_vj->utc_to_local_offset) {
                        prev_vj->next_vj = vj;
                        vj->prev_vj = prev_vj;
                    }
                }
            }
        }
        prev_vj = vj;
    }
}

static std::map<std::string, std::set<std::string>>
make_departure_destinations_map(
    const std::vector<types::StopPointConnection*>& stop_point_connections)
{
    std::map<std::string, std::set<std::string>> res;
    for (auto conn: stop_point_connections) {
        res[conn->departure->uri].insert(conn->destination->uri);
    }
    return res;
}

static std::map<std::string, std::vector<types::StopPoint*>>
make_stop_area_stop_points_map(const std::vector<ed::types::StopPoint*>& stop_points) {
    std::map<std::string, std::vector<types::StopPoint*>> res;
    for (auto sp: stop_points) {
        if (sp->stop_area) res[sp->stop_area->uri].push_back(sp);
    }
    return res;
}


types::ValidityPattern* Data::get_or_create_validity_pattern(const types::ValidityPattern& vp) {
    auto find_vp_predicate = [&vp](types::ValidityPattern* vp2) {
        return vp.days == vp2->days;
    };
    auto it = boost::find_if(validity_patterns,
            find_vp_predicate);
    if(it != validity_patterns.end()) {
        return *(it);
    } else {
         validity_patterns.push_back(new types::ValidityPattern(vp));
         return validity_patterns.back();
    }
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
    }
    vp.days >>= 1;
}

void Data::shift_stop_times() {
    for (auto vj : vehicle_journeys) {
        const auto first_st_it = std::min_element(vj->stop_time_list.begin(), vj->stop_time_list.end(),
                [](const types::StopTime* st1, const types::StopTime* st2) { return st1->order < st2->order;});
        if (first_st_it == vj->stop_time_list.end()) {
            continue;
        }
        const auto first_st = *first_st_it;

        // For non-frequency vj, we must have the first stop time in
        // [0; 24:00[ (since they are ordered, every stop time will be
        // greatter than 0)
        //
        // For frequency vj, the start time must be in [0; 24:00[.
        const int start_time = vj->is_frequency() ? vj->start_time : first_st->arrival_time;

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
            default:
                throw navitia::exception("Ed: You have to shift more than one day, that's weird");
            }
            vj->validity_pattern = get_or_create_validity_pattern(vp);
            for (auto st: vj->stop_time_list)
                st->shift_times(shift);
        }

        if (vj->is_frequency()) {
            // shifting start_time in [0; 24:00[
            vj->start_time += shift * int(navitia::DateTimeUtils::SECONDS_PER_DAY);
            // vj->end_time must be gretter than 0 (but it can be lesser than start_time)
            while (vj->end_time < 0)
                vj->end_time += int(navitia::DateTimeUtils::SECONDS_PER_DAY);
        }
    }
}


void Data::complete(){
    build_journey_patterns();
    build_journey_pattern_points();
    build_block_id();

    build_grid_validity_pattern();
    build_associated_calendar();

    shift_stop_times();
    finalize_frequency();

    ::ed::normalize_uri(journey_patterns);
    ::ed::normalize_uri(routes);

    // generates default connections inside each stop area
    auto connections = make_departure_destinations_map(stop_point_connections);
    const auto sa_sps = make_stop_area_stop_points_map(stop_points);
    const auto connections_size = stop_point_connections.size();
    for(const types::StopArea * sa : stop_areas) {
        const auto& sps = find_or_default(sa->uri, sa_sps);
        for (const auto& sp1: sps) {
            for (const auto& sp2: sps) {
                // if the connection exists, do nothing
                if (find_or_default(sp1->uri, connections).count(sp2->uri) != 0) continue;

                const int conn_dur_itself = 0;
                const int conn_dur_other = 120;
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

    typedef std::vector<ed::types::VehicleJourney *> vjs;
    std::unordered_map<std::string, vjs> journey_pattern_vj;
    for(auto it = vehicle_journeys.begin(); it != vehicle_journeys.end(); ++it) {
        journey_pattern_vj[(*it)->journey_pattern->uri].push_back((*it));
    }

    int erase_overlap = 0, erase_emptiness = 0, erase_no_circulation = 0, erase_invalid_stoptimes = 0;

    for(auto it1 = journey_pattern_vj.begin(); it1 != journey_pattern_vj.end(); ++it1) {

        for(auto vj1 = it1->second.begin(); vj1 != it1->second.end(); ++vj1) {
            if (vj_to_erase.count(*vj1)) {
                toErase.insert((*vj1)->uri);
                continue;
            }
            if((*vj1)->stop_time_list.empty()) {
                toErase.insert((*vj1)->uri);
                ++erase_emptiness;
                continue;
            }
            if((*vj1)->validity_pattern->days.none() && (*vj1)->adapted_validity_pattern->days.none()) {
                toErase.insert((*vj1)->uri);
                ++erase_no_circulation;
                continue;
            }
            for(auto vj2 = (vj1+1); vj2 != it1->second.end(); ++vj2) {
                if(((*vj1)->validity_pattern->days & (*vj2)->validity_pattern->days).any()  &&
                        (*vj1)->stop_time_list.size() > 0 && (*vj2)->stop_time_list.size() > 0) {
                    ed::types::VehicleJourney *vjs1, *vjs2;
                    if((*vj1)->stop_time_list.front()->departure_time <= (*vj2)->stop_time_list.front()->departure_time) {
                        vjs1 = *vj1;
                        vjs2 = *vj2;
                    }
                    else {
                        vjs1 = *vj2;
                        vjs2 = *vj1;
                    }

                    using ed::types::StopTime;
                    const bool are_equal =
                        (*vj1)->validity_pattern->days != (*vj2)->validity_pattern->days
                        && boost::equal((*vj1)->stop_time_list, (*vj2)->stop_time_list,
                                     [](const StopTime* st1, const StopTime* st2) {
                                         return st1->departure_time == st2->departure_time
                                         && st1->arrival_time == st2->arrival_time;
                                     });
                    if (are_equal) {
                        LOG4CPLUS_WARN(logger, "Data::clean(): are equal with different overlapping vp: "
                                       << (*vj1)->uri << " and " << (*vj2)->uri);
                        continue;
                    }

                    for(auto rp = (*vj1)->journey_pattern->journey_pattern_point_list.begin(); rp != (*vj1)->journey_pattern->journey_pattern_point_list.end();++rp) {
                        if(vjs1->stop_time_list.at((*rp)->order)->departure_time >= vjs2->stop_time_list.at((*rp)->order)->departure_time ||
                           vjs1->stop_time_list.at((*rp)->order)->arrival_time >= vjs2->stop_time_list.at((*rp)->order)->arrival_time) {
                            toErase.insert((*vj2)->uri);
                            LOG4CPLUS_WARN(logger, "Data::clean(): overlap: "
                                           << (*vj1)->uri << ":"
                                           << (*vj1)->stop_time_list.front()->departure_time << "->"
                                           << (*vj1)->stop_time_list.at((*rp)->order)->departure_time
                                           << "->"
                                           << (*vj1)->stop_time_list.at((*rp)->order)->arrival_time
                                           << " and "
                                           << (*vj2)->uri << ":"
                                           << (*vj2)->stop_time_list.front()->departure_time << "->"
                                           << (*vj2)->stop_time_list.at((*rp)->order)->departure_time
                                           << "->"
                                           << (*vj2)->stop_time_list.at((*rp)->order)->arrival_time
                                );

                            ++erase_overlap;
                            break;
                        }
                    }
                }
            }
            // we check that no stop times are negatives
            for (const auto st: (*vj1)->stop_time_list) {
                if (st->departure_time < 0 || st->arrival_time < 0) {
                    toErase.insert((*vj1)->uri);
                    ++erase_invalid_stoptimes;
                    break;
                }
            }
        }
    }

    std::vector<size_t> erasest;

    for(int i=stops.size()-1; i >=0;--i) {
        auto it = toErase.find(stops[i]->vehicle_journey->uri);
        if(it != toErase.end()) {
            erasest.push_back(i);
        }
    }

    // For each stop_time to remove, we delete and put the last element of the vector in it's place
    // We avoid resizing the vector until completition for performance reasons.
    size_t num_elements = stops.size();
    for(size_t to_erase : erasest) {
        delete stops[to_erase];
        stops[to_erase] = stops[num_elements - 1];
        num_elements--;
    }


    stops.resize(num_elements);

    erasest.clear();
    for(int i=vehicle_journeys.size()-1; i >= 0;--i){
        auto it = toErase.find(vehicle_journeys[i]->uri);
        if(it != toErase.end()) {
            erasest.push_back(i);
        }
    }

    // The same but now with vehicle_journey's
    num_elements = vehicle_journeys.size();
    for(size_t to_erase : erasest) {
        auto vj = vehicle_journeys[to_erase];
        if(vj->next_vj) {
            vj->next_vj->prev_vj = nullptr;
        }
        if(vj->prev_vj) {
            vj->prev_vj->next_vj = nullptr;
        }
        //we need to remove the vj from the meta vj
        auto& metavj = meta_vj_map[vj->meta_vj_name];
        bool found = false;
        for (auto it = metavj.theoric_vj.begin() ; it != metavj.theoric_vj.end() ; ++it) {
            if (*it == vj) {
                metavj.theoric_vj.erase(it);
                found = true;
                break;
            }
        }
        if (! found) {
            throw navitia::exception("construction problem, impossible to find the vj " + vj->uri + " in the meta vj " + vj->meta_vj_name);
        }
        if (metavj.theoric_vj.empty()) {
            //we remove the meta vj
            meta_vj_map.erase(vj->meta_vj_name);
        }
        delete vj;
        vehicle_journeys[to_erase] = vehicle_journeys[num_elements - 1];
        num_elements--;
    }
    vehicle_journeys.resize(num_elements);

    LOG4CPLUS_INFO(logger, "Data::clean(): " << erase_overlap <<  " vehicle_journeys have been deleted because they overlap, "
                   << erase_emptiness << " because they do not contain any clean stop_times, "
                   << erase_no_circulation << " because they are never valid "
                   << " and " << erase_invalid_stoptimes << " because the stop times were negatives");

    // Delete duplicate connections
    // Connections are sorted by departure,destination
    auto sort_function = [](types::StopPointConnection * spc1, types::StopPointConnection *spc2) {return spc1->uri < spc2->uri
                                                                                                    || (spc1->uri == spc2->uri && spc1 < spc2 );};

    auto unique_function = [](types::StopPointConnection * spc1, types::StopPointConnection *spc2) {return spc1->uri == spc2->uri;};

    std::sort(stop_point_connections.begin(), stop_point_connections.end(), sort_function);
    num_elements = stop_point_connections.size();
    auto it_end = std::unique(stop_point_connections.begin(), stop_point_connections.end(), unique_function);
    //@TODO : Attention, it's leaking, it should succeed in erasing objects
    //Ce qu'il y a dans la fin du vecteur apres unique n'est pas garanti, on ne peut pas itérer sur la suite pour effacer
    stop_point_connections.resize(std::distance(stop_point_connections.begin(), it_end));
    LOG4CPLUS_INFO(logger, num_elements-stop_point_connections.size()
                   << " stop point connections deleted because of duplicate connections");
}

static void affect_shape(nt::LineString& to, const nt::MultiLineString& from) {
    if (from.empty()) return;
    if (to.size() < from.front().size()) { to = from.front(); }
}

// TODO : For now we construct one route per journey pattern
// We should factorise the routes.
void Data::build_journey_patterns() {
    auto logger = log4cplus::Logger::getInstance("log");
    LOG4CPLUS_INFO(logger, "building journey patterns...");

    // Associate each line-uri with the number of journey_pattern's found up to now
    for(auto it1 = this->vehicle_journeys.begin(); it1 != this->vehicle_journeys.end(); ++it1){
        types::VehicleJourney * vj1 = *it1;
        ed::types::Route* route = vj1->tmp_route;
        // If the VehicleJourney does not belong to any journey_pattern
        if(vj1->journey_pattern == 0) {
            std::string journey_pattern_uri = vj1->tmp_line->uri;
            journey_pattern_uri += "-";
            journey_pattern_uri += boost::lexical_cast<std::string>(this->journey_patterns.size());

            types::JourneyPattern * journey_pattern = new types::JourneyPattern();
            journey_pattern->uri = journey_pattern_uri;
            if(route == nullptr){
                route = new types::Route();
                route->line = vj1->tmp_line;
                route->uri = journey_pattern->uri;
                route->name = journey_pattern->name;
                this->routes.push_back(route);
            }
            journey_pattern->route = route;
            journey_pattern->physical_mode = vj1->physical_mode;
            affect_shape(journey_pattern->shape, find_or_default(vj1->shape_id, shapes));
            vj1->journey_pattern = journey_pattern;
            this->journey_patterns.push_back(journey_pattern);

            for(auto it2 = it1 + 1; it1 != this->vehicle_journeys.end() && it2 != this->vehicle_journeys.end(); ++it2){
                types::VehicleJourney * vj2 = *it2;
                if(vj2->journey_pattern == 0 && same_journey_pattern(vj1, vj2)){
                    vj2->journey_pattern = vj1->journey_pattern;
                    affect_shape(journey_pattern->shape, find_or_default(vj2->shape_id, shapes));
                }
            }
        }
    }
    LOG4CPLUS_INFO(logger, "Number of journey_patterns : " << journey_patterns.size());
}

typedef nt::LineString::const_iterator LineStringIter;
typedef std::pair<LineStringIter, LineStringIter> LineStringIterPair;

// Returns the nearest segment or point from coord under the form of a
// pair of iterators {it1, it2}.  If it is a segment, it1 + 1 == it2.
// If it is a point, it1 == it2.  If line is empty, it1 == it2 == line.end()
static LineStringIterPair
get_nearest(const nt::GeographicalCoord& coord, const nt::LineString& line) {
    if (line.empty()) return {line.end(), line.end()};
    auto nearest = std::make_pair(line.begin(), line.begin());
    auto nearest_dist = coord.project(*nearest.first, *nearest.second).second;
    for (auto it1 = line.begin(), it2 = it1 + 1; it2 != line.end(); ++it1, ++it2) {
        auto projection = coord.project(*it1, *it2);
        if (nearest_dist <= projection.second) continue;

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

static size_t abs_distance(LineStringIterPair pair) {
    return pair.first < pair.second
                        ? pair.second - pair.first
                        : pair.first - pair.second;
}

static LineStringIterPair
get_smallest_range(const LineStringIterPair& p1, const LineStringIterPair& p2) {
    typedef LineStringIterPair P;
    P res = {p1.first, p2.first};
    for (P p: {P(p1.first, p2.second), P(p1.second, p2.first), P(p1.second, p2.second)}) {
        if (abs_distance(res) > abs_distance(p)) res = p;
    }
    return res;
}

nt::LineString
create_shape(const nt::GeographicalCoord& from,
             const nt::GeographicalCoord& to,
             const nt::LineString& shape)
{
    const auto nearest_from = get_nearest(from, shape);
    const auto nearest_to = get_nearest(to, shape);
    if (nearest_from == nearest_to) { return {from, to}; }

    nt::LineString res;
    const auto range = get_smallest_range(nearest_from, nearest_to);
    res.push_back(from);
    if (range.first < range.second) {
        for (auto it = range.first; it <= range.second; ++it)
            res.push_back(*it);
    } else {
        for (auto it = range.first; it >= range.second; --it)
            res.push_back(*it);
    }
    res.push_back(to);

    // simplification at about 3m precision
    nt::LineString simplified;
    boost::geometry::simplify(res, simplified, 0.00003);

    return simplified;
}

void Data::build_journey_pattern_points(){
    auto logger = log4cplus::Logger::getInstance("log");
    LOG4CPLUS_TRACE(logger, "Construct journey_pattern points");
    std::map<std::string, ed::types::JourneyPatternPoint*> journey_pattern_point_map;

    size_t nb_stop_time = 0;
    for(types::VehicleJourney * vj : this->vehicle_journeys){
        const nt::GeographicalCoord* prev_coord = nullptr;
        int stop_seq = 0;
        for(types::StopTime * stop_time : vj->stop_time_list){
            ++nb_stop_time;
            std::string journey_pattern_point_extcode = vj->journey_pattern->uri + ":" + stop_time->tmp_stop_point->uri+":"+boost::lexical_cast<std::string>(stop_seq);
            auto journey_pattern_point_it = journey_pattern_point_map.find(journey_pattern_point_extcode);
            types::JourneyPatternPoint * journey_pattern_point;
            if(journey_pattern_point_it == journey_pattern_point_map.end()) {
                journey_pattern_point = new types::JourneyPatternPoint();
                journey_pattern_point->journey_pattern = vj->journey_pattern;
                journey_pattern_point->journey_pattern->journey_pattern_point_list.push_back(journey_pattern_point);
                journey_pattern_point->stop_point = stop_time->tmp_stop_point;
                journey_pattern_point_map[journey_pattern_point_extcode] = journey_pattern_point;
                journey_pattern_point->order = stop_seq;
                journey_pattern_point->uri = journey_pattern_point_extcode;
                if (prev_coord) {
                    journey_pattern_point->shape_from_prev =
                        create_shape(*prev_coord,
                                     stop_time->tmp_stop_point->coord,
                                     vj->journey_pattern->shape);
                }
                this->journey_pattern_points.push_back(journey_pattern_point);
            } else {
                journey_pattern_point = journey_pattern_point_it->second;
            }
            prev_coord = &stop_time->tmp_stop_point->coord;
            ++stop_seq;
            stop_time->journey_pattern_point = journey_pattern_point;
        }
    }

    for(types::JourneyPattern *journey_pattern : this->journey_patterns){
        if(! journey_pattern->journey_pattern_point_list.empty()){
            types::JourneyPatternPoint * last = journey_pattern->journey_pattern_point_list.back();
            if(last->stop_point->stop_area != NULL)
                journey_pattern->name = last->stop_point->stop_area->name;
            else
                journey_pattern->name = last->stop_point->name;
            if(journey_pattern->route->name.empty()){
                journey_pattern->route->name = journey_pattern->name;
            }
        }
    }
    LOG4CPLUS_INFO(logger, "Number of journey_pattern points : " << journey_pattern_points.size()
                   << " for " << nb_stop_time << " stop times");
}

types::ValidityPattern get_union_validity_pattern(const types::MetaVehicleJourney* meta_vj) {
    types::ValidityPattern validity;

    for (auto* vj: meta_vj->theoric_vj) {
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

list_cal_bitset
Data::find_matching_calendar(const Data&, const std::string& name, const types::ValidityPattern& validity_pattern,
                        const std::vector<types::Calendar*>& calendar_list, double relative_threshold) {
    list_cal_bitset res;
    //for the moment we keep lot's of trace, but they will be removed after a while
    auto log = log4cplus::Logger::getInstance("kraken::type::Data::Calendar");
    LOG4CPLUS_TRACE(log, "meta vj " << name << " :" << validity_pattern.days.to_string());

    for (const auto calendar : calendar_list) {
        // sometimes a calendar can be empty (for example if it's validity period does not
        // intersect the data's validity period)
        // we do not filter those calendar since it's a user input, but we do not match them
        if (! calendar->validity_pattern.days.any()) {
            continue;
        }
        auto diff = get_difference(calendar->validity_pattern.days, validity_pattern.days);
        size_t nb_diff = diff.count();

        LOG4CPLUS_TRACE(log, "cal " << calendar->uri << " :" << calendar->validity_pattern.days.to_string());

        //we associate the calendar to the vj if the diff are below a relative threshold
        //compared to the number of active days in the calendar
        size_t threshold = std::round(relative_threshold * calendar->validity_pattern.days.count());
        LOG4CPLUS_TRACE(log, "**** diff: " << nb_diff << " and threshold: " << threshold << (nb_diff <= threshold ? ", we keep it!!":""));

        if (nb_diff > threshold) {
            continue;
        }
        res.push_back({calendar, diff});
    }

    return res;
}

void Data::build_grid_validity_pattern() {
    for(types::Calendar* cal : this->calendars){
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

    for(auto meta_vj_pair : meta_vj_map) {
        auto& meta_vj = meta_vj_map[meta_vj_pair.first]; //meta_vj_pair.second;

        assert (! meta_vj.theoric_vj.empty());

        if(!meta_vj.associated_calendars.empty()) {
            LOG4CPLUS_TRACE(log, "The meta_vj " << meta_vj_pair.first << " was already linked to a grid_calendar");
            nb_ignored++;
            continue;
        }

        // we check the theoric vj of a meta vj
        // because we start from the postulate that the theoric VJs are the same VJ
        // split because of dst (day saving time)
        // because of that we try to match the calendar with the union of all theoric vj validity pattern
       types::ValidityPattern meta_vj_validity_pattern = get_union_validity_pattern(&meta_vj);

        //some check can be done on any theoric vj, we do them on the first
        auto* first_vj = meta_vj.theoric_vj.front();

        //some check can be done on any theoric vj, we do them on the first
        std::vector<types::Calendar*> calendar_list;

        for (types::Calendar* calendar : this->calendars) {
            for (types::Line* line : calendar->line_list) {
                if (line->uri == first_vj->journey_pattern->route->line->uri)
                    calendar_list.push_back(calendar);
            }
        }

        if (calendar_list.empty()) {
            LOG4CPLUS_TRACE(log, "the line of the vj " << first_vj->uri << " is associated to no calendar");
            nb_not_matched_vj++;
            continue;
        }

        //we check if we already computed the associated val for this validity pattern
        //since a validity pattern can be shared by many vj
        auto it = associated_vp.find(meta_vj_validity_pattern);
        if (it != associated_vp.end()) {
            for (; it->first == meta_vj_validity_pattern; ++it) {
                meta_vj.associated_calendars.insert({it->second->calendar->uri, it->second});
            }
            continue;
        }

        auto close_cal = find_matching_calendar(*this, meta_vj_pair.first, meta_vj_validity_pattern, calendar_list);

        if (close_cal.empty()) {
            LOG4CPLUS_TRACE(log, "the meta vj " << meta_vj_pair.first << " has been attached to no calendar");
            nb_not_matched_vj++;
            continue;
        }
        nb_matched++;

        std::stringstream cal_uri;
        for (auto cal_bit_set: close_cal) {
            auto associated_calendar = new types::AssociatedCalendar();
            associated_calendar->idx = this->associated_calendars.size();
            this->associated_calendars.push_back(associated_calendar);

            associated_calendar->calendar = cal_bit_set.first;
            //we need to create the associated exceptions
            for (size_t i = 0; i < cal_bit_set.second.size(); ++i) {
                if (! cal_bit_set.second[i]) {
                    continue; //cal_bit_set.second is the resulting differences, so 0 means no exception
                }
                navitia::type::ExceptionDate ex;
                ex.date = meta_vj_validity_pattern.beginning_date + boost::gregorian::days(i);
                //if the vj is active this day it's an addition, else a removal
                ex.type = (meta_vj_validity_pattern.days[i] ? navitia::type::ExceptionDate::ExceptionType::add : navitia::type::ExceptionDate::ExceptionType::sub);
                associated_calendar->exceptions.push_back(ex);
            }

            meta_vj.associated_calendars.insert({associated_calendar->calendar->uri, associated_calendar});
            associated_vp.insert({meta_vj_validity_pattern, associated_calendar});
            cal_uri << associated_calendar->calendar->uri << " ";
        }

        LOG4CPLUS_DEBUG(log, "the meta vj " << meta_vj_pair.first << " has been attached to " << cal_uri.str());
    }

    LOG4CPLUS_INFO(log, nb_matched << " vehicle journeys have been matched to at least one calendar");
    if (nb_not_matched_vj) {
        LOG4CPLUS_WARN(log, "no calendar found for " << nb_not_matched_vj << " vehicle journey");
    }
    if (nb_ignored) {
        LOG4CPLUS_WARN(log, nb_ignored << " vehicle journey were already linked and therefore ignored" );
    }
}

// Check if two vehicle_journey's belong to the same journey_pattern
bool same_journey_pattern(types::VehicleJourney * vj1, types::VehicleJourney * vj2){
 
    if (vj1->tmp_line != vj2->tmp_line || vj1->tmp_route != vj2->tmp_route){
        return false;
    }

    if(vj1->stop_time_list.size() != vj2->stop_time_list.size())
        return false;

    for(size_t i = 0; i < vj1->stop_time_list.size(); ++i)
        if(vj1->stop_time_list[i]->tmp_stop_point != vj2->stop_time_list[i]->tmp_stop_point){
            return false;
        }
    return true;
}

//For frequency based trips, make arrival and departure time relative from first stop.
void Data::finalize_frequency() {
    for(auto * vj : this->vehicle_journeys) {
        if(!vj->stop_time_list.empty() && vj->stop_time_list.front()->is_frequency) {
            auto * first_st = vj->stop_time_list.front();
            int begin = first_st->arrival_time;
            if (begin == 0){
                continue; //Time is already relative to 0
            }
            for(auto * st : vj->stop_time_list) {
                st->arrival_time   -= begin;
                st->departure_time -= begin;
            }
        }
    }
}

Georef::~Georef(){
    for(auto itm : this->nodes)
         delete itm.second;
    for(auto itm : this->edges)
         delete itm.second;
    for(auto itm : this->ways)
         delete itm.second;
    for(auto itm : this->admins)
         delete itm.second;
    for(auto itm : this->poi_types)
         delete itm.second;
}

}//namespace
