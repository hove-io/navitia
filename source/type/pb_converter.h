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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once
#include <utility>

#include "data.h"
#include "type/type.pb.h"
#include "type/response.pb.h"
#include "type/company.h"
#include "type/network.h"
#include "type/contributor.h"
#include "type/dataset.h"
#include "type/physical_mode.h"
#include "type/pt_data.h"
#include "type/static_data.h"
#include "vptranslator/vptranslator.h"
#include "ptreferential/ptreferential.h"
#include "utils/logger.h"

namespace pt = boost::posix_time;
namespace nt = navitia::type;
namespace ng = navitia::georef;

// we don't use a boolean to have a more type checks
enum class DumpMessage : bool { Yes, No };

enum class DumpLineSectionMessage : bool { Yes, No };
enum class DumpRailSectionMessage : bool { Yes, No };

struct DumpMessageOptions {
    DumpMessage dump_message;
    DumpLineSectionMessage dump_line_section;
    DumpRailSectionMessage dump_rail_section;
    const nt::Line* line;  // specific line to retrieve only related line-sections
    constexpr DumpMessageOptions(DumpMessage dump_message = DumpMessage::Yes,
                                 DumpLineSectionMessage dump_line_section = DumpLineSectionMessage::No,
                                 const nt::Line* line = nullptr,
                                 DumpRailSectionMessage dump_rail_section = DumpRailSectionMessage::No)
        : dump_message(dump_message),
          dump_line_section(dump_line_section),
          dump_rail_section(dump_rail_section),
          line(line) {}
};

static const auto null_time_period = pt::time_period(pt::not_a_date_time, pt::seconds(0));

// forward declare
namespace navitia {
namespace routing {
struct PathItem;
struct JourneyPattern;
struct JourneyPatternPoint;
using JppIdx = Idx<JourneyPatternPoint>;
using JpIdx = Idx<JourneyPattern>;
}  // namespace routing
namespace georef {
struct PathItem;
struct Way;
struct POI;
struct Path;
struct POIType;
}  // namespace georef
namespace fare {
struct results;
struct Ticket;
}  // namespace fare
namespace timetables {
struct Thermometer;
}
}  // namespace navitia

using jp_pair = std::pair<const navitia::routing::JpIdx, const navitia::routing::JourneyPattern&>;
using jpp_pair = std::pair<const navitia::routing::JppIdx, const navitia::routing::JourneyPatternPoint&>;

