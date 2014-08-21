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
#include "type.h"
#include "georef/georef.h"
#include "type/message.h"
#include "autocomplete/autocomplete.h"
#include "proximity_list/proximity_list.h"

#include <boost/serialization/map.hpp>

namespace navitia { namespace type {

// Maps pour external code
// À refléchir si un hash_map ne serait pas mieux ; pas forcément en lecture car hasher une chaîne c'est plus long que comparer
// En attendant on utilise std::map car on sait le sérialiser...
typedef std::map<std::string, idx_t> ExtCodeMap;

struct PT_Data : boost::noncopyable{
#define COLLECTION_AND_MAP(type_name, collection_name) std::vector<type_name*> collection_name; std::map<std::string, type_name *> collection_name##_map;
    ITERATE_NAVITIA_PT_TYPES(COLLECTION_AND_MAP)

    std::vector<StopTime*> stop_times;
    std::vector<StopPointConnection*> stop_point_connections;

    // meta vj map
    std::map<std::string, MetaVehicleJourney*> meta_vj;

    //associated cal for vj
    std::vector<AssociatedCalendar*> associated_calendars;

    // First letter
    autocomplete::Autocomplete<idx_t> stop_area_autocomplete = autocomplete::Autocomplete<idx_t>(navitia::type::Type_e::StopArea);
    autocomplete::Autocomplete<idx_t> stop_point_autocomplete = autocomplete::Autocomplete<idx_t>(navitia::type::Type_e::StopPoint);
    autocomplete::Autocomplete<idx_t> line_autocomplete = autocomplete::Autocomplete<idx_t>(navitia::type::Type_e::Line);

    // Proximity list
    proximitylist::ProximityList<idx_t> stop_area_proximity_list;
    proximitylist::ProximityList<idx_t> stop_point_proximity_list;

    //Message
    MessageHolder message_holder;

    /** Fonction qui permet de sérialiser (aka binariser la structure de données
      *
      * Elle est appelée par boost et pas directement
      */
    template<class Archive> void serialize(Archive & ar, const unsigned int) {
        ar
        #define SERIALIZE_ELEMENTS(type_name, collection_name) & collection_name & collection_name##_map
                ITERATE_NAVITIA_PT_TYPES(SERIALIZE_ELEMENTS)
                & stop_times
                // the first letters for autocomplete
                & stop_area_autocomplete & stop_point_autocomplete & line_autocomplete
                // the proximity lists
                & stop_area_proximity_list & stop_point_proximity_list
                // custom types
                & stop_point_connections
                //messages
                & message_holder
                //the meta vjs
                & meta_vj;
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

    /** Retrouve un élément par un attribut arbitraire de type chaine de caractères
      *
      * Le template a été surchargé pour gérer des const char* (string passée comme literal)
      */
    template<class RequestedType>
    std::vector<RequestedType*> find(std::string RequestedType::* attribute, const char * str){
        return find(attribute, std::string(str));
    }

    PT_Data& operator=(PT_Data&& other);

    /** Définis les idx des différents objets */
    void index();

    ~PT_Data();

};

}}
