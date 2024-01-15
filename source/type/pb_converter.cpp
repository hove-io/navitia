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

#include "pb_converter.h"

#include "fare/fare.h"
#include "georef/georef.h"
#include "georef/street_network.h"
#include "ptreferential/ptreferential.h"
#include "routing/dataraptor.h"
#include "time_tables/thermometer.h"
#include "type/geographical_coord.h"
#include "type/type_utils.h"
#include "type/access_point.h"
#include "utils/exception.h"
#include "utils/exception.h"
#include "utils/functions.h"
#include "type/type_utils.h"

#include <boost/algorithm/cxx11/none_of.hpp>
#include <boost/date_time/date_defs.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/algorithms/length.hpp>
#include <boost/lexical_cast.hpp>

#include <functional>

namespace gd = boost::gregorian;
namespace nd = navitia::type::disruption;

namespace navitia {

struct PbCreator::Filler::PtObjVisitor : public boost::static_visitor<> {
    const nd::Impact& impact;
    pbnavitia::Impact* pb_impact;

    Filler& filler;
    PtObjVisitor(const nd::Impact& impact, pbnavitia::Impact* pb_impact, PbCreator::Filler& filler)
        : impact(impact), pb_impact(pb_impact), filler(filler) {}
    template <typename NavitiaPTObject>
    pbnavitia::ImpactedObject* add_pt_object(const NavitiaPTObject* bo) const {
        auto* pobj = pb_impact->add_impacted_objects();

        filler.copy(0, DumpMessage::No).fill_pb_object(bo, pobj->mutable_pt_object());
        return pobj;
    }
    template <typename NavitiaPTObject>
    void operator()(const NavitiaPTObject* bo) const {
        add_pt_object(bo);
    }
    void operator()(const nd::LineSection& line_section) const {
        // a line section is displayed as a disruption on a line with additional information
        auto* pobj = add_pt_object(line_section.line);

        auto* impacted_section = pobj->mutable_impacted_section();

        filler.copy(0, DumpMessage::No).fill_pb_object(line_section.start_point, impacted_section->mutable_from());

        filler.copy(0, DumpMessage::No).fill_pb_object(line_section.end_point, impacted_section->mutable_to());
        filler.copy(0, DumpMessage::No).fill(line_section.routes, impacted_section->mutable_routes());
    }
    void operator()(const nd::RailSection& rail_section) const {
        // a line section is displayed as a disruption on a line with additional information
        auto* pobj = add_pt_object(rail_section.line);

        auto* impacted_rail_section = pobj->mutable_impacted_rail_section();

        filler.copy(0, DumpMessage::No).fill_pb_object(rail_section.start, impacted_rail_section->mutable_from());
        filler.copy(0, DumpMessage::No).fill_pb_object(rail_section.end, impacted_rail_section->mutable_to());

        std::vector<navitia::type::Route*> routes;
        if (rail_section.routes.empty()) {
            routes = rail_section.line->route_list;
        } else {
            routes = rail_section.routes;
        }
        filler.copy(0, DumpMessage::No).fill(routes, impacted_rail_section->mutable_routes());
    }

    pbnavitia::StopTimeUpdateStatus get_effect(nd::StopTimeUpdate::Status status) const {
        switch (status) {
            case nd::StopTimeUpdate::Status::ADDED:
            case nd::StopTimeUpdate::Status::ADDED_FOR_DETOUR:
                return pbnavitia::ADDED;
            case nd::StopTimeUpdate::Status::DELAYED:
                return pbnavitia::DELAYED;
            case nd::StopTimeUpdate::Status::DELETED:
            case nd::StopTimeUpdate::Status::DELETED_FOR_DETOUR:
                return pbnavitia::DELETED;
            case nd::StopTimeUpdate::Status::UNCHANGED:
            default:
                return pbnavitia::UNCHANGED;
        }
    }

    bool is_detour(nd::StopTimeUpdate::Status dep_status, nd::StopTimeUpdate::Status arr_status) const {
        return (
            contains({nd::StopTimeUpdate::Status::ADDED_FOR_DETOUR, nd::StopTimeUpdate::Status::DELETED_FOR_DETOUR},
                     dep_status)
            || contains({nd::StopTimeUpdate::Status::ADDED_FOR_DETOUR, nd::StopTimeUpdate::Status::DELETED_FOR_DETOUR},
                        arr_status));
    }

