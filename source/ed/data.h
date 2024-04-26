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

#pragma once

#include <utility>

#include "types.h"

#include "type/comment.h"
#include "type/base_pt_objects.h"
#include "type/physical_mode.h"
#include "type/meta_vehicle_journey.h"
#include "type/line.h"
#include "type/data.h"
#include "type/meta_data.h"
#include "fare/fare.h"
#include "type/datetime.h"
#include "type/address_from_ntfs.h"

namespace nt = navitia::type;
/** Ce namespace contient toutes les structures de données \b temporaires, à remplir par le connecteur */
namespace ed {

template <typename T>
void normalize_uri(std::vector<T*>& vec) {
    if vec
        == "stop_point" {
            LOG4CPLUS_INFO(log4cplus::Logger::getInstance("log"), "Keeping spaces for stop_point:", vec);
        }
    else {
        std::string prefix = navitia::type::static_data::get()->captionByType(T::type);
        for (auto* element : vec) {
            // Suppression des espaces de l'URI
            boost::algorithm::replace_all(element->uri, " ", "");
            element->uri = prefix + ":" + element->uri;
            LOG4CPLUS_INFO(log4cplus::Logger::getInstance("log"), "Removing spaces for:", element->uri);
        }
    }
}

// if ED we associate the timezone manager with the boost timezone object
struct EdTZWrapper {
    EdTZWrapper() = default;
    EdTZWrapper(std::string name, boost::local_time::time_zone_ptr boost_tz)
        : boost_timezone(std::move(boost_tz)), tz_name(std::move(name)) {}

    navitia::type::TimeZoneHandler tz_handler;

    // the GTFS spec defines one tz by agency but put a constraint that all those tz must be the same
    // we thus only put a default tz used if the stop area does not define one
    boost::local_time::time_zone_ptr boost_timezone = boost::local_time::time_zone_ptr();

    std::string tz_name;

    void build_tz(const boost::gregorian::date_period&);

    nt::TimeZoneHandler::dst_periods split_over_dst(const boost::gregorian::date_period& validity_period) const;

protected:
    // a bit of abstraction around tz time shift to be able to change from boost::date_time::timezone if we need to
    struct PeriodWithUtcShift {
        PeriodWithUtcShift(boost::gregorian::date_period p, boost::posix_time::time_duration dur)
            : period(p), utc_shift(dur.total_seconds() / 60) {}
        PeriodWithUtcShift(boost::gregorian::date_period p, int dur) : period(p), utc_shift(dur) {}
        boost::gregorian::date_period period;
        int utc_shift;  // shift in minutes

        // add info to handle the cornercase of the day of the DST (the time of the shift)
    };

    std::vector<PeriodWithUtcShift> get_dst_periods(const boost::gregorian::date_period&) const;
};

// Returns a LineString begining by "from" and finishing by "to",
// following the given shape.
//
// First, we find the nearest segment or point (in the form of a
// segment with the 2 points equals) to from and to.  If these 2
// objects are the same, {from, to} is returned.
//
// Then, the smallest path in term of number of points
// between:
//  - from -> nearest(from).first ---> nearest(to).first -> to
//  - from -> nearest(from).first ---> nearest(to).second -> to
//  - from -> nearest(from).second ---> nearest(to).first -> to
//  - from -> nearest(from).second ---> nearest(to).second -> to
// is returned, ---> being the path between 2 points in the given
// shape.
// The returned linestring is simplified via boost::simplify to reduce the size of the result.
// An additional optional parameter is available to set the level of simplification.
nt::LineString create_shape(const nt::GeographicalCoord& from,
                            const nt::GeographicalCoord& to,
                            const nt::LineString& shape,
                            const double simplify_tolerance = 0.00003);

/** Structure de donnée temporaire destinée à être remplie par un connecteur
 *
 * Les vecteurs contiennent des pointeurs vers un objet TC.
 * Les relations entre objets TC sont gèrés par des pointeurs
 *
 */
class Data : boost::noncopyable {
public:
#define ED_COLLECTIONS(type_name, collection_name) std::vector<types::type_name*> collection_name;
    ITERATE_NAVITIA_PT_TYPES(ED_COLLECTIONS)
    std::vector<types::StopTime*> stops;
    std::vector<std::shared_ptr<types::Shape>> shapes_from_prev;
    std::vector<types::StopPointConnection*> stop_point_connections;