namespace navitia {

struct VjOrigDest {
    const nt::VehicleJourney* vj;
    const nt::StopPoint* orig;
    const nt::StopPoint* dest;
    VjOrigDest(const nt::VehicleJourney* vj, nt::StopPoint* orig, nt::StopPoint* dest)
        : vj(vj), orig(orig), dest(dest) {}
};

struct VjStopTimes {
    const nt::VehicleJourney* vj;
    const std::vector<const nt::StopTime*>& stop_times;
    VjStopTimes(const nt::VehicleJourney* vj, const std::vector<const nt::StopTime*>& st) : vj(vj), stop_times(st) {}
};

struct StopTimeCalendar {
    const nt::StopTime* stop_time;
    const navitia::DateTime& date_time;
    boost::optional<const std::string> calendar_id;
    StopTimeCalendar(const nt::StopTime* stop_time,
                     const navitia::DateTime& date_time,
                     boost::optional<const std::string> calendar_id)
        : stop_time(stop_time), date_time(date_time), calendar_id(std::move(calendar_id)) {}
};

/**
 * get_label() is a function that returns:
 * * the label (or a function get_label()) if the object has it
 * * else return the name of the object
 *
 * Note: the trick is that int is a better match for 0 than
 * long and template resolution takes the best match
 */
namespace detail {
template <typename T>
auto get_label_if_exists(const T* v, int) -> decltype(v->label) {
    return v->label;
}

template <typename T>
auto get_label_if_exists(const T* v, int) -> decltype(v->get_label()) {
    return v->get_label();
}

template <typename T>
auto get_label_if_exists(const T* v, long) -> decltype(v->name) {
    return v->name;
}
}  // namespace detail

template <typename T>
std::string get_label(const T* v) {
    return detail::get_label_if_exists(v, 0);
}
inline pbnavitia::NavitiaType get_embedded_type(const nt::Calendar*) {
    return pbnavitia::CALENDAR;
}
inline pbnavitia::NavitiaType get_embedded_type(const nt::VehicleJourney*) {
    return pbnavitia::VEHICLE_JOURNEY;
}
inline pbnavitia::NavitiaType get_embedded_type(const nt::Line*) {
    return pbnavitia::LINE;
}
inline pbnavitia::NavitiaType get_embedded_type(const nt::Route*) {
    return pbnavitia::ROUTE;
}
inline pbnavitia::NavitiaType get_embedded_type(const nt::Company*) {
    return pbnavitia::COMPANY;
}
inline pbnavitia::NavitiaType get_embedded_type(const nt::Network*) {
    return pbnavitia::NETWORK;
}
inline pbnavitia::NavitiaType get_embedded_type(const ng::POI*) {
    return pbnavitia::POI;
}
inline pbnavitia::NavitiaType get_embedded_type(const ng::Admin*) {
    return pbnavitia::ADMINISTRATIVE_REGION;
}
inline pbnavitia::NavitiaType get_embedded_type(const nt::StopArea*) {
    return pbnavitia::STOP_AREA;
}
inline pbnavitia::NavitiaType get_embedded_type(const nt::StopPoint*) {
    return pbnavitia::STOP_POINT;
}
inline pbnavitia::NavitiaType get_embedded_type(const nt::CommercialMode*) {
    return pbnavitia::COMMERCIAL_MODE;
}
inline pbnavitia::NavitiaType get_embedded_type(const nt::MetaVehicleJourney*) {
    return pbnavitia::TRIP;
}

struct PbCreator {
    std::set<const nt::Contributor*, Less> contributors;
    // Used for journeys
    std::set<const nt::StopArea*, Less> terminus;
    std::set<boost::shared_ptr<type::disruption::Impact>, Less> impacts;
    // std::reference_wrapper<const nt::Data> data;
    const nt::Data* data = nullptr;
    pt::ptime now;
    pt::time_period action_period = null_time_period;
    // Do we need to disable geojson in the request
    bool disable_geojson = false;
    bool disable_feedpublisher = false;
    bool disable_disruption = false;
    // Raptor api
    size_t nb_sections = 0;
    std::map<std::pair<pbnavitia::Journey*, size_t>, std::string> routing_section_map;

    PbCreator() = default;

    PbCreator(const nt::Data* data,
              const pt::ptime now,
              const pt::time_period action_period,
              const bool disable_geojson = false,
              const bool disable_feedpublisher = false,
              const bool disable_disruption = false)
        : data(data),
          now(now),
          action_period(action_period),
          disable_geojson(disable_geojson),
          disable_feedpublisher(disable_feedpublisher),
          disable_disruption(disable_disruption) {}

    void init(const nt::Data* data,
              const pt::ptime now,
              const pt::time_period action_period,
              const bool disable_geojson = false,
              const bool disable_feedpublisher = false,
              const bool disable_disruption = false) {
        this->data = data;
        this->now = now;
        this->action_period = action_period;
        this->disable_geojson = disable_geojson;
        this->disable_feedpublisher = disable_feedpublisher;
        this->disable_disruption = disable_disruption;
        this->nb_sections = 0;

        this->contributors.clear();
        this->impacts.clear();
        this->routing_section_map.clear();
        this->response.Clear();
    }

    PbCreator(const PbCreator&) = delete;
    PbCreator& operator=(const PbCreator&) = delete;

    template <typename N, typename P>
    void fill(const N& item,
              P* proto,
              int depth,
              const DumpMessageOptions& dump_message_options = DumpMessageOptions{}) {
        Filler(depth, dump_message_options, *this).fill_pb_object(item, proto);
    }

    template <typename N>
    void fill(const N& item, int depth, const DumpMessageOptions& dump_message_options = DumpMessageOptions{}) {
        Filler(depth, dump_message_options, *this).fill_pb_object(item, &response);
    }

