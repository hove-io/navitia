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

#include "pt_data.h"

#include "georef/adminref.h"
#include "georef/georef.h"
#include "type/serialization.h"
#include "type/connection.h"
#include "type/calendar.h"
#include "type/company.h"
#include "type/contributor.h"
#include "type/dataset.h"
#include "type/network.h"
#include "type/base_pt_objects.h"
#include "type/meta_vehicle_journey.h"
#include "type/multi_polygon_map.h"
#include "type/commercial_mode.h"
#include "type/physical_mode.h"
#include "utils/functions.h"

#include <boost/range/algorithm/find_if.hpp>

namespace nt = navitia::type;

namespace navitia {
namespace type {

template <class Archive>
void PT_Data::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar
#define SERIALIZE_ELEMENTS(type_name, collection_name) &collection_name& collection_name##_map
            ITERATE_NAVITIA_PT_TYPES(SERIALIZE_ELEMENTS)
        & stop_area_autocomplete& stop_point_autocomplete& line_autocomplete& network_autocomplete& mode_autocomplete&
              route_autocomplete& stop_area_proximity_list& stop_point_proximity_list& stop_point_connections&
                  disruption_holder& meta_vjs& stop_points_by_area& comments& codes& headsign_handler& tz_manager;
}
SERIALIZABLE(PT_Data)

size_t PT_Data::nb_stop_times() const {
    size_t nb = 0;
    for (const auto* route : routes) {
        route->for_each_vehicle_journey([&](const nt::VehicleJourney& vj) {
            nb += vj.stop_time_list.size();
            return true;
        });
    };
    return nb;
}

type::Network* PT_Data::get_or_create_network(const std::string& uri, const std::string& name, int sort) {
    const auto it = networks_map.find(uri);
    if (it != networks_map.end()) {
        return it->second;
    }

    auto* network = new nt::Network();
    network->uri = uri;
    network->name = name;
    network->sort = sort;

    network->idx = networks.size();
    networks.push_back(network);
    networks_map[uri] = network;

    return network;
}

type::Company* PT_Data::get_or_create_company(const std::string& uri, const std::string& name) {
    auto company_it = companies_map.find(uri);

    if (company_it != companies_map.end())
        return company_it->second;

    auto* company = new navitia::type::Company();
    company->idx = companies.size();
    company->name = name;
    company->uri = uri;
    companies.push_back(company);
    companies_map.insert({uri, company});

    return company;
}

type::CommercialMode* PT_Data::get_or_create_commercial_mode(const std::string& uri, const std::string& name) {
    const auto it = commercial_modes_map.find(uri);
    if (it != commercial_modes_map.end()) {
        return it->second;
    }

    auto* mode = new nt::CommercialMode();
    mode->uri = uri;
    mode->name = name;

    mode->idx = commercial_modes.size();
    commercial_modes.push_back(mode);
    commercial_modes_map[uri] = mode;

    return mode;
}

type::PhysicalMode* PT_Data::get_or_create_physical_mode(const std::string& uri,
                                                         const std::string& name,
                                                         const double co2_emission) {
    const auto pmode_it = physical_modes_map.find(uri);

    if (pmode_it != physical_modes_map.end())
        return pmode_it->second;

    auto* mode = new navitia::type::PhysicalMode();
    mode->name = name;
    mode->uri = uri;

    if (co2_emission >= 0.f)
        mode->co2_emission = co2_emission;

    mode->idx = physical_modes.size();
    physical_modes.push_back(mode);
    physical_modes_map.insert({mode->uri, mode});

    return mode;
}

type::Dataset* PT_Data::get_or_create_dataset(const std::string& uri,
                                              const std::string& name,
                                              type::Contributor* contributor) {
    const auto dataset_it = datasets_map.find(uri);

    if (dataset_it != datasets_map.end())
        return dataset_it->second;

    auto* dataset = new navitia::type::Dataset();
    dataset->idx = datasets.size();
    dataset->uri = uri;
    dataset->name = name;
    dataset->contributor = contributor;
    contributor->dataset_list.insert(dataset);
    datasets.push_back(dataset);
    datasets_map.insert({dataset->uri, dataset});

    return dataset;
}

type::Contributor* PT_Data::get_or_create_contributor(const std::string& uri, const std::string& name) {
    const auto contrib_it = contributors_map.find(uri);

    if (contrib_it != contributors_map.end())
        return contrib_it->second;

    auto* contributor = new navitia::type::Contributor();
    contributor->uri = uri;
    contributor->name = name;

    contributor->idx = contributors.size();
    contributors.push_back(contributor);
    contributors_map.insert({contributor->uri, contributor});

    return contributor;
}

type::Line* PT_Data::get_or_create_line(const std::string& uri,
                                        const std::string& name,
                                        type::Network* network,
                                        type::CommercialMode* commercial_mode,
                                        int sort,
                                        const std::string& color,
                                        const std::string& text_color) {
    const auto it = lines_map.find(uri);
    if (it != lines_map.end()) {
        return it->second;
    }

    auto* line = new nt::Line();
    line->uri = uri;
    line->name = name;
    line->sort = sort;
    line->color = color;
    line->text_color = text_color;

    line->network = network;
    network->line_list.push_back(line);
    line->commercial_mode = commercial_mode;
    commercial_mode->line_list.push_back(line);

    line->idx = lines.size();
    lines.push_back(line);
    lines_map[uri] = line;

    return line;
}

type::Route* PT_Data::get_or_create_route(const std::string& uri,
                                          const std::string& name,
                                          type::Line* line,
                                          type::StopArea* destination,
                                          const std::string& direction_type) {
    const auto it = routes_map.find(uri);
    if (it != routes_map.end()) {
        return it->second;
    }

    auto* route = new nt::Route();
    route->uri = uri;
    route->name = name;
    route->destination = destination;
    route->direction_type = direction_type;

    route->line = line;
    line->route_list.push_back(route);

    route->idx = routes.size();
    routes.push_back(route);
    routes_map[uri] = route;

    return route;
}

const type::TimeZoneHandler* PT_Data::get_main_timezone() {
    // using TZ already used by MetaVJ as they all use the same when reading base_schedule data
    if (meta_vjs.size() > 0) {
        return meta_vjs.begin()->get()->tz_handler;
    }
    return tz_manager.get_first_timezone();
}

type::MetaVehicleJourney* PT_Data::get_or_create_meta_vehicle_journey(const std::string& uri,
                                                                      const type::TimeZoneHandler* tz) {
    auto* mvj = meta_vjs.get_or_create(uri);
    mvj->tz_handler = tz;

    return mvj;
}

ValidityPattern* PT_Data::get_or_create_validity_pattern(const ValidityPattern& vp_ref) {
    for (auto vp : validity_patterns) {
        if (vp->days == vp_ref.days && vp->beginning_date == vp_ref.beginning_date) {
            return vp;
        }
    }
    auto vp = new nt::ValidityPattern();
    vp->idx = validity_patterns.size();
    vp->uri = make_adapted_uri(vp->uri);
    vp->beginning_date = vp_ref.beginning_date;
    vp->days = vp_ref.days;
    validity_patterns.push_back(vp);
    validity_patterns_map[vp->uri] = vp;
    return vp;
}

void PT_Data::sort_and_index() {
#define SORT_AND_INDEX(type_name, collection_name)                            \
    std::stable_sort(collection_name.begin(), collection_name.end(), Less()); \
    std::for_each(collection_name.begin(), collection_name.end(), Indexer<nt::idx_t>());
    ITERATE_NAVITIA_PT_TYPES(SORT_AND_INDEX)
#undef SORT_AND_INDEX

    std::stable_sort(stop_point_connections.begin(), stop_point_connections.end());
    std::for_each(stop_point_connections.begin(), stop_point_connections.end(), Indexer<idx_t>());
}

void PT_Data::build_autocomplete(const navitia::georef::GeoRef& georef) {
    this->stop_area_autocomplete.clear();
    for (const StopArea* sa : this->stop_areas) {
        // Don't add it to the dictionnary if name is empty
        if ((!sa->name.empty()) && (sa->visible)) {
            std::string key;
            for (navitia::georef::Admin* admin : sa->admin_list) {
                if (admin->level == 8) {
                    key += " " + admin->name;
                }
            }
            this->stop_area_autocomplete.add_string(sa->name + key, sa->idx, georef.ghostwords, georef.synonyms);
        }
    }
    this->stop_area_autocomplete.build();

    this->stop_point_autocomplete.clear();
    for (const StopPoint* sp : this->stop_points) {
        // Don't add it to the dictionnary if name is empty
        if ((!sp->name.empty()) && ((sp->stop_area == nullptr) || (sp->stop_area->visible))) {
            std::string key;
            for (navitia::georef::Admin* admin : sp->admin_list) {
                if (admin->level == 8) {
                    key += key + " " + admin->name;
                }
            }
            this->stop_point_autocomplete.add_string(sp->name + key, sp->idx, georef.ghostwords, georef.synonyms);
        }
    }
    this->stop_point_autocomplete.build();

    this->line_autocomplete.clear();
    for (const Line* line : this->lines) {
        if (!line->name.empty()) {
            std::string key;
            key = line->code;
            if (line->network) {
                if (!key.empty()) {
                    key += " ";
                }
                key += line->network->name;
            }
            if (line->commercial_mode) {
                if (!key.empty()) {
                    key += " ";
                }
                key += line->commercial_mode->name;
            }
            this->line_autocomplete.add_string(key + " " + line->name, line->idx, georef.ghostwords, georef.synonyms);
        }
    }
    this->line_autocomplete.build();

    this->network_autocomplete.clear();
    for (const Network* network : this->networks) {
        if (!network->name.empty()) {
            this->network_autocomplete.add_string(network->name, network->idx, georef.ghostwords, georef.synonyms);
        }
    }
    this->network_autocomplete.build();

    this->mode_autocomplete.clear();
    for (const CommercialMode* mode : this->commercial_modes) {
        if (!mode->name.empty()) {
            this->mode_autocomplete.add_string(mode->name, mode->idx, georef.ghostwords, georef.synonyms);
        }
    }
    this->mode_autocomplete.build();

    this->route_autocomplete.clear();
    for (const Route* route : this->routes) {
        if (!route->name.empty()) {
            std::string key;
            if (route->line) {
                if (route->line->network) {
                    key = route->line->network->name;
                }
                if (route->line->commercial_mode) {
                    if (!key.empty()) {
                        key += " ";
                    }
                    key += route->line->commercial_mode->name;
                }
                if (!key.empty()) {
                    key += " ";
                }
                key += route->line->code;
            }
            this->route_autocomplete.add_string(key + " " + route->name, route->idx, georef.ghostwords,
                                                georef.synonyms);
        }
    }
    this->route_autocomplete.build();
}

void PT_Data::compute_score_autocomplete(navitia::georef::GeoRef& georef) {
    // Compute admin score using stop_point count in each admin
    georef.fl_admin.compute_score((*this), georef, type::Type_e::Admin);
    // use the score of each admin for it's objects like "POI", "way" and "stop_point"
    georef.fl_way.compute_score((*this), georef, type::Type_e::Way);
    georef.fl_poi.compute_score((*this), georef, type::Type_e::POI);
    this->stop_point_autocomplete.compute_score((*this), georef, type::Type_e::StopPoint);
    // Compute stop_area score using it's stop_point count
    this->stop_area_autocomplete.compute_score((*this), georef, type::Type_e::StopArea);
}

void PT_Data::build_proximity_list() {
    this->stop_area_proximity_list.clear();
    for (const StopArea* stop_area : this->stop_areas) {
        this->stop_area_proximity_list.add(stop_area->coord, stop_area->idx);
    }
    this->stop_area_proximity_list.build();

    this->stop_point_proximity_list.clear();
    for (const StopPoint* stop_point : this->stop_points) {
        this->stop_point_proximity_list.add(stop_point->coord, stop_point->idx);
    }
    this->stop_point_proximity_list.build();
}

void PT_Data::build_admins_stop_areas() {
    for (navitia::type::StopPoint* stop_point : this->stop_points) {
        if (!stop_point->stop_area) {
            continue;
        }
        for (navitia::georef::Admin* admin : stop_point->admin_list) {
            auto find_predicate = [&](navitia::georef::Admin* adm) { return adm->idx == admin->idx; };
            auto it = std::find_if(stop_point->stop_area->admin_list.begin(), stop_point->stop_area->admin_list.end(),
                                   find_predicate);
            if (it == stop_point->stop_area->admin_list.end()) {
                stop_point->stop_area->admin_list.push_back(admin);
            }
        }
    }
}

void PT_Data::build_uri() {
#define NORMALIZE_EXT_CODE(type_name, collection_name) \
    for (auto element : collection_name)               \
        collection_name##_map[element->uri] = element;
    ITERATE_NAVITIA_PT_TYPES(NORMALIZE_EXT_CODE)
}

/** Foncteur fixe le membre "idx" d'un objet en incrémentant toujours de 1
 *
 * Cela permet de numéroter tous les objets de 0 à n-1 d'un vecteur de pointeurs
 */
struct Indexer {
    idx_t idx{0};
    Indexer() = default;

