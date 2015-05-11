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

#pragma once
#include "types.h"

#include "type/type.h"
#include "type/data.h"
#include "type/meta_data.h"
#include "fare/fare.h"
#include "type/datetime.h"

namespace nt = navitia::type;
/** Ce namespace contient toutes les structures de données \b temporaires, à remplir par le connecteur */
namespace ed{

template<typename T>
void normalize_uri(std::vector<T*>& vec){
    std::string prefix = navitia::type::static_data::get()->captionByType(T::type);
    for(auto* element : vec){
        // Suppression des espaces de l'URI
        boost::algorithm::replace_all(element->uri," ","");
        element->uri = prefix + ":" + element->uri;
    }
}

bool same_journey_pattern(types::VehicleJourney * vj1, types::VehicleJourney * vj2);

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
nt::LineString
create_shape(const nt::GeographicalCoord& from,
             const nt::GeographicalCoord& to,
             const nt::LineString& shape);


/** Structure de donnée temporaire destinée à être remplie par un connecteur
      *
      * Les vecteurs contiennent des pointeurs vers un objet TC.
      * Les relations entre objets TC sont gèrés par des pointeurs
      *
      */
class Data: boost::noncopyable {
public:
#define ED_COLLECTIONS(type_name, collection_name) std::vector<types::type_name*> collection_name;
    ITERATE_NAVITIA_PT_TYPES(ED_COLLECTIONS)
    std::vector<types::StopTime*> stops;
    std::vector<types::StopPointConnection*> stop_point_connections;

    //fare:
    std::vector<std::tuple<navitia::fare::State, navitia::fare::State, navitia::fare::Transition>> transitions; // transition with state before and after
    std::map<std::string, navitia::fare::DateTicket> fare_map;
    std::map<navitia::fare::OD_key, std::map<navitia::fare::OD_key, std::vector<std::string>>> od_tickets;

    // the shapes are here, and then copied where needed
    std::unordered_map<std::string, navitia::type::MultiLineString> shapes;

    std::unordered_map<std::string, nt::MultiPolygon> areas;

    std::vector<ed::types::AdminStopArea*>  admin_stop_areas;

    std::vector<ed::types::ObjectProperty> object_properties;

    std::map<std::string, types::MetaVehicleJourney> meta_vj_map; //meta vj by original vj name

    std::vector<types::AssociatedCalendar*> associated_calendars;

    navitia::type::MetaData meta;

    std::set<types::VehicleJourney*> vj_to_erase; //badly formated vj, to erase

    std::vector<ed::types::ObjectCode*> object_codes;

    size_t count_too_long_connections = 0,
           count_empty_connections = 0;

    /**
         * trie les différentes donnée et affecte l'idx
         *
         */
    void sort();

    /// Construit les journey_patterns en retrouvant les paterns à partir des VJ
    void build_journey_patterns();

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

    /// Construit les journey_patternpoint
    void build_journey_pattern_points();

    void build_block_id();

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

    std::vector<std::pair<const types::Calendar*, types::ValidityPattern::year_bitset>>
    find_matching_calendar(const Data&, const std::string& name, const types::ValidityPattern& validity_pattern,
                           const std::vector<types::Calendar*>& calendar_list, double relative_threshold = 0.1);

    ~Data(){
#define DELETE_ALL_ELEMENTS(type_name, collection_name) for(auto element : collection_name) delete element;
        ITERATE_NAVITIA_PT_TYPES(DELETE_ALL_ELEMENTS)
        for(ed::types::StopTime* stop : stops){
            delete stop;
        }
        for (ed::types::AssociatedCalendar* cal: associated_calendars) {
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

struct Georef{
    std::unordered_map<std::string, types::Node* > nodes;
    std::unordered_map<std::string, types::Edge* > edges;
    std::unordered_map<std::string, types::Way* > ways;
    std::unordered_map<std::string, types::HouseNumber> house_numbers;
    std::unordered_map<std::string, types::Admin *> admins;
                      // Old uri way, New uri way
    std::unordered_map<std::string, types::Way*> fusion_ways;

    std::unordered_map<std::string, types::PoiType *> poi_types;
    std::unordered_map<std::string, types::Poi> pois;
    ~Georef();
};

}
