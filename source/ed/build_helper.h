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

#include "utils/functions.h"
#include "type/data.h"
#include <boost/date_time/gregorian_calendar.hpp>
#include "georef/georef.h"
#include "type/meta_data.h"

/** Ce connecteur permet de faciliter la construction d'un réseau en code
 *
 *  Cela est particulièrement (voire exclusivement) utile pour les tests unitaires
 */

namespace ed {

struct builder;

/// Structure retournée à la construction d'un VehicleJourney
struct VJ {
    builder & b;
    navitia::type::VehicleJourney * vj;

    /// Construit un nouveau vehicle journey
    VJ(builder & b, const std::string &line_name, const std::string &validity_pattern, const std::string & block_id, bool is_adapted = true, const std::string& uri="", std::string meta_vj_name = "");

    /// Ajout un nouveau stopTime
    /// Lorsque le depart n'est pas specifié, on suppose que c'est le même qu'à l'arrivée
    /// Si le stopPoint n'est pas connu, on le crée avec un stopArea ayant le même nom
    VJ& operator()(const std::string &stopPoint, int arrivee, int depart = -1, uint16_t local_traffic_zone = std::numeric_limits<uint16_t>::max(),
                   bool drop_off_allowed = true, bool pick_up_allowed = true);

    VJ& operator()(const std::string &stopPoint, const std::string& arrivee, const std::string& depart,
            uint16_t local_traffic_zone = std::numeric_limits<uint16_t>::max(), bool drop_off_allowed = true, bool pick_up_allowed = true);

    /// Transforme les horaires en horaire entre start_time et end_time, toutes les headways secs
    VJ & frequency(uint32_t start_time, uint32_t end_time, uint32_t headway_secs);

};

struct SA {
    builder & b;
    navitia::type::StopArea * sa;

    /// Construit un nouveau stopArea
    SA(builder & b, const std::string & sa_name, double x, double y, bool is_adapted = true);

    /// Construit un stopPoint appartenant au stopArea courant
    SA & operator()(const std::string & sp_name, double x = 0, double y = 0, bool is_adapted = true);
};


struct builder{
    std::map<std::string, navitia::type::Line *> lines;
    std::map<std::string, navitia::type::ValidityPattern *> vps;
    std::map<std::string, navitia::type::StopArea *> sas;
    std::map<std::string, navitia::type::StopPoint *> sps;
    std::map<std::string, navitia::type::Network *> nts;
    std::multimap<std::string, navitia::type::VehicleJourney*> block_vjs;
    boost::gregorian::date begin;

    std::unique_ptr<navitia::type::Data> data = std::make_unique<navitia::type::Data>();
    navitia::georef::GeoRef street_network;


    /// Il faut préciser là date de début des différents validity patterns
    builder(const std::string & date) : begin(boost::gregorian::date_from_iso_string(date)) {
        data->meta->production_date = {begin, begin + boost::gregorian::years(1)};
        data->meta->timezone = "UTC"; //default timezone is UTC
		data->loaded = true;
    }

    /// Crée un vehicle journey
    VJ vj(const std::string &line_name, const std::string &validity_pattern = "11111111", const std::string & block_id="", const bool wheelchair_boarding = true, const std::string& uri="", std::string meta_vj="");
    VJ vj(const std::string &network_name, const std::string &line_name, const std::string &validity_pattern = "11111111", const std::string & block_id="", const bool wheelchair_boarding = true, const std::string& uri="", std::string meta_vj="");

    /// Crée un nouveau stop area
    SA sa(const std::string & name, double x = 0, double y = 0, const bool is_adapted = true);
    SA sa(const std::string & name, navitia::type::GeographicalCoord geo, bool is_adapted = true ){return sa(name,geo.lon(), geo.lat(), is_adapted);}

    /// Crée une connexion
    void connection(const std::string & name1, const std::string & name2, float length);
    void build(navitia::type::PT_Data & pt_data);
    void build_relations(navitia::type::PT_Data & data);
    void build_blocks();
    void finish();
    void generate_dummy_basis();
    void manage_admin();
    void build_autocomplete();
};

}