    void operator()(const nd::UnknownPtObj& /*unused*/) const {}
    void operator()(const nt::MetaVehicleJourney* mvj) const {
        auto* pobj = add_pt_object(mvj);

        // for meta vj we also need to output the impacted stoptimes
        for (const auto& stu : impact.aux_info.stop_times) {
            auto* impacted_stop = pobj->add_impacted_stops();
            impacted_stop->set_cause(stu.cause);
            impacted_stop->set_is_detour(is_detour(stu.departure_status, stu.arrival_status));

            impacted_stop->set_departure_status(get_effect(stu.departure_status));
            impacted_stop->set_arrival_status(get_effect(stu.arrival_status));

            // for retrocompatibility we set the global 'effect', we set it to the most important status
            impacted_stop->set_effect(get_effect(std::max(stu.departure_status, stu.arrival_status)));

            filler.copy(0, DumpMessage::No)
                .fill_pb_object(stu.stop_time.stop_point, impacted_stop->mutable_stop_point());

            filler.copy(0, DumpMessage::No).fill_pb_object(&stu, impacted_stop->mutable_amended_stop_time());

            // we need to get the base stoptime
            const auto* base_st = stu.stop_time.get_base_stop_time();
            if (base_st) {
                filler.copy(0, DumpMessage::No).fill_pb_object(base_st, impacted_stop->mutable_base_stop_time());
            }
        }
    }
};

static bool is_principal_destination(const nt::StopTime* stop_time) {
    return stop_time->vehicle_journey && stop_time->vehicle_journey->route
           && stop_time->vehicle_journey->route->destination && !stop_time->vehicle_journey->stop_time_list.empty()
           && stop_time->vehicle_journey->stop_time_list.back().stop_point
           && stop_time->vehicle_journey->route->destination
                  == stop_time->vehicle_journey->stop_time_list.back().stop_point->stop_area;
}

namespace {
template <typename Pb>
struct NavitiaToProto;
template <>
struct NavitiaToProto<nt::Line> {
    using type = pbnavitia::Line;
};
template <>
struct NavitiaToProto<nt::ValidityPattern> {
    using type = pbnavitia::ValidityPattern;
};
template <>
struct NavitiaToProto<nt::StopArea> {
    using type = pbnavitia::StopArea;
};
template <>
struct NavitiaToProto<nt::StopPoint> {
    using type = pbnavitia::StopPoint;
};
template <>
struct NavitiaToProto<nt::Network> {
    using type = pbnavitia::Network;
};
template <>
struct NavitiaToProto<nt::Route> {
    using type = pbnavitia::Route;
};
template <>
struct NavitiaToProto<nt::Company> {
    using type = pbnavitia::Company;
};
template <>
struct NavitiaToProto<nt::CommercialMode> {
    using type = pbnavitia::CommercialMode;
};
template <>
struct NavitiaToProto<nt::PhysicalMode> {
    using type = pbnavitia::PhysicalMode;
};
template <>
struct NavitiaToProto<nt::LineGroup> {
    using type = pbnavitia::LineGroup;
};
template <>
struct NavitiaToProto<ng::POI> {
    using type = pbnavitia::Poi;
};
template <>
struct NavitiaToProto<ng::POIType> {
    using type = pbnavitia::PoiType;
};
template <>
struct NavitiaToProto<nt::VehicleJourney> {
    using type = pbnavitia::VehicleJourney;
};
template <>
struct NavitiaToProto<nt::Calendar> {
    using type = pbnavitia::Calendar;
};
template <>
struct NavitiaToProto<nt::Contributor> {
    using type = pbnavitia::Contributor;
};
template <>
struct NavitiaToProto<nt::Dataset> {
    using type = pbnavitia::Dataset;
};
template <>
struct NavitiaToProto<nt::StopPointConnection> {
    using type = pbnavitia::Connection;
};
template <>
struct NavitiaToProto<nt::MetaVehicleJourney> {
    using type = pbnavitia::Trip;
};

template <typename T>
using NavToProtoCollec = ::google::protobuf::RepeatedPtrField<typename NavitiaToProto<T>::type>*;

template <typename Pb>
NavToProtoCollec<Pb> get_mutable(pbnavitia::Response& resp);
template <>
NavToProtoCollec<nt::Line> get_mutable<nt::Line>(pbnavitia::Response& resp) {
    return resp.mutable_lines();
}
template <>
NavToProtoCollec<nt::ValidityPattern> get_mutable<nt::ValidityPattern>(pbnavitia::Response& resp) {
    return resp.mutable_validity_patterns();
}
template <>
NavToProtoCollec<nt::StopPoint> get_mutable<nt::StopPoint>(pbnavitia::Response& resp) {
    return resp.mutable_stop_points();
}
template <>
NavToProtoCollec<nt::StopArea> get_mutable<nt::StopArea>(pbnavitia::Response& resp) {
    return resp.mutable_stop_areas();
}
template <>
NavToProtoCollec<nt::Network> get_mutable<nt::Network>(pbnavitia::Response& resp) {
    return resp.mutable_networks();
}
template <>
NavToProtoCollec<nt::Route> get_mutable<nt::Route>(pbnavitia::Response& resp) {
    return resp.mutable_routes();
}
template <>
NavToProtoCollec<nt::Company> get_mutable<nt::Company>(pbnavitia::Response& resp) {
    return resp.mutable_companies();
}
template <>
NavToProtoCollec<nt::PhysicalMode> get_mutable<nt::PhysicalMode>(pbnavitia::Response& resp) {
    return resp.mutable_physical_modes();
}
template <>
NavToProtoCollec<nt::CommercialMode> get_mutable<nt::CommercialMode>(pbnavitia::Response& resp) {
    return resp.mutable_commercial_modes();
}
template <>
NavToProtoCollec<nt::LineGroup> get_mutable<nt::LineGroup>(pbnavitia::Response& resp) {
    return resp.mutable_line_groups();
}
template <>
NavToProtoCollec<ng::POI> get_mutable<ng::POI>(pbnavitia::Response& resp) {
    return resp.mutable_pois();
}
template <>
NavToProtoCollec<ng::POIType> get_mutable<ng::POIType>(pbnavitia::Response& resp) {
    return resp.mutable_poi_types();
}
template <>
NavToProtoCollec<nt::VehicleJourney> get_mutable<nt::VehicleJourney>(pbnavitia::Response& resp) {
    return resp.mutable_vehicle_journeys();
}
template <>
NavToProtoCollec<nt::Calendar> get_mutable<nt::Calendar>(pbnavitia::Response& resp) {
    return resp.mutable_calendars();
}
template <>
NavToProtoCollec<nt::Contributor> get_mutable<nt::Contributor>(pbnavitia::Response& resp) {
    return resp.mutable_contributors();
}
template <>
NavToProtoCollec<nt::Dataset> get_mutable<nt::Dataset>(pbnavitia::Response& resp) {
    return resp.mutable_datasets();
}
template <>
NavToProtoCollec<nt::StopPointConnection> get_mutable<nt::StopPointConnection>(pbnavitia::Response& resp) {
    return resp.mutable_connections();
}

pbnavitia::AdministrativeRegion* get_sub_object(const ng::Admin* /*unused*/, pbnavitia::PtObject* pt_object) {
    return pt_object->mutable_administrative_region();
}

pbnavitia::Calendar* get_sub_object(const nt::Calendar* /*unused*/, pbnavitia::PtObject* pt_object) {
    return pt_object->mutable_calendar();
}
template <typename PB>
pbnavitia::CommercialMode* get_sub_object(const nt::CommercialMode* /*unused*/, PB* pt_object) {
    return pt_object->mutable_commercial_mode();
}
template <typename PB>
pbnavitia::Company* get_sub_object(const nt::Company* /*unused*/, PB* pt_object) {
    return pt_object->mutable_company();
}
template <typename PB>
pbnavitia::Contributor* get_sub_object(const nt::Contributor* /*unused*/, PB* pt_object) {
    return pt_object->mutable_contributor();
}
template <typename PB>
pbnavitia::Address* get_sub_object(const nt::GeographicalCoord* /*unused*/, PB* pt_object) {
    return pt_object->mutable_address();
}
template <typename PB>
pbnavitia::Address* get_sub_object(const ng::Address* /*unused*/, PB* pt_object) {
    return pt_object->mutable_address();
}

template <typename PB>
pbnavitia::JourneyPattern* get_sub_object(const jp_pair* /*unused*/, PB* pt_object) {
    return pt_object->mutable_journey_pattern();
}
template <typename PB>
pbnavitia::JourneyPatternPoint* get_sub_object(const jpp_pair* /*unused*/, PB* pt_object) {
    return pt_object->mutable_journey_pattern_point();
}
template <typename PB>
pbnavitia::Line* get_sub_object(const nt::Line* /*unused*/, PB* pt_object) {
    return pt_object->mutable_line();
}
template <typename PB>
pbnavitia::MultiLineString* get_sub_object(const nt::MultiLineString* /*unused*/, PB* pt_object) {
    return pt_object->mutable_geojson();
}
template <typename PB>
pbnavitia::Network* get_sub_object(const nt::Network* /*unused*/, PB* pt_object) {
    return pt_object->mutable_network();
}
template <typename PB>
pbnavitia::PhysicalMode* get_sub_object(const nt::PhysicalMode* /*unused*/, PB* pt_object) {
    return pt_object->mutable_physical_mode();
}
template <typename PB>
pbnavitia::Poi* get_sub_object(const ng::POI* /*unused*/, PB* pt_object) {
    return pt_object->mutable_poi();
}
template <typename PB>
pbnavitia::PoiType* get_sub_object(const ng::POIType* /*unused*/, PB* pt_object) {
    return pt_object->mutable_poi_type();
}
template <typename PB>
pbnavitia::Route* get_sub_object(const nt::Route* /*unused*/, PB* pt_object) {
    return pt_object->mutable_route();
}
template <typename PB>
pbnavitia::StopArea* get_sub_object(const nt::StopArea* /*unused*/, PB* pt_object) {
    return pt_object->mutable_stop_area();
}
template <typename PB>
pbnavitia::StopPoint* get_sub_object(const nt::StopPoint* /*unused*/, PB* pt_object) {
    return pt_object->mutable_stop_point();
}
template <typename PB>
pbnavitia::Trip* get_sub_object(const nt::MetaVehicleJourney* /*unused*/, PB* pt_object) {
    return pt_object->mutable_trip();
}
template <typename PB>
pbnavitia::VehicleJourney* get_sub_object(const nt::VehicleJourney* /*unused*/, PB* pt_object) {
    return pt_object->mutable_vehicle_journey();
}

template <typename T>
pbnavitia::NavitiaType get_pb_type();
template <>
pbnavitia::NavitiaType get_pb_type<nt::Calendar>() {
    return pbnavitia::CALENDAR;
}
template <>
pbnavitia::NavitiaType get_pb_type<nt::VehicleJourney>() {
    return pbnavitia::VEHICLE_JOURNEY;
}
template <>
pbnavitia::NavitiaType get_pb_type<nt::Line>() {
    return pbnavitia::LINE;
}
template <>
pbnavitia::NavitiaType get_pb_type<nt::Route>() {
    return pbnavitia::ROUTE;
}
template <>
pbnavitia::NavitiaType get_pb_type<nt::Company>() {
    return pbnavitia::COMPANY;
}
template <>
pbnavitia::NavitiaType get_pb_type<nt::Network>() {
    return pbnavitia::NETWORK;
}
template <>
pbnavitia::NavitiaType get_pb_type<ng::POI>() {
    return pbnavitia::POI;
}
template <>
pbnavitia::NavitiaType get_pb_type<ng::Admin>() {
    return pbnavitia::ADMINISTRATIVE_REGION;
}
template <>
pbnavitia::NavitiaType get_pb_type<nt::StopArea>() {
    return pbnavitia::STOP_AREA;
}
template <>
pbnavitia::NavitiaType get_pb_type<nt::StopPoint>() {
    return pbnavitia::STOP_POINT;
}
template <>
pbnavitia::NavitiaType get_pb_type<nt::CommercialMode>() {
    return pbnavitia::COMMERCIAL_MODE;
}
template <>
pbnavitia::NavitiaType get_pb_type<nt::MetaVehicleJourney>() {
    return pbnavitia::TRIP;
}
}  // anonymous namespace

template <typename Target, typename Source>
std::vector<Target*> PbCreator::Filler::ptref_indexes(const Source* nav_obj) {
    return navitia::ptref_indexes<Target, Source>(nav_obj, *pb_creator.data);
}

template <typename T>
void PbCreator::Filler::add_contributor(const T* nav) {
    if (pb_creator.disable_feedpublisher) {
        return;
    }
    const auto& contributors = ptref_indexes<nt::Contributor>(nav);
    for (const nt::Contributor* c : contributors) {
        if (!c->license.empty()) {
            pb_creator.contributors.insert(c);
        }
    }
}

template <typename NAV, typename PB>
void PbCreator::Filler::fill(NAV* nav_object, PB* pb_object) {
    if (nav_object == nullptr) {
        return;
    }
    DumpMessageOptions new_dump_options{dump_message_options};
    new_dump_options.dump_line_section = DumpLineSectionMessage::No;
    new_dump_options.dump_rail_section = DumpRailSectionMessage::No;
    copy(depth - 1, new_dump_options).fill_pb_object(nav_object, get_sub_object(nav_object, pb_object));
}
template <typename NAV, typename F>
void PbCreator::Filler::fill_with_creator(NAV* nav_object, F creator) {
    if (nav_object == nullptr) {
        return;
    }
    copy(depth - 1, dump_message_options).fill_pb_object(nav_object, creator());
}

template <typename T>
void PbCreator::Filler::fill_pb_object(const T* value, pbnavitia::PtObject* pt_object) {
    if (value == nullptr) {
        return;
    }

    fill_pb_object(value, get_sub_object(value, pt_object));
    pt_object->set_name(get_label(value));
    pt_object->set_uri(value->uri);
    pt_object->set_embedded_type(get_pb_type<T>());
}
template void PbCreator::Filler::fill_pb_object<georef::Admin>(const georef::Admin*, pbnavitia::PtObject*);
template void PbCreator::Filler::fill_pb_object<nt::Calendar>(const nt::Calendar*, pbnavitia::PtObject*);
template void PbCreator::Filler::fill_pb_object<nt::Company>(const nt::Company*, pbnavitia::PtObject*);
template void PbCreator::Filler::fill_pb_object<nt::CommercialMode>(const nt::CommercialMode*, pbnavitia::PtObject*);
template void PbCreator::Filler::fill_pb_object<nt::Line>(const nt::Line*, pbnavitia::PtObject*);
template void PbCreator::Filler::fill_pb_object<nt::Network>(const nt::Network*, pbnavitia::PtObject*);
template void PbCreator::Filler::fill_pb_object<nt::Route>(const nt::Route*, pbnavitia::PtObject*);
template void PbCreator::Filler::fill_pb_object<nt::StopArea>(const nt::StopArea*, pbnavitia::PtObject*);
template void PbCreator::Filler::fill_pb_object<nt::StopPoint>(const nt::StopPoint*, pbnavitia::PtObject*);
template void PbCreator::Filler::fill_pb_object<nt::VehicleJourney>(const nt::VehicleJourney*, pbnavitia::PtObject*);

PbCreator::Filler PbCreator::Filler::copy(int depth, const DumpMessageOptions& dump_message_options) {
    if (depth <= 0) {
        return {0, dump_message_options, pb_creator};
    }
    return {depth, dump_message_options, pb_creator};
}

void PbCreator::Filler::fill_pb_object(const nt::Contributor* cb, pbnavitia::Contributor* contrib) {
    contrib->set_uri(cb->uri);
    contrib->set_name(cb->name);
    contrib->set_license(cb->license);
    contrib->set_website(cb->website);
}

void PbCreator::Filler::fill_pb_object(const nt::Dataset* ds, pbnavitia::Dataset* dataset) {
    dataset->set_uri(ds->uri);
    pt::time_duration td = pt::time_duration(0, 0, 0, 0);
    dataset->set_start_validation_date(navitia::to_posix_timestamp(pt::ptime(ds->validation_period.begin(), td)));
    dataset->set_end_validation_date(navitia::to_posix_timestamp(pt::ptime(ds->validation_period.end(), td)));
    dataset->set_desc(ds->desc);
    dataset->set_system(ds->system);
    dataset->set_realtime_level(to_pb_realtime_level(ds->realtime_level));
    fill(ds->contributor, dataset);
}

void PbCreator::Filler::fill_pb_object(const nt::StopArea* sa, pbnavitia::StopArea* stop_area) {
    stop_area->set_uri(sa->uri);
    add_contributor(sa);
    stop_area->set_name(sa->name);
    stop_area->set_label(sa->label);
    stop_area->set_timezone(sa->timezone);

    fill_comments(sa, stop_area);

    if (sa->coord.is_initialized()) {
        stop_area->mutable_coord()->set_lon(sa->coord.lon());
        stop_area->mutable_coord()->set_lat(sa->coord.lat());
    }
    if (depth > 0) {
        fill(sa->admin_list, stop_area->mutable_administrative_regions());
    }

    if (depth > 1) {
        std::vector<nt::CommercialMode*> cm = ptref_indexes<nt::CommercialMode>(sa);
        fill(cm, stop_area->mutable_commercial_modes());

        std::vector<nt::PhysicalMode*> pm = ptref_indexes<nt::PhysicalMode>(sa);
        fill(pm, stop_area->mutable_physical_modes());
    }

    fill_messages(sa, stop_area);
    fill_codes(sa, stop_area);

    if (depth > 2) {
        std::vector<nt::Line*> lines = ptref_indexes<nt::Line>(sa);
        copy(0, dump_message_options).fill(lines, stop_area->mutable_lines());
    }
}

void PbCreator::Filler::fill_pb_object(const ng::Admin* adm, pbnavitia::AdministrativeRegion* admin) {
    admin->set_name(adm->name);
    admin->set_uri(adm->uri);
    admin->set_label(adm->label);
    admin->set_zip_code(adm->postal_codes_to_string());
    admin->set_level(adm->level);
    if (adm->coord.is_initialized()) {
        admin->mutable_coord()->set_lat(adm->coord.lat());
        admin->mutable_coord()->set_lon(adm->coord.lon());
    }
    if (!adm->insee.empty()) {
        admin->set_insee(adm->insee);
    }
    if (depth > 1) {
        // for the admin we add the main stop area, but with the minimum vital information
        auto minimum_filler = Filler(0, {DumpMessage::No, DumpLineSectionMessage::No}, pb_creator);
        for (const auto& sa : adm->main_stop_areas) {
            auto* pb_sa = admin->add_main_stop_areas();

            minimum_filler.fill_pb_object(sa, pb_sa);
            for (const auto& sp : sa->stop_point_list) {
                auto* pb_sp = pb_sa->add_stop_points();
                minimum_filler.fill_pb_object(sp, pb_sp);
            }
        }
    }
}

void PbCreator::Filler::create_access_point(const nt::AccessPoint& access_point, pbnavitia::AccessPoint* ap) {
    ap->set_name(access_point.name);
    ap->set_uri(access_point.uri);
    ap->set_embedded_type(pbnavitia::pt_access_point);
    if (access_point.coord.is_initialized()) {
        ap->mutable_coord()->set_lon(access_point.coord.lon());
        ap->mutable_coord()->set_lat(access_point.coord.lat());
    }
    ap->set_is_entrance(access_point.is_entrance);
    ap->set_is_exit(access_point.is_exit);
    if (access_point.pathway_mode != nt::ap_default_value) {
        ap->set_pathway_mode(access_point.pathway_mode);
    }
    if (access_point.length != nt::ap_default_value) {
        ap->set_length(access_point.length);
    }
    if (access_point.traversal_time != nt::ap_default_value) {
        ap->set_traversal_time(access_point.traversal_time);
    }
    if (access_point.stair_count != nt::ap_default_value) {
        ap->set_stair_count(access_point.stair_count);
    }
    if (access_point.max_slope != nt::ap_default_value) {
        ap->set_max_slope(access_point.max_slope);
    }
    if (access_point.min_width != nt::ap_default_value) {
        ap->set_min_width(access_point.min_width);
    }
    if (!access_point.signposted_as.empty()) {
        ap->set_signposted_as(access_point.signposted_as);
    }
    if (!access_point.reversed_signposted_as.empty()) {
        ap->set_reversed_signposted_as(access_point.reversed_signposted_as);
    }
    if (!access_point.stop_code.empty()) {
        ap->set_stop_code(access_point.stop_code);
    }
    if (access_point.parent_station != nullptr) {
        pbnavitia::StopArea* sa = ap->mutable_parent_station();
        fill_pb_object(access_point.parent_station, sa);
    }
}

void PbCreator::Filler::fill_pb_object(const nt::AccessPoint& ap, pbnavitia::AccessPoint* pb_ap) {
    create_access_point(ap, pb_ap);
}

void PbCreator::Filler::fill_access_points(const std::set<nt::AccessPoint>& access_points,
                                           pbnavitia::StopPoint* stop_point) {
    for (const auto& access_point : access_points) {
        pbnavitia::AccessPoint* ap = stop_point->add_access_points();
        create_access_point(access_point, ap);
    }
}

void PbCreator::Filler::fill_pb_object(const nt::StopPoint* sp, pbnavitia::StopPoint* stop_point) {
    stop_point->set_uri(sp->uri);
    add_contributor(sp);
    stop_point->set_name(sp->name);
    stop_point->set_label(sp->label);
    if (!sp->platform_code.empty()) {
        stop_point->set_platform_code(sp->platform_code);
    }
    fill_comments(sp, stop_point);

    if (sp->coord.is_initialized()) {
        stop_point->mutable_coord()->set_lon(sp->coord.lon());
        stop_point->mutable_coord()->set_lat(sp->coord.lat());
    }

    pbnavitia::hasEquipments* has_equipments = stop_point->mutable_has_equipments();
    if (sp->wheelchair_boarding()) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_wheelchair_boarding);
    }
    if (sp->sheltered()) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_sheltered);
    }
    if (sp->elevator()) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_elevator);
    }
    if (sp->escalator()) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_escalator);
    }
    if (sp->bike_accepted()) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_bike_accepted);
    }
    if (sp->bike_depot()) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_bike_depot);
    }
    if (sp->visual_announcement()) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_visual_announcement);
    }
    if (sp->audible_announcement()) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_audible_announcement);
    }
    if (sp->appropriate_escort()) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_appropriate_escort);
    }
    if (sp->appropriate_signage()) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_appropriate_signage);
    }
    if (depth > 0) {
        fill(sp->admin_list, stop_point->mutable_administrative_regions());
    }
    // access points
    if (depth > 2) {
        fill_access_points(sp->access_points, stop_point);
    }

    fill(sp->address, stop_point);

    if (depth > 0) {
        fill(sp->stop_area, stop_point);
    }

    if (depth > 1) {
        std::vector<nt::CommercialMode*> cm = ptref_indexes<nt::CommercialMode>(sp);
        fill(cm, stop_point->mutable_commercial_modes());

        std::vector<nt::PhysicalMode*> pm = ptref_indexes<nt::PhysicalMode>(sp);
        fill(pm, stop_point->mutable_physical_modes());
    }

    if (!sp->fare_zone.empty()) {
        auto* farezone = stop_point->mutable_fare_zone();
        farezone->set_name(sp->fare_zone);
    }

    fill_messages(sp, stop_point);
    fill_codes(sp, stop_point);

    if (depth > 2) {
        std::vector<nt::Line*> lines = ptref_indexes<nt::Line>(sp);
        copy(0, dump_message_options).fill(lines, stop_point->mutable_lines());
    }
}