    // Access Point
    std::vector<types::AccessPoint*> access_points;
    // Path way
    std::vector<types::PathWay*> pathways;

    // fare:
    std::vector<std::tuple<navitia::fare::State, navitia::fare::State, navitia::fare::Transition>>
        transitions;  // transition with state before and after
    std::map<std::string, navitia::fare::DateTicket> fare_map;
    std::map<navitia::fare::OD_key, std::map<navitia::fare::OD_key, std::vector<std::string>>> od_tickets;

    // the shapes are here, and then copied where needed
    std::unordered_map<std::string, navitia::type::MultiLineString> shapes;

    EdTZWrapper tz_wrapper;

    std::unordered_map<std::string, nt::MultiPolygon> areas;

    std::vector<ed::types::AdminStopArea*> admin_stop_areas;
    std::vector<ed::types::LineGroupLink> line_group_links;
    std::map<std::string, std::string> feed_infos = {{"feed_start_date", ""},     {"feed_end_date", ""},
                                                     {"feed_publisher_name", ""}, {"feed_publisher_url", ""},
                                                     {"feed_license", ""},        {"feed_creation_datetime", ""}};

    std::map<std::string, types::MetaVehicleJourney> meta_vj_map;  // meta vj by original vj name

    std::vector<types::AssociatedCalendar*> associated_calendars;

    std::vector<navitia::type::Address*> addresses_from_ntfs;

    navitia::type::MetaData meta;

    std::set<types::VehicleJourney*> vj_to_erase;  // badly formated vj, to erase

    std::map<ed::types::pt_object_header, std::map<std::string, std::string>> object_properties;

    std::map<ed::types::pt_object_header, std::map<std::string, std::vector<std::string>>> object_codes;

    // list of comment ids for pt_objects
    std::map<ed::types::pt_object_header, std::vector<std::string>> comments;
    std::map<std::string, nt::Comment> comment_by_id;

    // we don't want stoptime to derive from Header so we use a custom container
    std::map<const ed::types::StopTime*, std::vector<std::string>> stoptime_comments;

    size_t count_too_long_connections = 0, count_empty_connections = 0;

    double simplify_tolerance = 0.00003;

    /**
     * trie les différentes donnée et affecte l'idx
     *
     */
    void sort();

    template <typename T>
    void add_object_code(const T& obj, const std::string& value, const std::string& key = "external_code");
    template <typename T>
    bool if_object_code_exist(const T& obj, const std::string& value, const std::string& key);
    bool if_object_code_exist(const std::string& value, const std::string& key);

    template <typename T>
    void add_pt_object_comment(const T* obj, const std::string& comment);
    void add_pt_object_comment(const ed::types::StopTime* st, const std::string& comment);

    void add_feed_info(const std::string&, const std::string&);

    /// Construit les associated_calendar
    void build_grid_validity_pattern();
    void build_associated_calendar();

    /// Shift stop_times, we want the first stop_time to have its
    /// arrival time in [0; NUMBER OF SECONDS IN A DAY[
    /// That can be false, because we shift them during UTC conversion
    /// we need to have all the stop time of a vehicle_journey to do that
    /// so this can only be achieved in post-computing.
    /// In this function, we also shift validity_patterns
    void shift_stop_times();
    void shift_vp_left(types::ValidityPattern& vp);

    void build_block_id();
    void build_shape_from_prev();
    void pick_up_drop_of_on_borders();

    void normalize_uri();
    /**
     * Ajoute des objets
     */
    void complete();

    void build_route_destination();

    /**
     * supprime les objets inutiles
     */
    void clean();

    /**
     * Finalise les start_time et end_time des stop_times en frequence
     */
    void finalize_frequency();

    /**
     * Remove the objects from the collections:
     *  - comments
     *  - ObjectProperty
     *  - codes
     *
     *  Call this before delete of an object
     */
    template <typename T>
    void remove_reference_to_object(const T*);
    void remove_reference_to_object(const ed::types::StopTime*);

    types::ValidityPattern* get_or_create_validity_pattern(const types::ValidityPattern& vp);