    template <class T>
    void operator()(T* obj) {
        obj->idx = idx;
        idx++;
    }
};

void PT_Data::clean_weak_impacts() {
    for (const auto& obj : stop_points) {
        obj->clean_weak_impacts();
    }
    for (const auto& obj : stop_areas) {
        obj->clean_weak_impacts();
    }
    for (const auto& obj : networks) {
        obj->clean_weak_impacts();
    }
    for (const auto& obj : lines) {
        obj->clean_weak_impacts();
    }
    for (const auto& obj : routes) {
        obj->clean_weak_impacts();
    }
    for (const auto& obj : meta_vjs) {
        obj->clean_weak_impacts();
    }
}

Indexes PT_Data::get_impacts_idx(const std::vector<boost::shared_ptr<disruption::Impact>>& impacts) const {
    Indexes result;
    idx_t i = 0;
    const auto& impacts_pool = disruption_holder.get_weak_impacts();
    for (const auto& impact : impacts_pool) {
        auto impact_sptr = impact.lock();
        assert(impact_sptr);
        if (navitia::contains(impacts, impact_sptr)) {
            result.insert(i);  // TODO use bulk insert ?
        }
        ++i;
    }
    return result;
}

const StopPointConnection* PT_Data::get_stop_point_connection(const StopPoint& from, const StopPoint& to) const {
    const auto& connections = from.stop_point_connection_list;
    auto is_the_one = [&](const type::StopPointConnection* conn) {
        return &from == conn->departure && &to == conn->destination;
    };
    const auto search = boost::find_if(connections, is_the_one);
    if (search == connections.end()) {
        return nullptr;
    }
    return *search;
}

std::vector<const StopPoint*> PT_Data::get_stop_points_by_area(const GeographicalCoord& coord) {
    return stop_points_by_area->find(coord);
}

void PT_Data::add_stop_point_area(const MultiPolygon& area, StopPoint* sp) {
    stop_points_by_area->insert(area, sp);
}

PT_Data::PT_Data() : stop_points_by_area(std::make_unique<StopPointPolygonMap>()) {}

PT_Data::~PT_Data() {
    // big uggly hack :(
    // the vj are objects owned by the jp,
    // TODO change all pt object (but vj and st) to unique ptr!
    vehicle_journeys.clear();

#define DELETE_PTDATA(type_name, collection_name) \
    std::for_each(collection_name.begin(), collection_name.end(), [](type_name* obj) { delete obj; });
    ITERATE_NAVITIA_PT_TYPES(DELETE_PTDATA)

    for (auto conn : stop_point_connections) {
        delete conn;
    }
    for (auto cal : associated_calendars) {
        delete cal;
    }
}

#define GENERIC_PT_DATA_COLLECTION_SPECIALIZATION(type_name, collection_name) \
    template <>                                                               \
    const std::vector<type_name*>& PT_Data::collection() const {              \
        return collection_name;                                               \
    }
ITERATE_NAVITIA_PT_TYPES(GENERIC_PT_DATA_COLLECTION_SPECIALIZATION)
#undef GENERIC_PT_DATA_COLLECTION_SPECIALIZATION

}  // namespace type
}  // namespace navitia