void PbCreator::Filler::fill_pb_object(const nt::Company* c, pbnavitia::Company* company) {
    company->set_name(c->name);
    company->set_uri(c->uri);
    add_contributor(c);

    fill_codes(c, company);
}

void PbCreator::Filler::fill_pb_object(const nt::Network* n, pbnavitia::Network* network) {
    network->set_name(n->name);
    network->set_uri(n->uri);
    add_contributor(n);

    fill_messages(n, network);
    fill_codes(n, network);
}

void PbCreator::Filler::fill_pb_object(const nt::PhysicalMode* m, pbnavitia::PhysicalMode* physical_mode) {
    physical_mode->set_name(m->name);
    physical_mode->set_uri(m->uri);

    if (depth == 0) {
        return;
    }

    if (m->co2_emission) {
        auto* emission_rate = physical_mode->mutable_co2_emission_rate();
        emission_rate->set_value(*m->co2_emission);
        emission_rate->set_unit("gEC/Km");
    }
}

void PbCreator::Filler::fill_pb_object(const nt::CommercialMode* m, pbnavitia::CommercialMode* commercial_mode) {
    commercial_mode->set_name(m->name);
    commercial_mode->set_uri(m->uri);
}

void PbCreator::Filler::fill_pb_object(const nt::Line* l, pbnavitia::Line* line) {
    fill_comments(l, line);

    if (!l->code.empty()) {
        line->set_code(l->code);
    }
    if (!l->color.empty()) {
        line->set_color(l->color);
    }

    if (!l->text_color.empty()) {
        line->set_text_color(l->text_color);
    }

    line->set_name(l->name);
    line->set_uri(l->uri);
    add_contributor(l);
    if (l->opening_time) {
        line->set_opening_time((*l->opening_time).total_seconds());
    }
    if (l->closing_time) {
        line->set_closing_time((*l->closing_time).total_seconds());
    }
    fill(l->physical_mode_list, line->mutable_physical_modes());
    fill(l->commercial_mode, line);
    fill(l->network, line);

    if (depth > 0) {
        if (!this->pb_creator.disable_geojson) {
            fill(&l->shape, line);
        }

        copy(0, dump_message_options).fill(l->route_list, line->mutable_routes());

        fill(l->line_group_list, line->mutable_line_groups());
    }

    fill_messages(l, line);
    fill_codes(l, line);

    for (auto property : l->properties) {
        auto* pb_property = line->add_properties();
        pb_property->set_name(property.first);
        pb_property->set_value(property.second);
    }

    if (dump_message_options.dump_message == DumpMessage::Yes
        && dump_message_options.dump_line_section == DumpLineSectionMessage::Yes) {
        /*
         * Here we dump the impacts which impact LineSection.
         * We could have link the LineSection impact with the line, but that would change the code and
         * the behavior too much.
         * */
        auto fill_line_section_message = [&](const nt::VehicleJourney& vj) {
            for (const auto& impact_ptr : vj.meta_vj->get_publishable_messages(pb_creator.now)) {
                if (impact_ptr->is_line_section_of(*vj.route->line)) {
                    fill_message(impact_ptr, line);
                }
            }
            return true;
        };
        for (const auto* route : l->route_list) {
            route->for_each_vehicle_journey(fill_line_section_message);
        }
    }
    if (dump_message_options.dump_message == DumpMessage::Yes
        && dump_message_options.dump_rail_section == DumpRailSectionMessage::Yes) {
        /*
         * Here we dump the impacts which impact RailSection.
         * We could have link the LineSection impact with the line, but that would change the code and
         * the behavior too much.
         * */
        auto fill_rail_section_message = [&](const nt::VehicleJourney& vj) {
            for (const auto& impact_ptr : vj.meta_vj->get_publishable_messages(pb_creator.now)) {
                if (impact_ptr->is_rail_section_of(*vj.route->line)) {
                    fill_message(impact_ptr, line);
                }
            }
            return true;
        };
        for (const auto* route : l->route_list) {
            route->for_each_vehicle_journey(fill_rail_section_message);
        }
    }
}

void PbCreator::Filler::fill_pb_object(const nt::Route* r, pbnavitia::Route* route) {
    route->set_name(r->name);
    route->set_direction_type(r->direction_type);

    fill_with_creator(r->destination, [&]() { return route->mutable_direction(); });
    fill_comments(r, route);

    route->set_uri(r->uri);
    add_contributor(r);
    fill_messages(r, route);

    fill_codes(r, route);

    if (depth == 0) {
        return;
    }

    copy(0, dump_message_options).fill(r->line, route);
    if ((depth > 1) && (!this->pb_creator.disable_geojson)) {
        fill(&r->line->shape, route->mutable_line());
    }

    if (!this->pb_creator.disable_geojson) {
        fill(&r->shape, route);
    }

    if (depth > 2) {
        auto thermometer = navitia::timetables::Thermometer();
        thermometer.generate_thermometer(r);
        for (auto idx : thermometer.get_thermometer()) {
            auto stop_point = pb_creator.data->pt_data->stop_points[idx];
            fill_with_creator(stop_point, [&]() { return route->add_stop_points(); });
        }
        std::vector<nt::PhysicalMode*> pm = ptref_indexes<nt::PhysicalMode>(r);
        fill(pm, route->mutable_physical_modes());
    }
}

void PbCreator::Filler::fill_pb_object(const nt::LineGroup* lg, pbnavitia::LineGroup* line_group) {
    line_group->set_name(lg->name);
    line_group->set_uri(lg->uri);
    add_contributor(lg);

    if (depth > 0) {
        fill(lg->line_list, line_group->mutable_lines());
        copy(0, dump_message_options).fill_pb_object(lg->main_line, line_group->mutable_main_line());
        fill_comments(lg, line_group);
    }
}

void PbCreator::Filler::fill_pb_object(const nt::Calendar* cal, pbnavitia::Calendar* pb_cal) {
    pb_cal->set_uri(cal->uri);
    add_contributor(cal);
    pb_cal->set_name(cal->name);
    auto vp = pb_cal->mutable_validity_pattern();
    vp->set_beginning_date(gd::to_iso_string(cal->validity_pattern.beginning_date));
    std::string vp_str = cal->validity_pattern.str();
    std::reverse(vp_str.begin(), vp_str.end());
    vp->set_days(vp_str);
    auto week = pb_cal->mutable_week_pattern();
    week->set_monday(cal->week_pattern[navitia::Monday]);
    week->set_tuesday(cal->week_pattern[navitia::Tuesday]);
    week->set_wednesday(cal->week_pattern[navitia::Wednesday]);
    week->set_thursday(cal->week_pattern[navitia::Thursday]);
    week->set_friday(cal->week_pattern[navitia::Friday]);
    week->set_saturday(cal->week_pattern[navitia::Saturday]);
    week->set_sunday(cal->week_pattern[navitia::Sunday]);

    for (const auto& p : cal->active_periods) {
        auto pb_period = pb_cal->add_active_periods();
        pb_period->set_begin(gd::to_iso_string(p.begin()));
        pb_period->set_end(gd::to_iso_string(p.end()));
    }
    fill(cal->exceptions, pb_cal->mutable_exceptions());
}

void PbCreator::Filler::fill_pb_object(const nt::ExceptionDate* exception_date,
                                       pbnavitia::CalendarException* calendar_exception) {
    pbnavitia::ExceptionType type;
    switch (exception_date->type) {
        case nt::ExceptionDate::ExceptionType::add:
            type = pbnavitia::Add;
            break;
        case nt::ExceptionDate::ExceptionType::sub:
            type = pbnavitia::Remove;
            break;
        default:
            throw navitia::exception("exception date case not handled");
    }
    calendar_exception->set_uri("exception:" + std::to_string(type) + gd::to_iso_string(exception_date->date));
    calendar_exception->set_type(type);
    calendar_exception->set_date(gd::to_iso_string(exception_date->date));
}

void PbCreator::Filler::fill_pb_object(const nt::ValidityPattern* vp, pbnavitia::ValidityPattern* validity_pattern) {
    auto vp_string = gd::to_iso_string(vp->beginning_date);
    validity_pattern->set_beginning_date(vp_string);
    validity_pattern->set_days(vp->days.to_string());
}

void PbCreator::Filler::fill_pb_object(const nt::VehicleJourney* vj, pbnavitia::VehicleJourney* vehicle_journey) {
    vehicle_journey->set_name(vj->name);
    vehicle_journey->set_uri(vj->uri);
    vehicle_journey->set_headsign(vj->headsign);
    add_contributor(vj);
    fill_comments(vj, vehicle_journey);
    vehicle_journey->set_odt_message(vj->odt_message);
    vehicle_journey->set_is_adapted(vj->realtime_level == nt::RTLevel::Adapted);

    vehicle_journey->set_wheelchair_accessible(vj->wheelchair_accessible());
    vehicle_journey->set_bike_accepted(vj->bike_accepted());
    vehicle_journey->set_air_conditioned(vj->air_conditioned());
    vehicle_journey->set_visual_announcement(vj->visual_announcement());
    vehicle_journey->set_appropriate_escort(vj->appropriate_escort());
    vehicle_journey->set_appropriate_signage(vj->appropriate_signage());
    vehicle_journey->set_school_vehicle(vj->school_vehicle());

    if (depth > 0) {
        const auto& jp_idx = pb_creator.data->dataRaptor->jp_container.get_jp_from_vj()[navitia::routing::VjIdx(*vj)];
        const auto& pair_jp = pb_creator.data->dataRaptor->jp_container.get_jps()[jp_idx.val];
        fill(&pair_jp, vehicle_journey);

        fill(vj->stop_time_list, vehicle_journey->mutable_stop_times());
        fill(vj->meta_vj, vehicle_journey);

        fill(vj->physical_mode, vehicle_journey->mutable_journey_pattern());
        fill_with_creator(vj->base_validity_pattern(), [&]() { return vehicle_journey->mutable_validity_pattern(); });
        fill_with_creator(vj->adapted_validity_pattern(),
                          [&]() { return vehicle_journey->mutable_adapted_validity_pattern(); });

        const auto& vector_bp = navitia::vptranslator::translate(*vj->base_validity_pattern());

        fill(vector_bp, vehicle_journey->mutable_calendars());

        if (auto* v = dynamic_cast<const nt::FrequencyVehicleJourney*>(vj)) {
            fill_pb_object(v, vehicle_journey);
        }
    }
    fill_messages(vj->meta_vj, vehicle_journey);

    // For a realtime vj, we should get de base vj to fill codes
    if (vj->realtime_level == nt::RTLevel::RealTime && !vj->meta_vj->get_base_vj().empty()) {
        const auto& base_vjs = vj->meta_vj->get_base_vj();
        vj = base_vjs.front().get();
    }
    fill_codes(vj, vehicle_journey);
}

