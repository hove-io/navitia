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
#include "type/fwd_type.h"
#include "georef/fwd_georef.h"
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
#include "type/timezone_manager.h"
#include <memory>

namespace navitia {
template <>
struct enum_size_trait<pbnavitia::PlaceCodeRequest::Type> {
    static constexpr typename get_enum_type<pbnavitia::PlaceCodeRequest::Type>::type size() { return 8; }
};
namespace type {

typedef std::map<std::string, std::string> code_value_map_type;
using type_code_codes_map_type = std::map<std::string, code_value_map_type>;
class PT_Data : boost::noncopyable {
public:
    PT_Data();

    using StopPointPolygonMap = MultiPolygonMap<const StopPoint*>;

    template <typename T>
    const std::vector<T*>& collection() const {
        static_assert(!std::is_same<T, T>::value, "PT_Data::collection() not implemented");
        static const std::vector<T*> collection;
        return collection;
    }
#define COLLECTION_AND_MAP(type_name, collection_name) \
    std::vector<type_name*> collection_name;           \
    std::unordered_map<std::string, type_name*> collection_name##_map;
    ITERATE_NAVITIA_PT_TYPES(COLLECTION_AND_MAP)
#undef COLLECTION_AND_MAP

    std::vector<StopPointConnection*> stop_point_connections;
    // meta vj factory
    navitia::ObjFactory<MetaVehicleJourney> meta_vjs;

    // associated cal for vj
    std::vector<AssociatedCalendar*> associated_calendars;

    // First letter
    autocomplete::Autocomplete<idx_t> stop_area_autocomplete =
        autocomplete::Autocomplete<idx_t>(navitia::type::Type_e::StopArea);
    autocomplete::Autocomplete<idx_t> stop_point_autocomplete =
        autocomplete::Autocomplete<idx_t>(navitia::type::Type_e::StopPoint);
    autocomplete::Autocomplete<idx_t> line_autocomplete =
        autocomplete::Autocomplete<idx_t>(navitia::type::Type_e::Line);
    autocomplete::Autocomplete<idx_t> network_autocomplete =
        autocomplete::Autocomplete<idx_t>(navitia::type::Type_e::Network);
    autocomplete::Autocomplete<idx_t> mode_autocomplete =
        autocomplete::Autocomplete<idx_t>(navitia::type::Type_e::CommercialMode);
    autocomplete::Autocomplete<idx_t> route_autocomplete =
        autocomplete::Autocomplete<idx_t>(navitia::type::Type_e::Route);

    // Proximity list
    proximitylist::ProximityList<idx_t> stop_area_proximity_list;
    proximitylist::ProximityList<idx_t> stop_point_proximity_list;

    // Message
    disruption::DisruptionHolder disruption_holder;

    std::vector<const StopPoint*> get_stop_points_by_area(const GeographicalCoord& coord);
    void add_stop_point_area(const MultiPolygon& area, StopPoint* sp);

    // Comments container
    Comments comments;

    // Code container
    CodeContainer codes;

    // Headsign handler
    HeadsignHandler headsign_handler;

    // timezone manager
    TimeZoneManager tz_manager;

    template <class Archive>
    void serialize(Archive& ar, const unsigned int);
    /** Construit l'indexe ExternelCode */
    void build_uri();

    /** Construit l'indexe Autocomplete */
    void build_autocomplete(const navitia::georef::GeoRef&);

    /** Calcul le score des objectTC */
    void compute_score_autocomplete(navitia::georef::GeoRef&);

    /** Construit l'indexe ProximityList */
    void build_proximity_list();
    void build_admins_stop_areas();
    /// sort the collections and set the corresponding idx field
    void sort_and_index();

    size_t nb_stop_times() const;

    type::ValidityPattern* get_or_create_validity_pattern(const ValidityPattern& vp_ref);

    type::Network* get_or_create_network(const std::string& uri,
                                         const std::string& name,
                                         int sort = std::numeric_limits<int>::max());
    type::Company* get_or_create_company(const std::string& uri, const std::string& name);
    type::CommercialMode* get_or_create_commercial_mode(const std::string& uri, const std::string& name);
    type::PhysicalMode* get_or_create_physical_mode(const std::string& uri,
                                                    const std::string& name,
                                                    const double co2_emission);
    type::Contributor* get_or_create_contributor(const std::string& uri, const std::string& name);
    type::Dataset* get_or_create_dataset(const std::string& uri,
                                         const std::string& name,
                                         type::Contributor* contributor);
    type::Line* get_or_create_line(const std::string& uri,
                                   const std::string& name,
                                   type::Network* network,
                                   type::CommercialMode* commercial_mode,
                                   int sort = std::numeric_limits<int>::max(),
                                   const std::string& color = {"000000"},
                                   const std::string& text_color = {"FFFFFF"});
    type::Route* get_or_create_route(const std::string& uri,
                                     const std::string& name,
                                     type::Line* line,
                                     type::StopArea* destination = nullptr,
                                     const std::string& direction_type = {});
    const type::TimeZoneHandler* get_main_timezone();
    type::MetaVehicleJourney* get_or_create_meta_vehicle_journey(const std::string& uri,
                                                                 const type::TimeZoneHandler* tz);

    void clean_weak_impacts();

    Indexes get_impacts_idx(const std::vector<boost::shared_ptr<disruption::Impact>>& impacts) const;

    const StopPointConnection* get_stop_point_connection(const StopPoint& from, const StopPoint& to) const;

    ~PT_Data();

private:
    // rtree for zonal stop_points
    std::unique_ptr<StopPointPolygonMap> stop_points_by_area;
};

#define GENERIC_PT_DATA_COLLECTION_SPECIALIZATION(type_name, collection_name) \
    template <>                                                               \
    const std::vector<type_name*>& PT_Data::collection() const;
ITERATE_NAVITIA_PT_TYPES(GENERIC_PT_DATA_COLLECTION_SPECIALIZATION)
#undef GENERIC_PT_DATA_COLLECTION_SPECIALIZATION

}  // namespace type
}  // namespace navitia