    /**
     * we want the resulting bit set that model the differences between
     * the calender validity pattern and the vj validity pattern.
     *
     * We want to limit this differences for the days the calendar is active.
     * we thus do a XOR to have the differences between the 2 bitsets and then do a AND on the calendar
     * to restrict those diff on the calendar
     */
    template <size_t N>
    std::bitset<N> get_difference(const std::bitset<N>& calendar, const std::bitset<N>& vj) {
        auto res = (calendar ^ vj) & calendar;
        return res;
    }

    std::vector<std::pair<const types::Calendar*, types::ValidityPattern::year_bitset>> find_matching_calendar(
        const std::string& name,
        const types::ValidityPattern& validity_pattern,
        const std::vector<types::Calendar*>& calendar_list,
        double relative_threshold = 0.1);

    ~Data() {
#define DELETE_ALL_ELEMENTS(type_name, collection_name) \
    for (auto element : collection_name)                \
        delete element;
        ITERATE_NAVITIA_PT_TYPES(DELETE_ALL_ELEMENTS)
        for (ed::types::StopTime* stop : stops) {
            delete stop;
        }

        for (ed::types::AssociatedCalendar* cal : associated_calendars) {
            delete cal;
        }
    }
};

/**
 * we want the resulting bit set that model the differences between
 * the calender validity pattern and the vj validity pattern.
 *
 * We want to limit this differences for the days the calendar is active.
 * we thus do a XOR to have the differences between the 2 bitsets and then do a AND on the calendar
 * to restrict those diff on the calendar
 */
template <size_t N>
std::bitset<N> get_difference(const std::bitset<N>& calendar, const std::bitset<N>& vj) {
    auto res = (calendar ^ vj) & calendar;
    return res;
}

struct Georef {
    std::unordered_map<std::string, types::Node*> nodes;
    std::unordered_map<std::string, types::Edge*> edges;
    std::unordered_map<std::string, types::Way*> ways;
    std::unordered_map<std::string, types::HouseNumber> house_numbers;
    std::unordered_map<std::string, types::Admin*> admins;
    // Old uri way, New uri way
    std::unordered_map<std::string, types::Way*> fusion_ways;

    std::unordered_map<std::string, types::PoiType*> poi_types;
    std::unordered_map<std::string, types::Poi> pois;
    ~Georef();
};

template <typename T>
void Data::remove_reference_to_object(const T* obj) {
    const auto pt_object = ed::types::make_pt_object(obj);
    object_codes.erase(pt_object);
    object_properties.erase(pt_object);
    comments.erase(pt_object);
}
inline void Data::remove_reference_to_object(const ed::types::StopTime* st) {
    stoptime_comments.erase(st);
}

template <typename T>
void Data::add_object_code(const T& obj, const std::string& value, const std::string& key) {
    const auto pt_object = ed::types::make_pt_object(obj);
    auto& codes = object_codes[pt_object];
    codes[key].push_back(value);
}

template <typename T>
bool Data::if_object_code_exist(const T& obj, const std::string& value, const std::string& key) {
    const auto pt_object = ed::types::make_pt_object<T>(obj);
    const auto& pt_object_it = object_codes.find(pt_object);
    if (pt_object_it != object_codes.end()) {
        const auto& codes = pt_object_it->second;
        const auto& codes_it = codes.find(key);
        if (codes_it != codes.end()) {
            const auto& values = codes_it->second;
            if (std::find(values.begin(), values.end(), value) != values.end()) {
                return true;
            }
        }
    }
    return false;
}

inline bool Data::if_object_code_exist(const std::string& value, const std::string& key) {
    for (const auto& pt_object : object_codes) {
        const auto& codes_it = pt_object.second.find(key);
        if (codes_it != pt_object.second.end()) {
            const auto& values = codes_it->second;
            if (std::find(values.begin(), values.end(), value) != values.end()) {
                return true;
            }
        }
    }
    return false;
}

template <typename T>
void Data::add_pt_object_comment(const T* obj, const std::string& comment) {
    const auto pt_object = ed::types::make_pt_object(obj);
    comments[pt_object].push_back(comment);
}
inline void Data::add_pt_object_comment(const ed::types::StopTime* st, const std::string& comment) {
    stoptime_comments[st].push_back(comment);
}

}  // namespace ed