void PbCreator::Filler::fill_pb_object(const nt::FrequencyVehicleJourney* fvj, pbnavitia::VehicleJourney* pb_vj) {
    pb_vj->set_start_time(fvj->start_time);
    pb_vj->set_end_time(fvj->end_time);
    pb_vj->set_headway_secs(fvj->headway_secs);
}
void PbCreator::Filler::fill_pb_object(const nt::MetaVehicleJourney* nav_mvj, pbnavitia::Trip* pb_trip) {
    pb_trip->set_uri(nav_mvj->uri);

    // the name of the meta vj is the name of it's first vj (all sub vj must have the same name)
    nav_mvj->for_all_vjs([&pb_trip](const nt::VehicleJourney& vj) {
        pb_trip->set_name(vj.name);
        return false;  // we stop at the first one
    });
}

void PbCreator::Filler::fill_pb_object(const nt::MultiLineString* shape, pbnavitia::MultiLineString* geojson) {
    for (const std::vector<nt::GeographicalCoord>& line : *shape) {
        auto l = geojson->add_lines();
        for (const auto& coord : line) {
            auto c = l->add_coordinates();
            c->set_lon(coord.lon());
            c->set_lat(coord.lat());
        }
    }
}

void PbCreator::Filler::fill_pb_object(const nt::GeographicalCoord* coord, pbnavitia::Address* address) {
    if (!coord->is_initialized()) {
        return;
    }

    try {
        const auto nb_way = pb_creator.data->geo_ref->nearest_addr(*coord);
        const auto& ng_address = navitia::georef::Address(nb_way.second, *coord, nb_way.first);
        fill_pb_object(&ng_address, address);
    } catch (const navitia::proximitylist::NotFound&) {
        LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("logger"),
                        "unable to find a way from coord [" << coord->lon() << "-" << coord->lat() << "]");
    }
}

void PbCreator::Filler::fill_pb_object(const ng::Address* ng_address, pbnavitia::Address* address) {
    if (ng_address->way == nullptr) {
        return;
    }

    address->set_name(ng_address->way->name);
    std::string label;
    if (ng_address->number >= 1) {
        address->set_house_number(ng_address->number);
        label += std::to_string(ng_address->number) + " ";
    }
    label += get_label(ng_address->way);
    address->set_label(label);

    if (ng_address->coord.is_initialized()) {
        address->mutable_coord()->set_lon(ng_address->coord.lon());
        address->mutable_coord()->set_lat(ng_address->coord.lat());
        std::stringstream ss;
        ss << std::setprecision(16) << ng_address->coord.lon() << ";" << ng_address->coord.lat();
        address->set_uri(ss.str());
    }

    if (depth > 0) {
        fill(ng_address->way->admin_list, address->mutable_administrative_regions());
    }
}

void PbCreator::Filler::fill_pb_object(const nt::StopPointConnection* c, pbnavitia::Connection* connection) {
    connection->set_duration(c->duration);
    connection->set_display_duration(c->display_duration);
    connection->set_max_duration(c->max_duration);
    if (depth > 0) {
        fill_with_creator(c->departure, [&]() { return connection->mutable_origin(); });
        fill_with_creator(c->destination, [&]() { return connection->mutable_destination(); });
    }
}

static uint32_t get_local_time(const nt::StopTime* st, const uint32_t time) {
    static const auto flag = std::numeric_limits<uint32_t>::max();
    uint32_t offset = 0;
    if (time != flag && st->vehicle_journey != nullptr) {
        // getting the positive offset to be safe about integer overflow
        const int32_t day = DateTimeUtils::SECONDS_PER_DAY;
        // That's `offset mod day`. `offset % day` can be negative, thus we
        // trick a bit to always have the good solution.
        offset = (st->vehicle_journey->utc_to_local_offset() % day + day) % day;
    }
    return time + offset;
}

void PbCreator::Filler::fill_pb_object(const nd::StopTimeUpdate* stu, pbnavitia::StopTime* stop_time) {
    const auto* st = &stu->stop_time;
    // we don't want to output amended departure/arrival for deleted departure/arrival
    if ((stu->arrival_status != nd::StopTimeUpdate::Status::DELETED)
        && (stu->arrival_status != nd::StopTimeUpdate::Status::DELETED_FOR_DETOUR)) {
        stop_time->set_arrival_time(get_local_time(st, st->arrival_time));
        stop_time->set_utc_arrival_time(st->arrival_time);
    }
    if ((stu->departure_status != nd::StopTimeUpdate::Status::DELETED)
        && (stu->departure_status != nd::StopTimeUpdate::Status::DELETED_FOR_DETOUR)) {
        stop_time->set_departure_time(get_local_time(st, st->departure_time));
        stop_time->set_utc_departure_time(st->departure_time);
    }
}

void PbCreator::Filler::fill_pb_object(const nt::StopTime* st, pbnavitia::StopTime* stop_time) {
    // arrival/departure in protobuff are as seconds from midnight in local time
    stop_time->set_arrival_time(get_local_time(st, st->arrival_time));
    stop_time->set_utc_arrival_time(st->arrival_time);
    stop_time->set_departure_time(get_local_time(st, st->departure_time));
    stop_time->set_utc_departure_time(st->departure_time);
    stop_time->set_headsign(pb_creator.data->pt_data->headsign_handler.get_headsign(*st));

    stop_time->set_pickup_allowed(st->pick_up_allowed());
    stop_time->set_drop_off_allowed(st->drop_off_allowed());
    stop_time->set_skipped_stop(st->skipped_stop());

    // TODO V2: the dump of the JPP is deprecated, but we keep it for retrocompatibility
    if (depth > 0) {
        const auto& jpp_idx = pb_creator.data->dataRaptor->jp_container.get_jpp(*st);
        const auto& pair_jpp = pb_creator.data->dataRaptor->jp_container.get_jpps()[jpp_idx.val];
        fill(&pair_jpp, stop_time);
    }

    // we always dump the stop point (with the same depth)
    copy(depth, dump_message_options).fill_pb_object(st->stop_point, stop_time->mutable_stop_point());

    if (depth > 0) {
        fill(st->vehicle_journey, stop_time);
    }
}

void PbCreator::Filler::fill_pb_object(const nt::StopTime* st, pbnavitia::StopDateTime* stop_date_time) {
    auto* properties = stop_date_time->mutable_properties();
    fill_with_creator(st, [&]() { return properties; });
    fill(pb_creator.data->pt_data->comments.get(*st), properties->mutable_notes());
}

void PbCreator::Filler::fill_pb_object(const nt::Comment* comment, pbnavitia::Note* note) {
    std::hash<std::string> hash_fn;
    note->set_uri("note:" + std::to_string(hash_fn(comment->value + comment->type)));
    note->set_note(comment->value);
    if (!comment->type.empty()) {
        note->set_comment_type(comment->type);
    }
}

void PbCreator::Filler::fill_pb_object(const std::string* comment, pbnavitia::Note* note) {
    std::hash<std::string> hash_fn;
    note->set_uri("note:" + std::to_string(hash_fn(*comment)));
    note->set_note(*comment);
}

void PbCreator::Filler::fill_pb_object(const jp_pair* jp, pbnavitia::JourneyPattern* journey_pattern) {
    const std::string id = pb_creator.data->dataRaptor->jp_container.get_id(jp->first);
    journey_pattern->set_name(id);
    journey_pattern->set_uri(id);
    if (depth > 0) {
        const auto* route = pb_creator.data->pt_data->routes[jp->second.route_idx.val];
        fill(route, journey_pattern);
    }
}

void PbCreator::Filler::fill_pb_object(const jpp_pair* jpp, pbnavitia::JourneyPatternPoint* journey_pattern_point) {
    journey_pattern_point->set_uri(pb_creator.data->dataRaptor->jp_container.get_id(jpp->first));
    journey_pattern_point->set_order(jpp->second.order.val);

    if (depth > 0) {
        fill(pb_creator.data->pt_data->stop_points[jpp->second.sp_idx.val], journey_pattern_point);
        const auto& jp = pb_creator.data->dataRaptor->jp_container.get_jps()[jpp->second.jp_idx.val];
        fill(&jp, journey_pattern_point);
    }
}
void PbCreator::Filler::fill_pb_object(const navitia::vptranslator::BlockPattern* bp, pbnavitia::Calendar* pb_cal) {
    auto week = pb_cal->mutable_week_pattern();
    week->set_monday(bp->week[navitia::Monday]);
    week->set_tuesday(bp->week[navitia::Tuesday]);
    week->set_wednesday(bp->week[navitia::Wednesday]);
    week->set_thursday(bp->week[navitia::Thursday]);
    week->set_friday(bp->week[navitia::Friday]);
    week->set_saturday(bp->week[navitia::Saturday]);
    week->set_sunday(bp->week[navitia::Sunday]);

    for (const auto& p : bp->validity_periods) {
        auto pb_period = pb_cal->add_active_periods();
        pb_period->set_begin(gd::to_iso_string(p.begin()));
        pb_period->set_end(gd::to_iso_string(p.end()));
    }
    fill(bp->exceptions, pb_cal->mutable_exceptions());
}

void PbCreator::Filler::fill_pb_object(const nt::StopTime* stop_time, pbnavitia::Properties* properties) {
    if (((!stop_time->drop_off_allowed()) && stop_time->pick_up_allowed())
        // No display pick up only information if first stoptime in vehiclejourney
        && ((stop_time->vehicle_journey != nullptr)
            && (&stop_time->vehicle_journey->stop_time_list.front() != stop_time))) {
        properties->add_additional_informations(pbnavitia::Properties::pick_up_only);
    }
    if ((stop_time->drop_off_allowed() && (!stop_time->pick_up_allowed()))
        // No display drop off only information if last stoptime in vehiclejourney
        && ((stop_time->vehicle_journey != nullptr)
            && (&stop_time->vehicle_journey->stop_time_list.back() != stop_time))) {
        properties->add_additional_informations(pbnavitia::Properties::drop_off_only);
    }
    if (stop_time->odt()) {
        properties->add_additional_informations(pbnavitia::Properties::on_demand_transport);
    }
    if (stop_time->date_time_estimated()) {
        properties->add_additional_informations(pbnavitia::Properties::date_time_estimated);
    }
    if (stop_time->skipped_stop()) {
        properties->add_additional_informations(pbnavitia::Properties::skipped_stop);
    }
}

void PbCreator::Filler::fill_informed_entity(const nd::PtObj& ptobj,
                                             const nd::Impact& impact,
                                             pbnavitia::Impact* pb_impact) {
    auto filler = copy(depth - 1, dump_message_options);
    boost::apply_visitor(PtObjVisitor(impact, pb_impact, filler), ptobj);
}

static pbnavitia::ActiveStatus compute_disruption_status(const nd::Impact& impact, const pt::ptime& now) {
    return to_pb_active_status(impact.get_active_status(now));
}

static pbnavitia::Severity_Effect get_severity_effect(nd::Effect e) {
    switch (e) {
        case nd::Effect::NO_SERVICE:
            return pbnavitia::Severity_Effect::Severity_Effect_NO_SERVICE;
        case nd::Effect::REDUCED_SERVICE:
            return pbnavitia::Severity_Effect::Severity_Effect_REDUCED_SERVICE;
        case nd::Effect::SIGNIFICANT_DELAYS:
            return pbnavitia::Severity_Effect::Severity_Effect_SIGNIFICANT_DELAYS;
        case nd::Effect::DETOUR:
            return pbnavitia::Severity_Effect::Severity_Effect_DETOUR;
        case nd::Effect::ADDITIONAL_SERVICE:
            return pbnavitia::Severity_Effect::Severity_Effect_ADDITIONAL_SERVICE;
        case nd::Effect::MODIFIED_SERVICE:
            return pbnavitia::Severity_Effect::Severity_Effect_MODIFIED_SERVICE;
        case nd::Effect::OTHER_EFFECT:
            return pbnavitia::Severity_Effect::Severity_Effect_OTHER_EFFECT;
        case nd::Effect::STOP_MOVED:
            return pbnavitia::Severity_Effect::Severity_Effect_STOP_MOVED;
        case nd::Effect::UNKNOWN_EFFECT:
        default:
            return pbnavitia::Severity_Effect::Severity_Effect_UNKNOWN_EFFECT;
    }
}

