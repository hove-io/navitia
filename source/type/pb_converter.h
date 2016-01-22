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
#include "data.h"
#include "type/type.pb.h"
#include "type/response.pb.h"
#include "type/pt_data.h"
#include "vptranslator/vptranslator.h"
#include "ptreferential/ptreferential.h"

namespace pt = boost::posix_time;
namespace nt = navitia::type;
namespace ng = navitia::georef;

// we don't use a boolean to have a more type checks
enum class DumpMessage {
    Yes,
    No
};

static const auto null_time_period = pt::time_period(pt::not_a_date_time, pt::seconds(0));

//forward declare
namespace navitia {
    namespace routing {
        struct PathItem;
        struct JourneyPattern;
        struct JourneyPatternPoint;
        using JppIdx = Idx<JourneyPatternPoint>;
        using JpIdx = Idx<JourneyPattern>;
    }
    namespace georef {
        struct PathItem;
        struct Way;
        struct POI;
        struct Path;
        struct POIType;
    }
    namespace fare {
        struct results;
        struct Ticket;
    }
    namespace timetables {
        struct Thermometer;
    }
}

using jp_pair = std::pair<const navitia::routing::JpIdx, const navitia::routing::JourneyPattern&>;
using jpp_pair = std::pair<const navitia::routing::JppIdx, const navitia::routing::JourneyPatternPoint&>;