    template <typename N>
    void pb_fill(const std::vector<N*>& nav_list,
                 int depth,
                 const DumpMessageOptions& dump_message_options = DumpMessageOptions{});

    template <typename P>
    void fill_message(const boost::shared_ptr<nt::disruption::Impact>& impact, P pb_object, int depth) {
        Filler(depth, DumpMessageOptions{}, *this).fill_message(impact, pb_object);
    }

    const type::disruption::Impact* get_impact(const std::string& uri) const;

    // Raptor api
    const std::string& register_section(pbnavitia::Journey* j, size_t section_idx);
    std::string register_section();
    std::string get_section_id(pbnavitia::Journey* j, size_t section_idx);
    void fill_co2_emission(pbnavitia::Section* pb_section, const type::VehicleJourney* vehicle_journey);
    void fill_co2_emission_by_mode(pbnavitia::Section* pb_section, const std::string& mode_uri);
    void fill_fare_section(pbnavitia::Journey* pb_journey, const fare::results& fare);

    void fill_crowfly_section(const type::EntryPoint& origin,
                              const type::EntryPoint& destination,
                              const time_duration& crow_fly_duration,
                              type::Mode_e mode,
                              pt::ptime origin_time,
                              pbnavitia::Journey* pb_journey);

    void fill_street_sections(const type::EntryPoint& ori_dest,
                              const georef::Path& path,
                              pbnavitia::Journey* pb_journey,
                              const pt::ptime departure,
                              int max_depth = 1);

    void add_path_item(pbnavitia::StreetNetwork* sn, const ng::PathItem& item, const type::EntryPoint& ori_dest);

    void fill_additional_informations(google::protobuf::RepeatedField<int>* infos,
                                      const bool has_datetime_estimated,
                                      const bool has_odt,
                                      const bool is_zonal);

    void fill_pb_error(const pbnavitia::Error::error_id, const pbnavitia::ResponseType&, const std::string&);
    void fill_pb_error(const pbnavitia::Error::error_id, const std::string&);
    const pbnavitia::Response& get_response();
    void clear_feed_publishers();

    pbnavitia::PtObject* add_places_nearby();
    pbnavitia::Journey* add_journeys();
    pbnavitia::GraphicalIsochrone* add_graphical_isochrones();
    pbnavitia::HeatMap* add_heat_maps();
    pbnavitia::PtObject* add_places();
    pbnavitia::TrafficReports* add_traffic_reports();
    pbnavitia::LineReport* add_line_reports();
    pbnavitia::NearestStopPoint* add_nearest_stop_points();
    pbnavitia::JourneyPattern* add_journey_patterns();
    pbnavitia::JourneyPatternPoint* add_journey_pattern_points();
    pbnavitia::Trip* add_trips();
    pbnavitia::Impact* add_impacts();
    pbnavitia::RoutePoint* add_route_points();
    pbnavitia::EquipmentReport* add_equipment_reports();
    pbnavitia::VehiclePosition* add_vehicle_positions();
    pbnavitia::AccessPoint* add_access_points();

    ::google::protobuf::RepeatedPtrField<pbnavitia::PtObject>* get_mutable_places();
    bool has_error();
    bool has_response_type(const pbnavitia::ResponseType& resp_type);
    void set_response_type(const pbnavitia::ResponseType& resp_type);
    void sort_journeys();
    bool empty_journeys();
    pbnavitia::RouteSchedule* add_route_schedules();
    pbnavitia::StopSchedule* add_stop_schedules();
    pbnavitia::StopSchedule* add_terminus_schedules();
    int route_schedules_size();
    pbnavitia::Passage* add_next_departures();
    pbnavitia::Passage* add_next_arrivals();
    void make_paginate(const int, const int, const int, const int);
    int departure_boards_size();
    int terminus_schedules_size();
    int stop_schedules_size();
    int traffic_reports_size();
    int line_reports_size();
    int calendars_size();
    int equipment_reports_size();
    int vehicle_positions_size();
    pbnavitia::GeoStatus* mutable_geo_status();
    pbnavitia::Status* mutable_status();
    pbnavitia::Pagination* mutable_pagination();
    pbnavitia::Co2Emission* mutable_car_co2_emission();
    pbnavitia::StreetNetworkRoutingMatrix* mutable_sn_routing_matrix();
    pbnavitia::Metadatas* mutable_metadatas();
    pbnavitia::FeedPublisher* add_feed_publishers();
    void set_publication_date(pt::ptime ptime);
    void set_next_request_date_time(uint32_t next_request_date_time);

private:
    pbnavitia::Response response;
    struct Filler {
        struct PtObjVisitor;
        const int depth;
        DumpMessageOptions dump_message_options;
        PbCreator& pb_creator;