template <typename P>
void PbCreator::Filler::fill_message(const boost::shared_ptr<nd::Impact>& impact, P pb_object) {
    if (pb_creator.disable_disruption) {
        return;
    }
    using boost::algorithm::none_of;
    // Adding the uri only if not already present
    if (none_of(pb_object->impact_uris(), [&](const std::string& uri) { return impact->uri == uri; })) {
        *pb_object->add_impact_uris() = impact->uri;
    }
    pb_creator.impacts.insert(impact);
}
template void navitia::PbCreator::Filler::fill_message<pbnavitia::Network*>(
    boost::shared_ptr<navitia::type::disruption::Impact> const&,
    pbnavitia::Network*);
template void navitia::PbCreator::Filler::fill_message<pbnavitia::Line*>(
    boost::shared_ptr<navitia::type::disruption::Impact> const&,
    pbnavitia::Line*);
template void navitia::PbCreator::Filler::fill_message<pbnavitia::StopArea*>(
    boost::shared_ptr<navitia::type::disruption::Impact> const&,
    pbnavitia::StopArea*);
template void navitia::PbCreator::Filler::fill_message<pbnavitia::VehicleJourney*>(
    boost::shared_ptr<navitia::type::disruption::Impact> const&,
    pbnavitia::VehicleJourney*);

void PbCreator::Filler::fill_pb_object(const nd::Impact* impact, pbnavitia::Impact* pb_impact) {
    pb_impact->set_disruption_uri(impact->disruption->uri);

    if (!impact->disruption->contributor.empty()) {
        pb_impact->set_contributor(impact->disruption->contributor);
    }

    pb_impact->set_uri(impact->uri);
    for (const auto& app_period : impact->application_periods) {
        auto p = pb_impact->add_application_periods();
        p->set_begin(navitia::to_posix_timestamp(app_period.begin()));
        p->set_end(navitia::to_posix_timestamp(app_period.end()));
    }

    for (const auto& app_period : impact->application_patterns) {
        auto application_pattern = pb_impact->add_application_patterns();
        auto application_period = application_pattern->mutable_application_period();
        application_period->set_begin(
            navitia::to_posix_timestamp(navitia::ptime(app_period.application_period.begin())));
        application_period->set_end(navitia::to_posix_timestamp(navitia::ptime(app_period.application_period.end())));
        auto week = application_pattern->mutable_week_pattern();
        week->set_monday(app_period.week_pattern[navitia::Monday]);
        week->set_tuesday(app_period.week_pattern[navitia::Tuesday]);
        week->set_wednesday(app_period.week_pattern[navitia::Wednesday]);
        week->set_thursday(app_period.week_pattern[navitia::Thursday]);
        week->set_friday(app_period.week_pattern[navitia::Friday]);
        week->set_saturday(app_period.week_pattern[navitia::Saturday]);
        week->set_sunday(app_period.week_pattern[navitia::Sunday]);
        for (const auto& ts : app_period.time_slots) {
            auto time_slot = application_pattern->add_time_slots();
            time_slot->set_begin(ts.begin);
            time_slot->set_end(ts.end);
        }
    }

    // TODO: updated at must be computed with the max of all computed values (from disruption, impact, ...)
    pb_impact->set_updated_at(navitia::to_posix_timestamp(impact->updated_at));

    auto pb_severity = pb_impact->mutable_severity();
    pb_severity->set_name(impact->severity->wording);
    pb_severity->set_color(impact->severity->color);
    pb_severity->set_effect(get_severity_effect(impact->severity->effect));
    pb_severity->set_priority(impact->severity->priority);

    for (const auto& t : impact->disruption->tags) {
        pb_impact->add_tags(t->name);
    }

    for (const auto& p : impact->disruption->properties) {
        auto pb_p = pb_impact->add_properties();
        pb_p->set_key(p.key);
        pb_p->set_type(p.type);
        pb_p->set_value(p.value);
    }

    if (impact->disruption->cause) {
        pb_impact->set_cause(impact->disruption->cause->wording);
        if (!impact->disruption->cause->category.empty()) {
            pb_impact->set_category(impact->disruption->cause->category);
        }
    }

    for (const auto& m : impact->messages) {
        auto pb_m = pb_impact->add_messages();
        pb_m->set_text(m.text);

        auto translated_text = this->pb_creator.get_translated_message(m.translations, this->pb_creator.language);
        if (!translated_text.empty()) {
            pb_m->set_text(translated_text);
        }
        auto pb_channel = pb_m->mutable_channel();
        pb_channel->set_content_type(m.channel_content_type);
        pb_channel->set_id(m.channel_id);
        pb_channel->set_name(m.channel_name);
        for (const auto& type : m.channel_types) {
            switch (type) {
                case nd::ChannelType::web:
                    pb_channel->add_channel_types(pbnavitia::Channel::web);
                    break;
                case nd::ChannelType::sms:
                    pb_channel->add_channel_types(pbnavitia::Channel::sms);
                    break;
                case nd::ChannelType::email:
                    pb_channel->add_channel_types(pbnavitia::Channel::email);
                    break;
                case nd::ChannelType::mobile:
                    pb_channel->add_channel_types(pbnavitia::Channel::mobile);
                    break;
                case nd::ChannelType::notification:
                    pb_channel->add_channel_types(pbnavitia::Channel::notification);
                    break;
                case nd::ChannelType::twitter:
                    pb_channel->add_channel_types(pbnavitia::Channel::twitter);
                    break;
                case nd::ChannelType::facebook:
                    pb_channel->add_channel_types(pbnavitia::Channel::facebook);
                    break;
                case nd::ChannelType::unknown_type:
                    pb_channel->add_channel_types(pbnavitia::Channel::unknown_type);
                    break;
                case nd::ChannelType::title:
                    pb_channel->add_channel_types(pbnavitia::Channel::title);
                    break;
                case nd::ChannelType::beacon:
                    pb_channel->add_channel_types(pbnavitia::Channel::beacon);
                    break;
            }
        }
    }

    // we need to compute the active status
    pb_impact->set_status(compute_disruption_status(*impact, pb_creator.now));

    for (const auto& informed_entity : impact->informed_entities()) {
        fill_informed_entity(informed_entity, *impact, pb_impact);
    }
}

void PbCreator::Filler::fill_pb_object(const nt::Route* r, pbnavitia::PtDisplayInfo* pt_display_info) {
    pbnavitia::Uris* uris = pt_display_info->mutable_uris();
    uris->set_route(r->uri);
    fill(pb_creator.data->pt_data->comments.get(r), pt_display_info->mutable_notes());
    fill_messages(r, pt_display_info);

    if (r->destination != nullptr) {
        this->pb_creator.terminus.insert(r->destination);
        // we always dump the stop point (with the same depth)
        // Here we format display_informations.direction for stop_schedules.
        pt_display_info->set_direction(r->destination->name);
        uris->set_stop_area(r->destination->uri);
        for (auto admin : r->destination->admin_list) {
            if (admin->level == 8) {
                pt_display_info->set_direction(r->destination->name + " (" + admin->name + ")");
            }
        }
    }
    if (r->line != nullptr) {
        pt_display_info->set_color(r->line->color);
        pt_display_info->set_text_color(r->line->text_color);
        pt_display_info->set_code(r->line->code);
        pt_display_info->set_name(r->line->name);
        fill(pb_creator.data->pt_data->comments.get(r->line), pt_display_info->mutable_notes());

        fill_messages(r->line, pt_display_info);
        uris->set_line(r->line->uri);
        if (r->line->network != nullptr) {
            pt_display_info->set_network(r->line->network->name);
            uris->set_network(r->line->network->uri);

            fill_messages(r->line->network, pt_display_info);
        }
        if (r->line->commercial_mode != nullptr) {
            pt_display_info->set_commercial_mode(r->line->commercial_mode->name);
            uris->set_commercial_mode(r->line->commercial_mode->uri);
        }
    }
}

void PbCreator::Filler::fill_pb_object(const ng::POI* geopoi, pbnavitia::Poi* poi) {
    poi->set_name(geopoi->name);
    poi->set_uri(geopoi->uri);
    poi->set_label(geopoi->label);

    if (geopoi->coord.is_initialized()) {
        poi->mutable_coord()->set_lat(geopoi->coord.lat());
        poi->mutable_coord()->set_lon(geopoi->coord.lon());
    }

    if (depth > 0) {
        fill(pb_creator.data->geo_ref->poitypes[geopoi->poitype_idx], poi);

        fill(geopoi->admin_list, poi->mutable_administrative_regions());

        if (geopoi->address_name.empty()) {
            fill(&geopoi->coord, poi);
        } else {
            fill_with_creator(geopoi, [&]() { return poi->mutable_address(); });
        }
    }
    for (const auto& propertie : geopoi->properties) {
        pbnavitia::Code* pb_code = poi->add_properties();
        pb_code->set_type(propertie.first);
        pb_code->set_value(propertie.second);
    }
}

void PbCreator::Filler::fill_pb_object(const ng::POI* poi, pbnavitia::Address* address) {
    address->set_name(poi->address_name);
    std::string label;
    if (poi->address_number >= 1) {
        address->set_house_number(poi->address_number);
        label += std::to_string(poi->address_number) + " ";
    }
    label += poi->address_name;
    address->set_label(label);

    if (poi->coord.is_initialized()) {
        address->mutable_coord()->set_lon(poi->coord.lon());
        address->mutable_coord()->set_lat(poi->coord.lat());
        std::stringstream ss;
        ss << std::setprecision(16) << poi->coord.lon() << ";" << poi->coord.lat();
        address->set_uri(ss.str());
    }

    if (depth > 0) {
        fill(poi->admin_list, address->mutable_administrative_regions());
    }
}

void PbCreator::Filler::fill_pb_object(const ng::POIType* geo_poi_type, pbnavitia::PoiType* poi_type) {
    poi_type->set_name(geo_poi_type->name);
    poi_type->set_uri(geo_poi_type->uri);
}

void PbCreator::Filler::fill_pb_object(const VjOrigDest* vj_orig_dest, pbnavitia::hasEquipments* has_equipments) {
    bool wheelchair_accessible = vj_orig_dest->vj->wheelchair_accessible() && vj_orig_dest->orig->wheelchair_boarding()
                                 && vj_orig_dest->dest->wheelchair_boarding();
    if (wheelchair_accessible) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_wheelchair_accessibility);
    }
    if (vj_orig_dest->vj->bike_accepted()) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_bike_accepted);
    }
    if (vj_orig_dest->vj->air_conditioned()) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_air_conditioned);
    }
    bool visual_announcement = vj_orig_dest->vj->visual_announcement() && vj_orig_dest->orig->visual_announcement()
                               && vj_orig_dest->dest->visual_announcement();
    if (visual_announcement) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_visual_announcement);
    }
    bool audible_announcement = vj_orig_dest->vj->audible_announcement() && vj_orig_dest->orig->audible_announcement()
                                && vj_orig_dest->dest->audible_announcement();
    if (audible_announcement) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_audible_announcement);
    }
    bool appropriate_escort = vj_orig_dest->vj->appropriate_escort() && vj_orig_dest->orig->appropriate_escort()
                              && vj_orig_dest->dest->appropriate_escort();
    if (appropriate_escort) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_appropriate_escort);
    }
    bool appropriate_signage = vj_orig_dest->vj->appropriate_signage() && vj_orig_dest->orig->appropriate_signage()
                               && vj_orig_dest->dest->appropriate_signage();
    if (appropriate_signage) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_appropriate_signage);
    }
    if (vj_orig_dest->vj->school_vehicle()) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_school_vehicle);
    }
}

/*
 * override fill_messages for journey sections to handle line sections impacts
 *
 * for a journey section, we get:
 *  - all of the meta journey impacts but the line sections impacts
 *  - for the line sections impacts we check that they intersect the journey section
 */