namespace navitia {

struct VjOrigDest{
    const nt::VehicleJourney* vj;
    const nt::StopPoint* orig;
    const nt::StopPoint* dest;
    VjOrigDest(const nt::VehicleJourney*vj, nt::StopPoint* orig, nt::StopPoint* dest):vj(vj),
        orig(orig), dest(dest) {}
};

struct VjStopTimes{
    const nt::VehicleJourney* vj;
    const nt::StopTime* orig;
    const nt::StopTime* dest;
    VjStopTimes(const nt::VehicleJourney* vj, const nt::StopTime* orig, const nt::StopTime* dest): vj(vj),
        orig(orig), dest(dest){}
};

struct StopTimeCalandar{
    const nt::StopTime* stop_time;
    const navitia::DateTime& date_time;
    boost::optional<const std::string> calendar_id;
    StopTimeCalandar(const nt::StopTime* stop_time, const navitia::DateTime& date_time,
                     boost::optional<const std::string> calendar_id):
        stop_time(stop_time), date_time(date_time), calendar_id(calendar_id){}
};

struct WayCoord{
    const ng::Way* way;
    const nt::GeographicalCoord& coord;
    const int number;
    WayCoord(const ng::Way* way, const nt::GeographicalCoord& coord,
             const int number):
        way(way), coord(coord), number(number){}

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
template<typename T>
auto get_label_if_exists(const T* v, int) -> decltype(v->label) {
    return v->label;
}

template<typename T>
auto get_label_if_exists(const T* v, int) -> decltype(v->get_label()) {
    return v->get_label();
}

template<typename T>
auto get_label_if_exists(const T* v, long) -> decltype(v->name) {
    return v->name;
}
}

template<typename T>
std::string get_label(const T* v) {
    return detail::get_label_if_exists(v, 0);
}
inline pbnavitia::NavitiaType get_embedded_type(const nt::Calendar*) { return pbnavitia::CALENDAR; }
inline pbnavitia::NavitiaType get_embedded_type(const nt::VehicleJourney*) { return pbnavitia::VEHICLE_JOURNEY; }
inline pbnavitia::NavitiaType get_embedded_type(const nt::Line*) { return pbnavitia::LINE; }
inline pbnavitia::NavitiaType get_embedded_type(const nt::Route*) { return pbnavitia::ROUTE; }
inline pbnavitia::NavitiaType get_embedded_type(const nt::Company*) { return pbnavitia::COMPANY; }
inline pbnavitia::NavitiaType get_embedded_type(const nt::Network*) { return pbnavitia::NETWORK; }
inline pbnavitia::NavitiaType get_embedded_type(const ng::POI*) { return pbnavitia::POI; }
inline pbnavitia::NavitiaType get_embedded_type(const ng::Admin*) { return pbnavitia::ADMINISTRATIVE_REGION; }
inline pbnavitia::NavitiaType get_embedded_type(const nt::StopArea*) { return pbnavitia::STOP_AREA; }
inline pbnavitia::NavitiaType get_embedded_type(const nt::StopPoint*) { return pbnavitia::STOP_POINT; }
inline pbnavitia::NavitiaType get_embedded_type(const nt::CommercialMode*) { return pbnavitia::COMMERCIAL_MODE; }
inline pbnavitia::NavitiaType get_embedded_type(const nt::MetaVehicleJourney*) { return pbnavitia::TRIP; }

inline pbnavitia::Calendar* get_sub_object(const nt::Calendar*,
                                           pbnavitia::PtObject* pt_object) { return pt_object->mutable_calendar(); }
inline pbnavitia::VehicleJourney* get_sub_object(const nt::VehicleJourney*,
                                                 pbnavitia::PtObject* pt_object) { return pt_object->mutable_vehicle_journey(); }
inline pbnavitia::Line* get_sub_object(const nt::Line*,
                                       pbnavitia::PtObject* pt_object) { return pt_object->mutable_line(); }
inline pbnavitia::Route* get_sub_object(const nt::Route*,
                                        pbnavitia::PtObject* pt_object) { return pt_object->mutable_route(); }
inline pbnavitia::Company* get_sub_object(const nt::Company*,
                                          pbnavitia::PtObject* pt_object) { return pt_object->mutable_company(); }
inline pbnavitia::Network* get_sub_object(const nt::Network*,
                                          pbnavitia::PtObject* pt_object) { return pt_object->mutable_network(); }
inline pbnavitia::Poi* get_sub_object(const ng::POI*,
                                      pbnavitia::PtObject* pt_object) { return pt_object->mutable_poi(); }
inline pbnavitia::AdministrativeRegion* get_sub_object(const ng::Admin*,
                                                       pbnavitia::PtObject* pt_object) { return pt_object->mutable_administrative_region(); }
inline pbnavitia::StopArea* get_sub_object(const nt::StopArea*,
                                           pbnavitia::PtObject* pt_object) { return pt_object->mutable_stop_area(); }
inline pbnavitia::StopPoint* get_sub_object(const nt::StopPoint*,
                                            pbnavitia::PtObject* pt_object) { return pt_object->mutable_stop_point(); }
inline pbnavitia::CommercialMode* get_sub_object(const nt::CommercialMode*,
                                                 pbnavitia::PtObject* pt_object) { return pt_object->mutable_commercial_mode(); }
inline pbnavitia::Trip* get_sub_object(const nt::MetaVehicleJourney*,
                                       pbnavitia::PtObject* pt_object) { return pt_object->mutable_trip(); }

namespace {
template<typename T> nt::Type_e get_type_e() {
    static_assert(!std::is_same<T, T>::value, "get_type_e unimplemented");
    return nt::Type_e::Unknown;
}
template<> nt::Type_e get_type_e<nt::PhysicalMode>() {
    return nt::Type_e::PhysicalMode;
}
template<> nt::Type_e get_type_e<nt::CommercialMode>() {
    return nt::Type_e::CommercialMode;
}

template<typename T> const std::vector<T*>& get_data_vector(const nt::Data& data) {
    static_assert(!std::is_same<T, T>::value, "get_data_vector unimplemented");
    return {};
}
template<> const std::vector<nt::PhysicalMode*>& get_data_vector<nt::PhysicalMode>(const nt::Data& data) {
    return data.pt_data->physical_modes;
}
template<> const std::vector<nt::CommercialMode*>& get_data_vector<nt::CommercialMode>(const nt::Data& data) {
    return data.pt_data->commercial_modes;
}
}

struct PbCreator {
    const nt::Data& data;
    const pt::ptime now;
    const pt::time_period action_period;
    const bool show_codes;
    PbCreator(const nt::Data& data, const pt::ptime  now, const pt::time_period action_period,
              const bool show_codes):
        data(data), now(now), action_period(action_period),show_codes(show_codes) {}
    template<typename N, typename P>
    void fill(int depth, const DumpMessage dump_message, const N& item, P* proto) {
        Filler(depth, dump_message, *this).fill_pb_object(item, proto);
    }

private:
    struct Filler {
        struct PtObjVisitor;
        const int depth;
        DumpMessage dump_message;

        PbCreator& pb_creator;
        Filler(int depth, const DumpMessage dump_message, PbCreator & pb_creator):
            depth(depth), dump_message(dump_message), pb_creator(pb_creator){};

        Filler copy(int, DumpMessage);
        template<typename NAV, typename PB>        
        void fill(const NAV& nav_object, PB* pb_object) {
            copy(depth-1, dump_message).fill_pb_object(nav_object, pb_object);
        }

        template<typename NAV, typename PB>
        void fill(const NAV& nav_object, PB* pb_object, int d) {
            copy(d, dump_message).fill_pb_object(nav_object, pb_object);
        }

        template<typename NAV, typename PB>
        void fill(const NAV& nav_object, PB* pb_object, int d, DumpMessage dm) {
            copy(d, dm).fill_pb_object(nav_object, pb_object);
        }

        template<typename Nav, typename Pb>
        void fill_pb_object(const std::vector<Nav*>& nav_list,
                            ::google::protobuf::RepeatedPtrField<Pb>* pb_list) {
            for (auto* nav_obj: nav_list) {
                fill_pb_object(nav_obj, pb_list->Add());
            }
        }

