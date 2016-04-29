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

/** PTReferential permet d'explorer les données en effectuant des filtre sur tout autre objet TC
  *
  * Par exemple on veut vouloir les StopArea qui sont traversés par la ligne 42
  * On appelle "filtre" toute restriction des objets à retourner
  * On peut faire poser des filtres sur n'importe quel objet
  *
  * ptref_graph.cpp contient toutes les relations entre entités : par quelles relations on obtient
  * les stoppoints d'une commune
  *
  * ptreferential.h permet d'analyser les filtres saisis par l'utilisateur. Le parsage se fait avec
  * boost::spirit. Ce n'est pas la lib la plus simple à apprendre, mais elle est performante et puissante
  *
  * Le fonctionnement global est :
  * 1) Pour chaque filtre on trouve les indexes qui correspondent
  * 2) On fait l'intersection des indexes obtenus par chaque filtre
  * 3) On remplit le protobuf
  */

#include "type/type.h"
#include "georef/georef.h"
#include "where.h"
#include "utils/paginate.h"

using navitia::type::Type_e;
namespace navitia{ namespace ptref{

// Filter is something like `stop_area.uri = "kikoolol"` or `vehicle_journey.has_headsign("john")`
struct Filter {
    navitia::type::Type_e navitia_type; // parsed type
    std::string object; // concerned object ("stop_area", "vehicle_journey")
    std::string attribute; // Attribute ("uri")
    Operator_e op; // comparison operator ("=")
    std::string value; // right value compared ("kikoolol") or arg used in method ("john")
    std::string method; // method called (has_headsign)
    std::vector<std::string> args; // method arguments

    Filter(std::string object, std::string attribute, Operator_e op, std::string value):
        object(std::move(object)), attribute(std::move(attribute)), op(op), value(std::move(value)) {}
    Filter(std::string object, std::string value):
        object(std::move(object)), op(HAVING), value(std::move(value)) {}
    Filter(std::string value): object("journey_pattern_point"), op(AFTER), value(std::move(value)) {}
    Filter(std::string object, std::string method, boost::optional<std::vector<std::string>> args):
        object(std::move(object)), method(std::move(method)) {
        if (args) {
            this->args = *args;
        }
    }

    Filter() {}
};

//@TODO inherit from navitia::exception
struct ptref_error : public std::exception {
    std::string more;

    ptref_error(const std::string & more) : more(more) {}
    virtual const char* what() const noexcept override;
};

struct parsing_error : public ptref_error{
    enum error_type {
        global_error ,
        partial_error,
        unknown_object
    };

    error_type type;

    parsing_error(error_type type, const std::string & str) : ptref_error(str), type(type) {}
    parsing_error(const parsing_error&) = default;
    parsing_error& operator=(const parsing_error&) = default;
    ~parsing_error() noexcept;
};

/// Exécute une requête sur les données Data : retourne les idx des objets demandés
type::Indexes make_query(const type::Type_e requested_type,
                                    const std::string& request,
                                    const std::vector<std::string>& forbidden_uris,
                                    const type::OdtLevel_e odt_level,
                                    const boost::optional<boost::posix_time::ptime>& since,
                                    const boost::optional<boost::posix_time::ptime>& until,
                                    const type::Data& data);

type::Indexes make_query(const type::Type_e requested_type,
                                    const std::string& request,
                                    const std::vector<std::string>& forbidden_uris,
                                    const type::Data& data);

type::Indexes make_query(const type::Type_e requested_type,
                                    const std::string& request,
                                    const type::Data& data);


/// Trouve le chemin d'un type de données à un autre
/// Par exemple StopArea → StopPoint → JourneyPatternPoint
std::map<Type_e,Type_e> find_path(Type_e source);

/// À parti d'un élément, on veut retrouver tous ceux de destination
navitia::type::Indexes get(Type_e source, Type_e destination, type::idx_t source_idx, type::PT_Data & data);


std::vector<Filter> parse(std::string request);

type::Indexes get_difference(const type::Indexes&, const type::Indexes&);
type::Indexes get_intersection(const type::Indexes&, const type::Indexes&);

type::Indexes manage_odt_level(const type::Indexes& final_indexes,
                                          const navitia::type::Type_e requested_type,
                                          const navitia::type::OdtLevel_e,
                                          const type::Data & data);
}} //navitia::ptref