void PbCreator::Filler::fill_messages(const VjStopTimes* vj_stoptimes, pbnavitia::PtDisplayInfo* pt_display_info) {
    if (vj_stoptimes == nullptr) {
        return;
    }
    if (dump_message_options.dump_message == DumpMessage::No) {
        return;
    }
    const auto* meta_vj = vj_stoptimes->vj->meta_vj;
    for (const auto& message : meta_vj->get_applicable_messages(pb_creator.now, pb_creator.action_period)) {
        if (!message->is_relevant(vj_stoptimes->stop_times)) {
            continue;
        }
        fill_message(message, pt_display_info);
    }

    // here we add all the impacts on stop_point and stop_area of section.from
    const auto& from_sp = vj_stoptimes->stop_times.front()->stop_point;
    for (const auto& message : from_sp->get_applicable_messages(pb_creator.now, pb_creator.action_period)) {
        if (message->is_only_line_section()) {
            continue;
        }
        if (message->is_only_rail_section()) {
            continue;
        }
        fill_message(message, pt_display_info);
    }

    const auto& from_sa = vj_stoptimes->stop_times.front()->stop_point->stop_area;
    for (const auto& message : from_sa->get_applicable_messages(pb_creator.now, pb_creator.action_period)) {
        fill_message(message, pt_display_info);
    }

    // here we add all the impacts on stop_point and stop_area of section.to
    const auto& to_sa = vj_stoptimes->stop_times.back()->stop_point->stop_area;
    for (const auto& message : to_sa->get_applicable_messages(pb_creator.now, pb_creator.action_period)) {
        fill_message(message, pt_display_info);
    }

    const auto& to_sp = vj_stoptimes->stop_times.back()->stop_point;
    for (const auto& message : to_sp->get_applicable_messages(pb_creator.now, pb_creator.action_period)) {
        if (message->is_only_line_section()) {
            continue;
        }
        if (message->is_only_rail_section()) {
            continue;
        }
        fill_message(message, pt_display_info);
    }
}

void PbCreator::Filler::fill_pb_object(const VjStopTimes* vj_stoptimes, pbnavitia::PtDisplayInfo* pt_display_info) {
    if (vj_stoptimes->vj == nullptr) {
        return;
    }

    pbnavitia::Uris* uris = pt_display_info->mutable_uris();
    uris->set_vehicle_journey(vj_stoptimes->vj->uri);
    if (vj_stoptimes->vj->dataset
        && vj_stoptimes->vj->dataset->contributor
        // Only contributor with license
        && (!vj_stoptimes->vj->dataset->contributor->license.empty())) {
        this->pb_creator.contributors.insert(vj_stoptimes->vj->dataset->contributor);
    }
    if (depth > 0 && vj_stoptimes->vj->route) {
        fill_with_creator(vj_stoptimes->vj->route, [&]() { return pt_display_info; });
        uris->set_route(vj_stoptimes->vj->route->uri);
        const auto& jp_idx =
            pb_creator.data->dataRaptor->jp_container.get_jp_from_vj()[navitia::routing::VjIdx(*vj_stoptimes->vj)];
        uris->set_journey_pattern(pb_creator.data->dataRaptor->jp_container.get_id(jp_idx));
    }

    fill_messages(vj_stoptimes, pt_display_info);

    const auto* first_st = vj_stoptimes->stop_times.empty() ? nullptr : vj_stoptimes->stop_times.front();
    if (first_st != nullptr) {
        pt_display_info->set_headsign(pb_creator.data->pt_data->headsign_handler.get_headsign(*first_st));
    }
    pt_display_info->set_direction(vj_stoptimes->vj->get_direction());
    if (vj_stoptimes->vj->physical_mode != nullptr) {
        pt_display_info->set_physical_mode(vj_stoptimes->vj->physical_mode->name);
        uris->set_physical_mode(vj_stoptimes->vj->physical_mode->uri);
    }
    pt_display_info->set_description(vj_stoptimes->vj->odt_message);
    pbnavitia::hasEquipments* has_equipments = pt_display_info->mutable_has_equipments();
    if (vj_stoptimes->stop_times.size() >= 2) {
        const auto& vj = VjOrigDest(vj_stoptimes->vj, vj_stoptimes->stop_times.front()->stop_point,
                                    vj_stoptimes->stop_times.back()->stop_point);
        fill_with_creator(&vj, [&]() { return has_equipments; });
    } else {
        fill_with_creator(vj_stoptimes->vj, [&]() { return has_equipments; });
    }
    fill(pb_creator.data->pt_data->comments.get(vj_stoptimes->vj), pt_display_info->mutable_notes());
    pt_display_info->set_trip_short_name(vj_stoptimes->vj->name);
}

void PbCreator::Filler::fill_pb_object(const nt::VehicleJourney* vj, pbnavitia::hasEquipments* has_equipments) {
    if (vj->wheelchair_accessible()) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_wheelchair_accessibility);
    }
    if (vj->bike_accepted()) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_bike_accepted);
    }
    if (vj->air_conditioned()) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_air_conditioned);
    }
    if (vj->visual_announcement()) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_visual_announcement);
    }
    if (vj->audible_announcement()) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_audible_announcement);
    }
    if (vj->appropriate_escort()) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_appropriate_escort);
    }
    if (vj->appropriate_signage()) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_appropriate_signage);
    }
    if (vj->school_vehicle()) {
        has_equipments->add_has_equipments(pbnavitia::hasEquipments::has_school_vehicle);
    }
}

void PbCreator::Filler::fill_pb_object(const StopTimeCalendar* stop_time_calendar,
                                       pbnavitia::ScheduleStopTime* rs_date_time) {
    if (stop_time_calendar->stop_time == nullptr) {
        // we need to represent a 'null' value (for not found datetime)
        // before it was done with a empty string, but now it is the max value (since 0 is a valid value)
        rs_date_time->set_time(std::numeric_limits<u_int64_t>::max());
        return;
    }

    rs_date_time->set_time(navitia::DateTimeUtils::hour(stop_time_calendar->date_time));

    rs_date_time->set_realtime_level(
        to_pb_realtime_level(stop_time_calendar->stop_time->vehicle_journey->realtime_level));

    if (!stop_time_calendar->calendar_id) {
        // for calendar we don't want to have a date
        rs_date_time->set_date(
            navitia::to_int_date(navitia::to_posix_time(stop_time_calendar->date_time, *pb_creator.data)));

        if (auto base_st = stop_time_calendar->stop_time->get_base_stop_time()) {
            auto base_dt = get_date_time(routing::StopEvent::pick_up, stop_time_calendar->stop_time, base_st,
                                         navitia::to_posix_time(stop_time_calendar->date_time, *pb_creator.data), true);
            rs_date_time->set_base_date_time(to_posix_timestamp(base_dt));
        }

        const std::vector<const nt::StopTime*> stop_times = {stop_time_calendar->stop_time};
        const auto meta_vj = stop_time_calendar->stop_time->vehicle_journey->meta_vj;
        for (const auto& message : meta_vj->get_applicable_messages(pb_creator.now, pb_creator.action_period)) {
            if (message->is_relevant(stop_times)) {
                fill_message(message, rs_date_time);
            }
        }
    }

    pbnavitia::Properties* hn = rs_date_time->mutable_properties();
    fill_with_creator(stop_time_calendar->stop_time, [&]() { return hn; });

    // principal destination
    if (!is_principal_destination(stop_time_calendar->stop_time)) {
        auto sa = stop_time_calendar->stop_time->vehicle_journey->stop_time_list.back().stop_point->stop_area;
        pbnavitia::Destination* destination = hn->mutable_destination();
        std::hash<std::string> hash_fn;
        destination->set_uri("destination:" + std::to_string(hash_fn(sa->name)));
        destination->set_destination(sa->name);
    }
    fill(pb_creator.data->pt_data->comments.get(*stop_time_calendar->stop_time), hn->mutable_notes());
    if (stop_time_calendar->stop_time->vehicle_journey != nullptr) {
        if (!stop_time_calendar->stop_time->vehicle_journey->odt_message.empty()) {
            fill_with_creator(&stop_time_calendar->stop_time->vehicle_journey->odt_message,
                              [&]() { return hn->add_notes(); });
        }
        auto* properties = rs_date_time->mutable_properties();
        properties->set_vehicle_journey_id(stop_time_calendar->stop_time->vehicle_journey->uri);
        for (const auto& code :
             pb_creator.data->pt_data->codes.get_codes(stop_time_calendar->stop_time->vehicle_journey)) {
            for (const auto& value : code.second) {
                auto* pb_code = properties->add_vehicle_journey_codes();
                pb_code->set_type(code.first);
                pb_code->set_value(value);
            }
        }
    }

    if ((stop_time_calendar->calendar_id) && (stop_time_calendar->stop_time->vehicle_journey != nullptr)) {
        auto asso_cal_it = stop_time_calendar->stop_time->vehicle_journey->meta_vj->associated_calendars.find(
            *stop_time_calendar->calendar_id);
        if (asso_cal_it != stop_time_calendar->stop_time->vehicle_journey->meta_vj->associated_calendars.end()) {
            fill(asso_cal_it->second->exceptions, hn->mutable_exceptions());
        }
    }

    // For a skipped_stop we re-init time, base_date_time and realtime_level to have empty object data_time
    // with additional_informations and links filled in navitia
    if (stop_time_calendar->stop_time->skipped_stop()) {
        rs_date_time->set_time(std::numeric_limits<u_int64_t>::max());
        rs_date_time->clear_base_date_time();
        rs_date_time->clear_realtime_level();
    }
}

void PbCreator::Filler::fill_pb_object(const nt::EntryPoint* point, pbnavitia::PtObject* place) {
    if (point->type == nt::Type_e::StopPoint) {
        const auto it = pb_creator.data->pt_data->stop_points_map.find(point->uri);
        if (it != pb_creator.data->pt_data->stop_points_map.end()) {
            fill_with_creator(it->second, [&]() { return place; });
        }
    } else if (point->type == nt::Type_e::StopArea) {
        const auto it = pb_creator.data->pt_data->stop_areas_map.find(point->uri);
        if (it != pb_creator.data->pt_data->stop_areas_map.end()) {
            fill_with_creator(it->second, [&]() { return place; });
        }
    } else if (point->type == nt::Type_e::POI) {
        const auto it = pb_creator.data->geo_ref->poi_map.find(point->uri);
        if (it != pb_creator.data->geo_ref->poi_map.end()) {
            fill_with_creator(it->second, [&]() { return place; });
        }
    } else if (point->type == nt::Type_e::Admin) {
        const auto it = pb_creator.data->geo_ref->admin_map.find(point->uri);
        if (it != pb_creator.data->geo_ref->admin_map.end()) {
            fill_with_creator(pb_creator.data->geo_ref->admins[it->second], [&]() { return place; });
        }
    } else if (point->type == nt::Type_e::Coord) {
        try {
            auto address = pb_creator.data->geo_ref->nearest_addr(point->coordinates);
            const auto& ng_address = navitia::georef::Address(address.second, point->coordinates, address.first);
            fill_pb_object(&ng_address, place);
        } catch (const navitia::proximitylist::NotFound&) {
            // we didn't find a address at this coordinate, we fill the address manually with the coord, so we have a
            // valid output
            place->set_name("");
            place->set_uri(point->coordinates.uri());
            place->set_embedded_type(pbnavitia::ADDRESS);
            auto addr = place->mutable_address();
            auto c = addr->mutable_coord();
            c->set_lon(point->coordinates.lon());
            c->set_lat(point->coordinates.lat());
        }
    }
}

void PbCreator::Filler::fill_pb_object(const navitia::georef::Address* ng_address, pbnavitia::PtObject* place) {
    if (ng_address->way == nullptr) {
        return;
    }

    copy(depth, dump_message_options).fill_pb_object(ng_address, place->mutable_address());

    place->set_name(place->address().label());

    place->set_uri(place->address().uri());
    place->set_embedded_type(pbnavitia::ADDRESS);
}

void PbCreator::Filler::fill_pb_object(const nt::Contributor* c, pbnavitia::FeedPublisher* fp) {
    fp->set_id(c->uri);
    fp->set_name(c->name);
    fp->set_license(c->license);
    fp->set_url(c->website);
}

template <typename N>
void PbCreator::pb_fill(const std::vector<N*>& nav_list, int depth, const DumpMessageOptions& dump_message_options) {
    auto* pb_object = get_mutable<typename std::remove_cv<N>::type>(response);
    Filler(depth, dump_message_options, *this).fill_pb_object(nav_list, pb_object);
}