        Filler(int depth, const DumpMessageOptions& dump_message_options, PbCreator& pb_creator)
            : depth(depth), dump_message_options(dump_message_options), pb_creator(pb_creator) {}

        Filler copy(int, const DumpMessageOptions&);

        template <typename NAV, typename PB>
        void fill(NAV* nav_object, PB* pb_object);
        template <typename NAV, typename F>
        void fill_with_creator(NAV* nav_object, F creator);

        template <typename NAV, typename PB>
        void fill(const NAV& nav_object, PB* pb_object) {
            copy(depth - 1, dump_message_options).fill_pb_object(nav_object, pb_object);
        }

        template <typename Nav, typename Pb>
        void fill_pb_object(const std::vector<Nav*>& nav_list, ::google::protobuf::RepeatedPtrField<Pb>* pb_list) {
            for (const auto* nav_obj : nav_list) {
                fill_pb_object(nav_obj, pb_list->Add());
            }
        }

        template <typename Nav, typename Pb>
        void fill_pb_object(const std::vector<Nav>& nav_list, ::google::protobuf::RepeatedPtrField<Pb>* pb_list) {
            for (auto& nav_obj : nav_list) {
                fill_pb_object(&nav_obj, pb_list->Add());
            }
        }

        template <typename Nav, typename Cmp, typename Pb>
        void fill_pb_object(const std::set<Nav, Cmp>& nav_list, ::google::protobuf::RepeatedPtrField<Pb>* pb_list) {
            for (auto& nav_obj : nav_list) {
                fill_pb_object(&nav_obj, pb_list->Add());
            }
        }
        template <typename Nav, typename Cmp, typename Pb>
        void fill_pb_object(const std::set<Nav*, Cmp>& nav_list, ::google::protobuf::RepeatedPtrField<Pb>* pb_list) {
            for (auto* nav_obj : nav_list) {
                fill_pb_object(nav_obj, pb_list->Add());
            }
        }
        template <typename Nav, typename Cmp, typename Pb>
        void fill_pb_object(const std::set<boost::shared_ptr<Nav>, Cmp>& nav_list,
                            ::google::protobuf::RepeatedPtrField<Pb>* pb_list) {
            for (auto& nav_obj : nav_list) {
                fill_pb_object(nav_obj.get(), pb_list->Add());
            }
        }

        template <typename P>
        void fill_messages(const nt::HasMessages* nav_obj, P* pb_obj) {
            if (nav_obj == nullptr) {
                return;
            }
            if (dump_message_options.dump_message == DumpMessage::No) {
                return;
            }
            const bool dump_line_sections = dump_message_options.dump_line_section == DumpLineSectionMessage::Yes;
            const bool dump_rail_sections = dump_message_options.dump_rail_section == DumpRailSectionMessage::Yes;
            for (const auto& message : nav_obj->get_applicable_messages(pb_creator.now, pb_creator.action_period)) {
                if (message->is_only_line_section()) {
                    if (!dump_line_sections) {
                        continue;
                    }
                    if (dump_message_options.line != nullptr
                        && !message->is_line_section_of(*dump_message_options.line)) {
                        continue;  // if a specific line is requested: dumping only related line_sections' disruptions
                    }
                }
                if (message->is_only_rail_section()) {
                    if (!dump_rail_sections) {
                        continue;
                    }
                    if (dump_message_options.line != nullptr
                        && !message->is_rail_section_of(*dump_message_options.line)) {
                        continue;  // if a specific line is requested: dumping only related rail_sections' disruptions
                    }
                }
                fill_message(message, pb_obj);
            }
        }
        // override fill_messages for journey sections to handle line sections impacts
        void fill_messages(const VjStopTimes*, pbnavitia::PtDisplayInfo*);