        template<typename Nav, typename Pb>
        void fill_pb_object(const std::vector<Nav>& nav_list,
                            ::google::protobuf::RepeatedPtrField<Pb>* pb_list) {
            for (auto& nav_obj: nav_list) {
                fill_pb_object(&nav_obj, pb_list->Add());
            }
        }

        template<typename Nav, typename Pb>
        void fill_pb_object(const std::set<Nav>& nav_list,
                            ::google::protobuf::RepeatedPtrField<Pb>* pb_list) {
            for (auto& nav_obj: nav_list) {
                fill_pb_object(&nav_obj, pb_list->Add());
            }
        }

        template <typename NAV, typename P>
        void fill_messages(const NAV* nav_obj, P* pb_obj){
            if (dump_message == DumpMessage::No) { return; }
            for (const auto& message : nav_obj->get_applicable_messages(pb_creator.now,
                                                                        pb_creator.action_period)){
                fill_message(*message, pb_obj);
            }
        }

        template <typename Target, typename Source>
        std::vector<Target*> ptref_indexes(const Source* nav_obj) {
            const nt::Type_e type_e = get_type_e<Target>();
            std::vector<nt::idx_t> indexes;
            try{
                std::string request = nt::static_data::get()->captionByType(nav_obj->type) +
                        ".uri=" + nav_obj->uri;
                indexes = navitia::ptref::make_query(type_e, request, pb_creator.data);
            } catch(const navitia::ptref::parsing_error &parse_error) {
                LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("logger"),
                               "ptref_indexes, Unable to parse :" + parse_error.more);
            } catch(const navitia::ptref::ptref_error &pt_error) {
                LOG4CPLUS_DEBUG(log4cplus::Logger::getInstance("logger"),
                               "ptref_indexes, " + pt_error.more);
            }
            std::vector<Target*> res;
            const std::vector<Target*>& data_vec = get_data_vector<Target>(pb_creator.data);
            for (const auto& idx: indexes) {
                res.push_back(data_vec.at(idx));
            }
            return res;
        }

        template<typename NT, typename PB>
        void fill_codes(const NT* nt, PB* pb) {
            if (nt == nullptr) {return ;}
            if(!pb_creator.show_codes){ return ;}
            for (const auto& code: pb_creator.data.pt_data->codes.get_codes(nt)) {
                for (const auto& value: code.second) {
                    auto* pb_code = pb->add_codes();
                    pb_code->set_type(code.first);
                    pb_code->set_value(value);
                }
            }
        }

        template<typename NT, typename PB>
        void fill_comments(const NT* nt, PB* pb, const std::string& type = "standard") {
            for (const auto& comment: pb_creator.data.pt_data->comments.get(nt)) {
                auto com = pb->add_comments();
                com->set_value(comment);
                com->set_type(type);
            }
        }

        template <typename P>
        void fill_message(const nt::disruption::Impact& impact, P pb_object);

        void fill_informed_entity(const nt::disruption::PtObj& ptobj,const nt::disruption::Impact& impact,
                                  pbnavitia::Impact* pb_impact);

        void fill_pb_object(const nt::Contributor*, pbnavitia::Contributor*);
        void fill_pb_object(const nt::Frame*, pbnavitia::Frame*);
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
        void fill_pb_object(const nt::MetaVehicleJourney*, pbnavitia::Trip*);
        void fill_pb_object(const ng::Admin*, pbnavitia::AdministrativeRegion*);
        void fill_pb_object(const nt::ExceptionDate*, pbnavitia::CalendarException*);
        void fill_pb_object(const nt::MultiLineString*, pbnavitia::MultiLineString*);
        void fill_pb_object(const nt::GeographicalCoord*, pbnavitia::Address*);
        void fill_pb_object(const nt::StopPointConnection*, pbnavitia::Connection*);
        void fill_pb_object(const nt::StopTime* , pbnavitia::StopTime*);
        void fill_pb_object(const nt::StopTime*, pbnavitia::StopDateTime*);
        void fill_pb_object(const nt::StopTime* , pbnavitia::Properties*);
        void fill_pb_object(const std::string* , pbnavitia::Note*);
        void fill_pb_object(const jp_pair*, pbnavitia::JourneyPattern*);
        void fill_pb_object(const jpp_pair*, pbnavitia::JourneyPatternPoint*);
        void fill_pb_object(const navitia::vptranslator::BlockPattern* ,pbnavitia::Calendar*);
        void fill_pb_object(const nt::Route*, pbnavitia::PtDisplayInfo*);