template void PbCreator::pb_fill(const std::vector<ng::POI*>&, int, const DumpMessageOptions&);
template void PbCreator::pb_fill(const std::vector<ng::POIType*>&, int, const DumpMessageOptions&);
template void PbCreator::pb_fill(const std::vector<nt::Calendar*>&, int, const DumpMessageOptions&);
template void PbCreator::pb_fill(const std::vector<nt::CommercialMode*>&, int, const DumpMessageOptions&);
template void PbCreator::pb_fill(const std::vector<nt::Company*>&, int, const DumpMessageOptions&);
template void PbCreator::pb_fill(const std::vector<nt::Contributor*>&, int, const DumpMessageOptions&);
template void PbCreator::pb_fill(const std::vector<nt::Dataset*>&, int, const DumpMessageOptions&);
template void PbCreator::pb_fill(const std::vector<nt::Line*>&, int, const DumpMessageOptions&);
template void PbCreator::pb_fill(const std::vector<nt::LineGroup*>&, int, const DumpMessageOptions&);
template void PbCreator::pb_fill(const std::vector<nt::Network*>&, int, const DumpMessageOptions&);
template void PbCreator::pb_fill(const std::vector<nt::PhysicalMode*>&, int, const DumpMessageOptions&);
template void PbCreator::pb_fill(const std::vector<nt::Route*>&, int, const DumpMessageOptions&);
template void PbCreator::pb_fill(const std::vector<const nt::Route*>&, int, const DumpMessageOptions&);
template void PbCreator::pb_fill(const std::vector<nt::StopArea*>&, int, const DumpMessageOptions&);
template void PbCreator::pb_fill(const std::vector<nt::StopPoint*>&, int, const DumpMessageOptions&);
template void PbCreator::pb_fill(const std::vector<const nt::StopPoint*>&, int, const DumpMessageOptions&);
template void PbCreator::pb_fill(const std::vector<nt::StopPointConnection*>&, int, const DumpMessageOptions&);
template void PbCreator::pb_fill(const std::vector<nt::ValidityPattern*>&, int, const DumpMessageOptions&);
template void PbCreator::pb_fill(const std::vector<nt::VehicleJourney*>&, int, const DumpMessageOptions&);

const type::disruption::Impact* PbCreator::get_impact(const std::string& uri) const {
    for (const auto& impact : impacts) {
        if (impact->uri == uri) {
            return impact.get();
        }
    }
    return nullptr;
}

const std::string& PbCreator::register_section(pbnavitia::Journey* j, size_t section_idx) {
    routing_section_map[{j, section_idx}] = "section_" + std::to_string(nb_sections++);
    return routing_section_map[{j, section_idx}];
}

std::string PbCreator::register_section() {
    // For some section (transfer, waiting, streetnetwork, corwfly) we don't need info
    // about the item
    return "section_" + std::to_string(nb_sections++);
}

std::string PbCreator::get_section_id(pbnavitia::Journey* j, size_t section_idx) {
    auto it = routing_section_map.find({j, section_idx});
    if (it == routing_section_map.end()) {
        LOG4CPLUS_WARN(log4cplus::Logger::getInstance("logger"),
                       "Impossible to find section id for section idx " << section_idx);
        return "";
    }
    return it->second;
}

std::string PbCreator::get_translated_message(const std::vector<type::disruption::Translation>& translations,
                                              const std::string& language) {
    for (const auto& t : translations) {
        if (!t.text.empty() && (t.language == language)) {
            return t.text;
        }
    }
    return "";
}

void PbCreator::fill_co2_emission_by_mode(pbnavitia::Section* pb_section, const std::string& mode_uri) {
    if (!mode_uri.empty()) {
        const auto it_physical_mode = data->pt_data->physical_modes_map.find(mode_uri);
        if ((it_physical_mode != data->pt_data->physical_modes_map.end()) && (it_physical_mode->second->co2_emission)) {
            pbnavitia::Co2Emission* pb_co2_emission = pb_section->mutable_co2_emission();
            pb_co2_emission->set_unit("gEC");
            pb_co2_emission->set_value((pb_section->length() / 1000.0) * (*it_physical_mode->second->co2_emission));
        }
    }
}

void PbCreator::fill_co2_emission(pbnavitia::Section* pb_section, const type::VehicleJourney* vehicle_journey) {
    if (vehicle_journey && vehicle_journey->physical_mode) {
        this->fill_co2_emission_by_mode(pb_section, vehicle_journey->physical_mode->uri);
    }
}

void PbCreator::fill_fare_section(pbnavitia::Journey* pb_journey, const fare::results& fare) {
    auto pb_fare = pb_journey->mutable_fare();
    fill_fare(pb_fare, pb_journey, fare);
}

void PbCreator::fill_fare(pbnavitia::Fare* pb_fare, pbnavitia::Journey* pb_journey, const fare::results& fare) {
    size_t cpt_ticket = response.tickets_size();

    boost::optional<std::string> currency;
    for (const fare::Ticket& ticket : fare.tickets) {
        if (!ticket.is_default_ticket()) {
            if (!currency) {
                currency = ticket.currency;
            }
            if (ticket.currency != *currency) {
                throw navitia::exception("cannot have different currencies for tickets");
            }  // if we really had to handle different currencies it could be done, but I don't see the point
            // It may happen when not all tickets were found
        }

        pbnavitia::Ticket* pb_ticket = nullptr;
        if (ticket.is_default_ticket()) {
            pb_ticket = response.add_tickets();
            pb_ticket->set_name(ticket.caption);
            pb_ticket->set_found(false);
            pb_ticket->set_id("unknown_ticket_" + std::to_string(++cpt_ticket));
            pb_ticket->set_source_id(ticket.key);
            pb_ticket->set_comment("unknown ticket");
            pb_fare->add_ticket_id(pb_ticket->id());

        } else {
            pb_ticket = response.add_tickets();

            pb_ticket->set_name(ticket.caption);
            pb_ticket->set_found(true);
            pb_ticket->set_comment(ticket.comment);
            pb_ticket->set_id("ticket_" + std::to_string(++cpt_ticket));
            pb_ticket->set_source_id(ticket.key);
            pb_ticket->mutable_cost()->set_currency(*currency);
            pb_ticket->mutable_cost()->set_value(ticket.value.value);
            pb_fare->add_ticket_id(pb_ticket->id());
        }

        for (const auto& section : ticket.sections) {
            if (section.section_id) {
                pb_ticket->add_section_id(*section.section_id);
            } else if (pb_journey) {
                auto section_id = get_section_id(pb_journey, section.path_item_idx);
                pb_ticket->add_section_id(section_id);
            }
        }
    }
    pb_fare->mutable_total()->set_value(fare.total.value);
    if (currency) {
        pb_fare->mutable_total()->set_currency(*currency);
    }
    pb_fare->set_found(!fare.not_found);
}

void PbCreator::add_path_item(pbnavitia::StreetNetwork* sn,
                              const ng::PathItem& item,
                              const type::EntryPoint& ori_dest) {
    if (item.way_idx >= data->geo_ref->ways.size()) {
        throw navitia::exception("Wrong way idx : " + std::to_string(item.way_idx));
    }

    pbnavitia::PathItem* path_item = sn->add_path_items();
    path_item->set_name(data->geo_ref->ways[item.way_idx]->name);
    path_item->set_length(double(item.get_length(ori_dest.streetnetwork_params.speed_factor)));
    path_item->set_duration(double(item.duration.total_fractional_seconds()));
    path_item->set_direction(item.angle);

    // we add each path item coordinate to the global coordinate list
    for (auto coord : item.coordinates) {
        if (!coord.is_initialized()) {
            continue;
        }
        pbnavitia::GeographicalCoord* pb_coord = sn->add_coordinates();
        pb_coord->set_lon(coord.lon());
        pb_coord->set_lat(coord.lat());
    }
}

void PbCreator::fill_street_sections(const type::EntryPoint& ori_dest,
                                     const georef::Path& path,
                                     pbnavitia::Journey* pb_journey,
                                     const pt::ptime departure,
                                     int max_depth) {
    int depth = std::min(max_depth, 3);
    if (path.path_items.empty()) {
        return;
    }

    auto session_departure = departure;
    auto last_transportation_carac = boost::make_optional(false, georef::PathItem::TransportCaracteristic{});
    auto section = create_section(pb_journey, path.path_items.front(), depth);
    georef::PathItem last_item;

    // we create 1 section by mean of transport
    for (const auto& item : path.path_items) {
        auto transport_carac = item.transportation;

        if (last_transportation_carac && transport_carac != *last_transportation_carac) {
            // we end the last section
            finalize_section(section, last_item, item, session_departure, depth);
            session_departure += bt::seconds(section->duration());

            // and we create a new one
            section = create_section(pb_journey, item, depth);
        }

        this->add_path_item(section->mutable_street_network(), item, ori_dest);

        last_transportation_carac = transport_carac;
        last_item = item;
    }

    finalize_section(section, path.path_items.back(), {}, session_departure, depth);

    // We add consistency between origin/destination places and geojson
    auto sections = pb_journey->mutable_sections();
    for (auto& section : *sections) {
        auto destination_coord = get_coord(section.destination());
        auto sn = section.mutable_street_network();
        if (!destination_coord.IsInitialized() || sn->coordinates().empty()) {
            continue;
        }
        auto last_coord = sn->coordinates(sn->coordinates_size() - 1);
        if (last_coord.IsInitialized()
            && type::GeographicalCoord(last_coord.lon(), last_coord.lat())
                   != type::GeographicalCoord(destination_coord.lon(), destination_coord.lat())) {
            pbnavitia::GeographicalCoord* pb_coord = sn->add_coordinates();
            pb_coord->set_lon(destination_coord.lon());
            pb_coord->set_lat(destination_coord.lat());
        }
    }
}

const ng::POI* PbCreator::get_nearest_bss_station(const nt::GeographicalCoord& coord) {
    const auto* poi_type = data->geo_ref->poitype_map["poi_type:amenity:bicycle_rental"];
    return this->get_nearest_poi(coord, *poi_type);
}

const ng::POI* PbCreator::get_nearest_poi(const nt::GeographicalCoord& coord, const ng::POIType& poi_type) {
    // we loop through all poi near the coord to find a poi of the required type
    for (const auto& pair : data->geo_ref->poi_proximity_list.find_within(coord, 500)) {
        const auto poi_idx = pair.first;
        const auto poi = data->geo_ref->pois[poi_idx];
        if (poi->poitype_idx == poi_type.idx) {
            return poi;
        }
    }
    return nullptr;
}

const ng::POI* PbCreator::get_nearest_parking(const nt::GeographicalCoord& coord) {
    const auto* poi_type = data->geo_ref->poitype_map["poi_type:amenity:parking"];
    return get_nearest_poi(coord, *poi_type);
}

pbnavitia::RouteSchedule* PbCreator::add_route_schedules() {
    return response.add_route_schedules();
}

pbnavitia::StopSchedule* PbCreator::add_stop_schedules() {
    return response.add_stop_schedules();
}

pbnavitia::StopSchedule* PbCreator::add_terminus_schedules() {
    return response.add_terminus_schedules();
}

int PbCreator::route_schedules_size() {
    return response.route_schedules_size();
}
pbnavitia::Passage* PbCreator::add_next_departures() {
    return response.add_next_departures();
}

pbnavitia::Passage* PbCreator::add_next_arrivals() {
    return response.add_next_arrivals();
}

pbnavitia::Section* PbCreator::create_section(pbnavitia::Journey* pb_journey,
                                              const ng::PathItem& first_item,
                                              int depth) {
    pbnavitia::Section* prev_section =
        (pb_journey->sections_size() > 0) ? pb_journey->mutable_sections(pb_journey->sections_size() - 1) : nullptr;
    auto section = pb_journey->add_sections();
    section->set_id(this->register_section());
    section->set_type(pbnavitia::STREET_NETWORK);

    pbnavitia::PtObject* orig_place = section->mutable_origin();
    // we want to have a specific place mark for vls or for the departure if we started from a poi
    if (first_item.transportation == georef::PathItem::TransportCaracteristic::BssTake) {
        const auto vls_station = this->get_nearest_bss_station(first_item.coordinates.front());
        if (vls_station) {
            fill(vls_station, section->mutable_destination(), depth);
        } else {
            LOG4CPLUS_TRACE(
                log4cplus::Logger::getInstance("logger"),
                "impossible to find the associated BSS rent station poi for coord " << first_item.coordinates.front());
        }
    }
    if (prev_section) {
        orig_place->CopyFrom(prev_section->destination());
    } else if (first_item.way_idx != nt::invalid_idx) {
        auto way = data->geo_ref->ways[first_item.way_idx];
        type::GeographicalCoord departure_coord = first_item.coordinates.front();
        auto const& ng_address =
            navitia::georef::Address(way, departure_coord, way->nearest_number(departure_coord).first);
        fill(&ng_address, orig_place, depth);
    }

    // NOTE: do we want to add a placemark for crow fly sections (they won't have a proper way) ?

    auto origin_coord = get_coord(section->origin());
    if (origin_coord.IsInitialized() && !first_item.coordinates.empty()
        && first_item.coordinates.front().is_initialized()
        && first_item.coordinates.front() != type::GeographicalCoord(origin_coord.lon(), origin_coord.lat())) {
        pbnavitia::GeographicalCoord* pb_coord = section->mutable_street_network()->add_coordinates();
        pb_coord->set_lon(origin_coord.lon());
        pb_coord->set_lat(origin_coord.lat());
    }

    return section;
}