        template <typename Target, typename Source>
        std::vector<Target*> ptref_indexes(const Source* nav_obj);

        template <typename T>
        void add_contributor(const T* nav);

        template <typename NT, typename PB>
        void fill_codes(const NT* nt, PB* pb) {
            if (nt == nullptr) {
                return;
            }
            for (const auto& code : pb_creator.data->pt_data->codes.get_codes(nt)) {
                for (const auto& value : code.second) {
                    auto* pb_code = pb->add_codes();
                    pb_code->set_type(code.first);
                    pb_code->set_value(value);
                }
            }
        }

        template <typename NT, typename PB>
        void fill_comments(const NT* nt, PB* pb) {
            if (nt == nullptr) {
                return;
            }
            for (const auto& comment : pb_creator.data->pt_data->comments.get(nt)) {
                auto com = pb->add_comments();
                auto type = comment.type.empty() ? "information" : comment.type;
                com->set_value(comment.value);
                com->set_type(comment.type);
            }
        }

        template <typename P>
        void fill_message(const boost::shared_ptr<nt::disruption::Impact>& impact, P pb_object);

        void fill_informed_entity(const nt::disruption::PtObj& ptobj,
                                  const nt::disruption::Impact& impact,
                                  pbnavitia::Impact* pb_impact);

        void fill_pb_object(const nt::Contributor*, pbnavitia::FeedPublisher*);
        void fill_pb_object(const nt::Contributor*, pbnavitia::Contributor*);
        void fill_pb_object(const nt::Dataset*, pbnavitia::Dataset*);
        void fill_pb_object(const nt::StopArea*, pbnavitia::StopArea*);
        void fill_pb_object(const nt::StopPoint*, pbnavitia::StopPoint*);
        void fill_pb_object(const nt::Company*, pbnavitia::Company*);
        void fill_pb_object(const nt::Network*, pbnavitia::Network*);
        void fill_pb_object(const nt::PhysicalMode*, pbnavitia::PhysicalMode*);
        void fill_pb_object(const nt::CommercialMode*, pbnavitia::CommercialMode*);
        void fill_pb_object(const nt::Line*, pbnavitia::Line*);
        void fill_pb_object(const nt::Route*, pbnavitia::Route*);
        void fill_pb_object(const nt::LineGroup*, pbnavitia::LineGroup*);
        void fill_pb_object(const nt::Calendar*, pbnavitia::Calendar*);
        void fill_pb_object(const nt::ValidityPattern*, pbnavitia::ValidityPattern*);
        void fill_pb_object(const nt::VehicleJourney*, pbnavitia::VehicleJourney*);
        void fill_pb_object(const nt::FrequencyVehicleJourney*, pbnavitia::VehicleJourney*);
        void fill_pb_object(const nt::MetaVehicleJourney*, pbnavitia::Trip*);
        void fill_pb_object(const ng::Admin*, pbnavitia::AdministrativeRegion*);
        void fill_pb_object(const nt::ExceptionDate*, pbnavitia::CalendarException*);
        void fill_pb_object(const nt::MultiLineString*, pbnavitia::MultiLineString*);
        void fill_pb_object(const nt::GeographicalCoord*, pbnavitia::Address*);
        void fill_pb_object(const nt::StopPointConnection*, pbnavitia::Connection*);
        void fill_pb_object(const nt::StopTime*, pbnavitia::StopTime*);
        void fill_pb_object(const nt::disruption::StopTimeUpdate*, pbnavitia::StopTime*);
        void fill_pb_object(const nt::StopTime*, pbnavitia::StopDateTime*);
        void fill_pb_object(const nt::StopTime*, pbnavitia::Properties*);
        void fill_pb_object(const std::string*, pbnavitia::Note*);
        void fill_pb_object(const jp_pair*, pbnavitia::JourneyPattern*);
        void fill_pb_object(const jpp_pair*, pbnavitia::JourneyPatternPoint*);
        void fill_pb_object(const navitia::vptranslator::BlockPattern*, pbnavitia::Calendar*);
        void fill_pb_object(const nt::Route*, pbnavitia::PtDisplayInfo*);
        void fill_pb_object(const nt::disruption::Impact*, pbnavitia::Impact*);

