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
#include "multi_polygon_map.h"
#include "type.h"
#include "georef/georef.h"
#include "type/message.h"
#include "type/request.pb.h"
#include "autocomplete/autocomplete.h"
#include "proximity_list/proximity_list.h"
#include "utils/flat_enum_map.h"
#include "utils/functions.h"
#include "utils/obj_factory.h"
#include "comment_container.h"
#include "code_container.h"
#include "headsign_handler.h"

#include <boost/serialization/map.hpp>
#include "utils/serialization_unordered_map.h"
#include "utils/serialization_tuple.h"

namespace navitia {
template <>
struct enum_size_trait<pbnavitia::PlaceCodeRequest::Type> {
    static constexpr typename get_enum_type<pbnavitia::PlaceCodeRequest::Type>::type size() {
        return 8;
    }
};
namespace type {

typedef std::map<std::string, std::string> code_value_map_type;
typedef std::map<std::string, code_value_map_type> type_code_codes_map_type;
struct PT_Data : boost::noncopyable{
#define COLLECTION_AND_MAP(type_name, collection_name) std::vector<type_name*> collection_name; std::unordered_map<std::string, type_name *> collection_name##_map;
    ITERATE_NAVITIA_PT_TYPES(COLLECTION_AND_MAP)

    std::vector<StopPointConnection*> stop_point_connections;

    // meta vj factory
    navitia::ObjFactory<MetaVehicleJourney> meta_vjs;

    //associated cal for vj
    std::vector<AssociatedCalendar*> associated_calendars;

    // First letter
    autocomplete::Autocomplete<idx_t> stop_area_autocomplete = autocomplete::Autocomplete<idx_t>(navitia::type::Type_e::StopArea);
    autocomplete::Autocomplete<idx_t> stop_point_autocomplete = autocomplete::Autocomplete<idx_t>(navitia::type::Type_e::StopPoint);
    autocomplete::Autocomplete<idx_t> line_autocomplete = autocomplete::Autocomplete<idx_t>(navitia::type::Type_e::Line);
    autocomplete::Autocomplete<idx_t> network_autocomplete = autocomplete::Autocomplete<idx_t>(navitia::type::Type_e::Network);
    autocomplete::Autocomplete<idx_t> mode_autocomplete = autocomplete::Autocomplete<idx_t>(navitia::type::Type_e::CommercialMode);
    autocomplete::Autocomplete<idx_t> route_autocomplete = autocomplete::Autocomplete<idx_t>(navitia::type::Type_e::Route);

    // Proximity list
    proximitylist::ProximityList<idx_t> stop_area_proximity_list;
    proximitylist::ProximityList<idx_t> stop_point_proximity_list;

    //Message
    disruption::DisruptionHolder disruption_holder;

    // rtree for zonal stop_points
    MultiPolygonMap<const StopPoint*> stop_points_by_area;

    // Comments container
    Comments comments;

    // Code container
    CodeContainer codes;

    // Headsign handler
    HeadsignHandler headsign_handler;

    // timezone manager
    TimeZoneManager tz_manager;

    // shape manager
    struct ShapeManager {
        const LineString* get(const LineString& l) { return &*set.insert(l).first; }
        template<class Archive> void serialize(Archive & ar, const unsigned int) { ar & set; }
    private:
        std::set<LineString> set;
    };
    ShapeManager shape_manager;

    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar
                & shape_manager // before anything
        #define SERIALIZE_ELEMENTS(type_name, collection_name) & collection_name & collection_name##_map
                ITERATE_NAVITIA_PT_TYPES(SERIALIZE_ELEMENTS)
                & stop_area_autocomplete & stop_point_autocomplete & line_autocomplete
                & network_autocomplete & mode_autocomplete & route_autocomplete
                & stop_area_proximity_list & stop_point_proximity_list
                & stop_point_connections
                & disruption_holder
                & meta_vjs
                & stop_points_by_area
                & comments
                & codes
                & headsign_handler
                & tz_manager;
    }

    /** Initialise tous les indexes
      *
      * Les données doivent bien évidemment avoir été initialisés
      */
    void build_index();

    /** Construit l'indexe ExternelCode */
    void build_uri();

    /** Construit l'indexe Autocomplete */
    void build_autocomplete(const navitia::georef::GeoRef&);

    /** Calcul le score des objectTC */
    void compute_score_autocomplete(navitia::georef::GeoRef&);

    /** Construit l'indexe ProximityList */
    void build_proximity_list();
    void build_admins_stop_areas();
    /// tris les collections et affecte un idx a chaque élément
    void sort();

    size_t nb_stop_times() const {
        size_t nb = 0;
        for (const auto* route: routes) {
            route->for_each_vehicle_journey([&](const nt::VehicleJourney& vj){
                nb += vj.stop_time_list.size();
                return true;
            });
        };
        return nb;
    }

    type::ValidityPattern* get_or_create_validity_pattern(const ValidityPattern& vp_ref);

    /** Retrouve un élément par un attribut arbitraire de type chaine de caractères
      *
      * Le template a été surchargé pour gérer des const char* (string passée comme literal)
      */
    template<class RequestedType>
    std::vector<RequestedType*> find(std::string RequestedType::* attribute, const char * str){
        return find(attribute, std::string(str));
    }

    /** Définis les idx des différents objets */
    void index();

    Indexes
    get_impacts_idx(const std::vector<boost::shared_ptr<disruption::Impact>>& impacts) const;

    const StopPointConnection*
    get_stop_point_connection(const StopPoint& from, const StopPoint& to) const;

    ~PT_Data();

};

}
}