void PbCreator::finalize_section(pbnavitia::Section* section,
                                 const ng::PathItem& last_item,
                                 const ng::PathItem& item,
                                 const pt::ptime departure,
                                 int depth) {
    double total_duration = 0;
    double total_length = 0;
    for (int pb_item_idx = 0; pb_item_idx < section->mutable_street_network()->path_items_size(); ++pb_item_idx) {
        const pbnavitia::PathItem& pb_item = section->mutable_street_network()->path_items(pb_item_idx);
        total_length += pb_item.length();
        total_duration += pb_item.duration();
    }
    section->mutable_street_network()->set_duration(total_duration);
    section->mutable_street_network()->set_length(total_length);
    section->set_duration(total_duration);
    section->set_length(total_length);

    section->set_begin_date_time(navitia::to_posix_timestamp(departure));
    section->set_end_date_time(navitia::to_posix_timestamp(departure + bt::seconds(long(total_duration))));

    // add the destination as a placemark
    pbnavitia::PtObject* dest_place = section->mutable_destination();

    // we want to have a specific place mark for vls or for the departure if we started from a poi
    switch (item.transportation) {
        case georef::PathItem::TransportCaracteristic::BssPutBack: {
            const auto vls_station = this->get_nearest_bss_station(item.coordinates.front());
            if (vls_station) {
                fill(vls_station, dest_place, depth);
            } else {
                LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("logger"),
                                "impossible to find the associated BSS putback station poi for coord "
                                    << last_item.coordinates.front());
            }
            break;
        }
        case georef::PathItem::TransportCaracteristic::CarPark:
        case georef::PathItem::TransportCaracteristic::CarLeaveParking: {
            const auto parking = this->get_nearest_parking(item.coordinates.front());
            if (parking) {
                fill(parking, dest_place, depth);
            } else {
                LOG4CPLUS_DEBUG(
                    log4cplus::Logger::getInstance("logger"),
                    "impossible to find the associated parking poi for coord " << last_item.coordinates.front());
            }
            break;
        }
        default:
            break;
    }
    if (!dest_place->IsInitialized()) {
        auto way = data->geo_ref->ways[last_item.way_idx];
        type::GeographicalCoord coord = last_item.coordinates.back();
        const auto& ng_address = navitia::georef::Address(way, coord, way->nearest_number(coord).first);
        fill(&ng_address, dest_place, depth);
    }

    switch (last_item.transportation) {
        case georef::PathItem::TransportCaracteristic::Walk:
            section->mutable_street_network()->set_mode(pbnavitia::Walking);
            break;
        case georef::PathItem::TransportCaracteristic::Bike:
            section->mutable_street_network()->set_mode(pbnavitia::Bike);
            this->fill_co2_emission_by_mode(section, "physical_mode:Bike");
            break;
        case georef::PathItem::TransportCaracteristic::Car:
            section->mutable_street_network()->set_mode(pbnavitia::Car);
            this->fill_co2_emission_by_mode(section, "physical_mode:Car");
            break;
        case georef::PathItem::TransportCaracteristic::BssTake:
            section->set_type(pbnavitia::BSS_RENT);
            break;
        case georef::PathItem::TransportCaracteristic::BssPutBack:
            section->set_type(pbnavitia::BSS_PUT_BACK);
            break;
        case georef::PathItem::TransportCaracteristic::CarPark:
            section->set_type(pbnavitia::PARK);
            break;
        case georef::PathItem::TransportCaracteristic::CarLeaveParking:
            section->set_type(pbnavitia::LEAVE_PARKING);
            break;
        default:
            throw navitia::exception("Unhandled TransportCaracteristic value in pb_converter");
    }
}

void PbCreator::fill_crowfly_section(const type::EntryPoint& origin,
                                     const type::EntryPoint& destination,
                                     const time_duration& crow_fly_duration,
                                     type::Mode_e mode,
                                     pt::ptime origin_time,
                                     pbnavitia::Journey* pb_journey) {
    pbnavitia::Section* section = pb_journey->add_sections();
    section->set_id(this->register_section());

    fill(&origin, section->mutable_origin(), 2);
    fill(&destination, section->mutable_destination(), 2);

    // we want to store the transportation mode used
    switch (mode) {
        case type::Mode_e::Walking:
            section->mutable_street_network()->set_mode(pbnavitia::Walking);
            break;
        case type::Mode_e::Bike:
            section->mutable_street_network()->set_mode(pbnavitia::Bike);
            break;
        case type::Mode_e::Car:
        case type::Mode_e::CarNoPark:
            section->mutable_street_network()->set_mode(pbnavitia::Car);
            break;
        case type::Mode_e::Bss:
            section->mutable_street_network()->set_mode(pbnavitia::Bss);
            break;
        default:
            throw navitia::exception("Unhandled TransportCaracteristic value in pb_converter");
    }

    section->set_begin_date_time(navitia::to_posix_timestamp(origin_time));
    section->set_duration(crow_fly_duration.total_seconds());
    if (crow_fly_duration.total_seconds() > 0) {
        section->set_length(origin.coordinates.distance_to(destination.coordinates));
        auto* new_coord = section->add_shape();
        new_coord->set_lon(origin.coordinates.lon());
        new_coord->set_lat(origin.coordinates.lat());
        new_coord = section->add_shape();
        new_coord->set_lon(destination.coordinates.lon());
        new_coord->set_lat(destination.coordinates.lat());
    } else {
        section->set_length(0);
        // For teleportation crow_fly (duration=0), the mode is always 'walking'
        section->mutable_street_network()->set_mode(pbnavitia::Walking);
    }
    section->set_end_date_time(navitia::to_posix_timestamp(origin_time + crow_fly_duration.to_posix()));
    section->set_type(pbnavitia::SectionType::CROW_FLY);
}

void PbCreator::fill_pb_error(const pbnavitia::Error::error_id id,
                              const pbnavitia::ResponseType& resp_type,
                              const std::string& message) {
    fill_pb_error(id, message);
    response.set_response_type(resp_type);
}

void PbCreator::fill_pb_error(const pbnavitia::Error::error_id id, const std::string& message) {
    pbnavitia::Error* error = response.mutable_error();
    error->set_id(id);
    error->set_message(message);
}

const pbnavitia::Response& PbCreator::get_response() {
    Filler(0, {DumpMessage::No}, *this).fill_pb_object(contributors, response.mutable_feed_publishers());
    contributors.clear();
    Filler(0, {DumpMessage::No}, *this).fill_pb_object(impacts, response.mutable_impacts());
    impacts.clear();
    Filler(0, {DumpMessage::No}, *this).fill_pb_object(terminus, response.mutable_terminus());
    terminus.clear();
    return response;
}

void PbCreator::fill_additional_informations(google::protobuf::RepeatedField<int>* infos,
                                             const bool has_datetime_estimated,
                                             const bool has_odt,
                                             const bool is_zonal) {
    if (has_datetime_estimated) {
        infos->Add(pbnavitia::HAS_DATETIME_ESTIMATED);
    }
    if (is_zonal) {
        infos->Add(pbnavitia::ODT_WITH_ZONE);
    } else if (has_odt && has_datetime_estimated) {
        infos->Add(pbnavitia::ODT_WITH_STOP_POINT);
    } else if (has_odt) {
        infos->Add(pbnavitia::ODT_WITH_STOP_TIME);
    }
    if (infos->empty()) {
        infos->Add(pbnavitia::REGULAR);
    }
}

pbnavitia::GeographicalCoord PbCreator::get_coord(const pbnavitia::PtObject& pt_object) {
    switch (pt_object.embedded_type()) {
        case pbnavitia::NavitiaType::STOP_AREA:
            return pt_object.stop_area().coord();
        case pbnavitia::NavitiaType::STOP_POINT:
            return pt_object.stop_point().coord();
        case pbnavitia::NavitiaType::POI:
            return pt_object.poi().coord();
        case pbnavitia::NavitiaType::ADDRESS:
            return pt_object.address().coord();
        default:
            return pbnavitia::GeographicalCoord();
    }
}

pbnavitia::PtObject* PbCreator::add_places_nearby() {
    return response.add_places_nearby();
}

pbnavitia::PtObject* PbCreator::add_places() {
    return response.add_places();
}

pbnavitia::TrafficReports* PbCreator::add_traffic_reports() {
    return response.add_traffic_reports();
}

pbnavitia::LineReport* PbCreator::add_line_reports() {
    return response.add_line_reports();
}

pbnavitia::NearestStopPoint* PbCreator::add_nearest_stop_points() {
    return response.add_nearest_stop_points();
}

pbnavitia::JourneyPattern* PbCreator::add_journey_patterns() {
    return response.add_journey_patterns();
}

pbnavitia::JourneyPatternPoint* PbCreator::add_journey_pattern_points() {
    return response.add_journey_pattern_points();
}

pbnavitia::Trip* PbCreator::add_trips() {
    return response.add_trips();
}

pbnavitia::Impact* PbCreator::add_impacts() {
    return response.add_impacts();
}

pbnavitia::RoutePoint* PbCreator::add_route_points() {
    return response.add_route_points();
}

pbnavitia::Journey* PbCreator::add_journeys() {
    return response.add_journeys();
}

pbnavitia::GraphicalIsochrone* PbCreator::add_graphical_isochrones() {
    return response.add_graphical_isochrones();
}

pbnavitia::HeatMap* PbCreator::add_heat_maps() {
    return response.add_heat_maps();
}

pbnavitia::EquipmentReport* PbCreator::add_equipment_reports() {
    return response.add_equipment_reports();
}

pbnavitia::VehiclePosition* PbCreator::add_vehicle_positions() {
    return response.add_vehicle_positions();
}

pbnavitia::AccessPoint* PbCreator::add_access_points() {
    return response.add_access_points();
}

pbnavitia::PtJourneyFare* PbCreator::add_pt_journey_fares() {
    return response.add_pt_journey_fares();
}

bool PbCreator::has_error() {
    return response.has_error();
}

bool PbCreator::has_response_type(const pbnavitia::ResponseType& resp_type) {
    return resp_type == response.response_type();
}

void PbCreator::set_response_type(const pbnavitia::ResponseType& resp_type) {
    response.set_response_type(resp_type);
}

::google::protobuf::RepeatedPtrField<pbnavitia::PtObject>* PbCreator::get_mutable_places() {
    return response.mutable_places();
}

void PbCreator::make_paginate(const int total_result,
                              const int start_page,
                              const int items_per_page,
                              const int items_on_page) {
    auto pagination = response.mutable_pagination();
    pagination->set_totalresult(total_result);
    pagination->set_startpage(start_page);
    pagination->set_itemsperpage(items_per_page);
    pagination->set_itemsonpage(items_on_page);
}

int PbCreator::departure_boards_size() {
    return response.departure_boards_size();
}

int PbCreator::terminus_schedules_size() {
    return response.terminus_schedules_size();
}

int PbCreator::stop_schedules_size() {
    return response.stop_schedules_size();
}

int PbCreator::traffic_reports_size() {
    return response.traffic_reports_size();
}

int PbCreator::line_reports_size() {
    return response.line_reports_size();
}

int PbCreator::calendars_size() {
    return response.calendars_size();
}

int PbCreator::equipment_reports_size() {
    return response.equipment_reports_size();
}

int PbCreator::vehicle_positions_size() {
    return response.vehicle_positions_size();
}

void PbCreator::sort_journeys() {
    std::sort(response.mutable_journeys()->begin(), response.mutable_journeys()->end(),
              [](const pbnavitia::Journey& journey1, const pbnavitia::Journey& journey2) {
                  auto duration1 = journey1.duration(), duration2 = journey2.duration();
                  if (duration1 != duration2) {
                      return duration1 < duration2;
                  }
                  return journey1.destination().uri() < journey2.destination().uri();
              });
}

bool PbCreator::empty_journeys() {
    return (response.journeys().empty());
}

pbnavitia::GeoStatus* PbCreator::mutable_geo_status() {
    return response.mutable_geo_status();
}

pbnavitia::Status* PbCreator::mutable_status() {
    return response.mutable_status();
}

pbnavitia::Pagination* PbCreator::mutable_pagination() {
    return response.mutable_pagination();
}

pbnavitia::Co2Emission* PbCreator::mutable_car_co2_emission() {
    return response.mutable_car_co2_emission();
}

pbnavitia::StreetNetworkRoutingMatrix* PbCreator::mutable_sn_routing_matrix() {
    return response.mutable_sn_routing_matrix();
}

pbnavitia::Metadatas* PbCreator::mutable_metadatas() {
    return response.mutable_metadatas();
}

void PbCreator::clear_feed_publishers() {
    contributors.clear();
}

pbnavitia::FeedPublisher* PbCreator::add_feed_publishers() {
    return response.add_feed_publishers();
}

void PbCreator::set_publication_date(pt::ptime ptime) {
    response.set_publication_date(navitia::to_posix_timestamp(ptime));
}

void PbCreator::set_next_request_date_time(uint32_t next_request_date_time) {
    response.set_next_request_date_time(next_request_date_time);
}

}  // namespace navitia