        void fill_pb_object(const ng::POI*, pbnavitia::Poi*);
        void fill_pb_object(const ng::POI*, pbnavitia::Address*);
        void fill_pb_object(const ng::POIType*, pbnavitia::PoiType*);

        void fill_pb_object(const VjOrigDest*, pbnavitia::hasEquipments*);
        void fill_pb_object(const VjStopTimes*, pbnavitia::PtDisplayInfo*);
        void fill_pb_object(const nt::VehicleJourney*, pbnavitia::hasEquipments*);
        void fill_pb_object(const StopTimeCalendar*, pbnavitia::ScheduleStopTime*);
        void fill_pb_object(const nt::EntryPoint*, pbnavitia::PtObject*);
        void fill_pb_object(const ng::Address*, pbnavitia::PtObject*);
        void fill_pb_object(const ng::Address*, pbnavitia::Address*);
        void fill_pb_object(const nt::Comment*, pbnavitia::Note*);
        void fill_pb_object(const nt::AccessPoint&, pbnavitia::AccessPoint*);

        void create_access_point(const nt::AccessPoint& access_point, pbnavitia::AccessPoint* ap);
        void fill_access_points(const std::set<nt::AccessPoint>& access_points, pbnavitia::StopPoint* stop_point);

        // Used for place
        template <typename T>
        void fill_pb_object(const T* value, pbnavitia::PtObject* pt_object);
    };
    // Raptor api
    pbnavitia::Section* create_section(pbnavitia::Journey*, const ng::PathItem&, int);
    const ng::POI* get_nearest_bss_station(const nt::GeographicalCoord&);
    const ng::POI* get_nearest_poi(const nt::GeographicalCoord&, const ng::POIType&);
    const ng::POI* get_nearest_parking(const nt::GeographicalCoord& coord);
    void finalize_section(pbnavitia::Section*, const ng::PathItem&, const ng::PathItem&, const pt::ptime, int);
    pbnavitia::GeographicalCoord get_coord(const pbnavitia::PtObject& pt_object);
};

template <typename N>
pbnavitia::Response get_response(const std::vector<N*>& nt_objects,
                                 const nt::Data& data,
                                 int depth = 0,
                                 const bool disable_geojson = false,
                                 const pt::ptime& now = pt::not_a_date_time,
                                 const pt::time_period& action_period = null_time_period,
                                 const DumpMessage dump_message = DumpMessage::Yes) {
    PbCreator creator(&data, now, action_period, disable_geojson);

    creator.pb_fill(nt_objects, depth, {dump_message, DumpLineSectionMessage::No});
    return creator.get_response();
}

void fill_pb_error(const pbnavitia::Error::error_id id,
                   const std::string& message,
                   pbnavitia::Error* error,
                   int max_depth = 0,
                   const pt::ptime& now = pt::not_a_date_time,
                   const pt::time_period& action_period = null_time_period);

template <typename Target, typename Source>
std::vector<Target*> ptref_indexes(const Source* nav_obj, const nt::Data& data) {
    const nt::Type_e type_e = nt::get_type_e<Target>();
    type::Indexes indexes;
    std::string request;
    try {
        std::stringstream ss;
        ss << nt::static_data::get()->captionByType(nav_obj->type) << ".uri=\""
           << boost::replace_all_copy(nav_obj->uri, "\"", "\\\"") << "\"";
        request = ss.str();
        indexes = navitia::ptref::make_query(type_e, request, data);
    } catch (const navitia::ptref::parsing_error& parse_error) {
        LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("logger"),
                        "ptref_indexes, Unable to parse :" << parse_error.more << ", request: " << request);
    } catch (const navitia::ptref::ptref_error& pt_error) {
        LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("logger"),
                        "pb_converter::ptref_indexes, " << pt_error.more << ", request: " << request);
    }
    return data.get_data<Target>(indexes);
}
}  // namespace navitia