        void fill_pb_object(const ng::POI*, pbnavitia::Poi*);
        void fill_pb_object(const ng::POI*, pbnavitia::Address*);
        void fill_pb_object(const ng::POIType*, pbnavitia::PoiType*);
        // messages
        template <typename PBOBJECT>
        void fill_pb_object(const nt::disruption::Impact* impact, PBOBJECT* obj){
            fill_message(*impact, obj);
        }

        void fill_pb_object(const VjOrigDest*, pbnavitia::hasEquipments*);
        void fill_pb_object(const VjStopTimes*, pbnavitia::PtDisplayInfo*);
        void fill_pb_object(const nt::VehicleJourney*, pbnavitia::hasEquipments*);
        void fill_pb_object(const StopTimeCalandar*, pbnavitia::ScheduleStopTime*);
        void fill_pb_object(const nt::EntryPoint*, pbnavitia::PtObject*);
        void fill_pb_object(const WayCoord*, pbnavitia::PtObject*);
        void fill_pb_object(const WayCoord*, pbnavitia::Address*);

        // Used for place
        template<typename T>
        void fill_pb_object(const T* value, pbnavitia::PtObject* pt_object) {
            if(value == nullptr) { return; }

            fill(value, get_sub_object(value, pt_object), depth, dump_message);
            pt_object->set_name(get_label(value));
            pt_object->set_uri(value->uri);
            pt_object->set_embedded_type(get_embedded_type(value));
        }
    };
};

template<typename N, typename P>
void fill_pb_object(const N& item, const nt::Data& data, P* proto, int depth = 0,
        const pt::ptime& now = pt::not_a_date_time,
        const pt::time_period& action_period = null_time_period,
        const bool show_codes = false, const DumpMessage dump_message = DumpMessage::Yes) {
    PbCreator creator(data, now, action_period, show_codes);
    creator.fill(depth, dump_message, item, proto);
}


struct EnhancedResponse {
    pbnavitia::Response response;
    size_t nb_sections = 0;
    std::map<std::pair<pbnavitia::Journey*, size_t>, std::string> routing_section_map;
    pbnavitia::Ticket* unknown_ticket = nullptr; //we want only one unknown ticket

    const std::string& register_section(pbnavitia::Journey* j,
                                        const routing::PathItem& /*routing_item*/, size_t section_idx) {
        routing_section_map[{j, section_idx}] = "section_" + boost::lexical_cast<std::string>(nb_sections++);
        return routing_section_map[{j, section_idx}];
    }

    std::string register_section() {
        // For some section (transfer, waiting, streetnetwork, corwfly) we don't need info
        // about the item
        return "section_" + boost::lexical_cast<std::string>(nb_sections++);
    }

    std::string get_section_id(pbnavitia::Journey* j, size_t section_idx) {
        auto it  = routing_section_map.find({j, section_idx});
        if (it == routing_section_map.end()) {
            LOG4CPLUS_WARN(log4cplus::Logger::getInstance("logger"),
                           "Impossible to find section id for section idx " << section_idx);
            return "";
        }
        return it->second;
    }
};

void fill_co2_emission(pbnavitia::Section *pb_section, const nt::Data& data,
                       const type::VehicleJourney* vehicle_journey);
void fill_co2_emission_by_mode(pbnavitia::Section *pb_section, const nt::Data& data,
                               const std::string& mode_uri);

void fill_crowfly_section(const type::EntryPoint& origin, const type::EntryPoint& destination,
                          const time_duration& crow_fly_duration, type::Mode_e mode,
                          pt::ptime origin_time, const type::Data& data,
                          EnhancedResponse& response,  pbnavitia::Journey* pb_journey,
                          const pt::ptime& now, const pt::time_period& action_period);

void fill_fare_section(EnhancedResponse& pb_response, pbnavitia::Journey* pb_journey, const fare::results& fare);

void fill_street_sections(EnhancedResponse& response, const type::EntryPoint &ori_dest,
                          const georef::Path & path, const type::Data &data, pbnavitia::Journey* pb_journey,
        const pt::ptime departure, int max_depth = 1,
        const pt::ptime& now = pt::not_a_date_time,
        const pt::time_period& action_period = null_time_period);

void add_path_item(pbnavitia::StreetNetwork* sn, const ng::PathItem& item,
                   const type::EntryPoint &ori_dest,
                   const nt::Data& data);


void fill_additional_informations(google::protobuf::RepeatedField<int>* infos,
                                  const bool has_datetime_estimated,
                                  const bool has_odt,
                                  const bool is_zonal);

void fill_pb_error(const pbnavitia::Error::error_id id, const std::string& comment,
                    pbnavitia::Error* error, int max_depth = 0 ,
                    const pt::ptime& now = pt::not_a_date_time,
                    const pt::time_period& action_period  = null_time_period);

}
