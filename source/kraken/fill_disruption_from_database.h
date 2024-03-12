
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
#include "type/pt_data.h"
#include "type/chaos.pb.h"
#include "make_disruption_from_chaos.h"

#include <pqxx/result.hxx>
#include <boost/property_tree/json_parser.hpp>

#include <string>
#include <memory>
#include <utility>
#include <map>

namespace navitia {
#define FILL_NULLABLE_(var_name, arg_name, col_name, type_name) \
    if (!const_it[#col_name].is_null())                         \
        var_name->set_##arg_name(const_it[#col_name].template as<type_name>());

#define FILL_NULLABLE(table_name, arg_name, type_name) \
    FILL_NULLABLE_(table_name, arg_name, table_name##_##arg_name, type_name)

#define FILL_REQUIRED(table_name, arg_name, type_name) \
    table_name->set_##arg_name(const_it[#table_name "_" #arg_name].template as<type_name>());

#define FILL_TIMESTAMPMIXIN(table_name)             \
    FILL_REQUIRED(table_name, id, std::string)      \
    FILL_REQUIRED(table_name, created_at, uint64_t) \
    FILL_NULLABLE(table_name, updated_at, uint64_t)

using ChaosDisruptionApplier = std::function<
    void(const chaos::Disruption& chaos_disruption, navitia::type::PT_Data& pt_data, const navitia::type::MetaData&)>;

struct DisruptionDatabaseReader {
    DisruptionDatabaseReader(type::PT_Data& pt_data,
                             const type::MetaData& meta,
                             ChaosDisruptionApplier disruption_callback = make_and_apply_disruption)
        : pt_data(pt_data), meta(meta), disruption_callback(std::move(disruption_callback)) {}

    std::unique_ptr<chaos::Disruption> disruption = nullptr;
    chaos::Impact* impact = nullptr;
    chaos::Tag* tag = nullptr;
    chaos::Message* message = nullptr;
    chaos::Channel* channel = nullptr;
    chaos::PtObject* pt_object = nullptr;
    chaos::DisruptionProperty* property = nullptr;
    chaos::Pattern* pattern = nullptr;

    std::string last_channel_type_id = "";

    std::map<std::string, chaos::PtObject*> map_ptobject;

    std::set<std::string> message_ids;
    // message_id + translation_language
    std::set<std::string> message_translate_ids;
    std::set<std::tuple<std::string, std::string, std::string>> properties;
    std::set<std::string> application_periods_ids;
    std::set<std::string> pattern_ids;
    std::set<std::string> time_slot_ids;
    std::set<std::tuple<std::string, std::string>> line_section_route_set;
    std::set<std::tuple<std::string, std::string>> rail_section_route_set;
    type::PT_Data& pt_data;
    const type::MetaData& meta;
    ChaosDisruptionApplier disruption_callback;

    // This function and all others below are templated so they can be tested
    template <typename T>
    void operator()(T const_it) {
        // This code is strongly related to the database query (and it's order): fill_disruption_from_database.cpp
        if (!disruption || disruption->id() != const_it["disruption_id"].template as<std::string>()) {
            fill_disruption(const_it);
            fill_cause(const_it);
            tag = nullptr;
            impact = nullptr;
            properties.clear();
        }

        if (disruption && !const_it["tag_id"].is_null()
            && (!tag || tag->id() != const_it["tag_id"].template as<std::string>())) {
            fill_tag(const_it);
        }

        if (disruption && (!impact || impact->id() != const_it["impact_id"].template as<std::string>())) {
            fill_impact(const_it);
            last_channel_type_id = "";
            message = nullptr;
            channel = nullptr;
            pt_object = nullptr;
            message_ids.clear();
            message_translate_ids.clear();
            application_periods_ids.clear();
            line_section_route_set.clear();
            rail_section_route_set.clear();
            pattern = nullptr;
            pattern_ids.clear();
            time_slot_ids.clear();
            map_ptobject.clear();
        }

        if (disruption && !const_it["property_key"].is_null() && !const_it["property_type"].is_null()
            && !const_it["property_value"].is_null()) {
            std::tuple<std::string, std::string, std::string> property(
                const_it["property_key"].template as<std::string>(),
                const_it["property_type"].template as<std::string>(),
                const_it["property_value"].template as<std::string>());

            if (!properties.count(property)) {
                fill_property(const_it);
                properties.insert(property);
            }
        }

        if (impact && !const_it["application_id"].is_null()
            && (!application_periods_ids.count(const_it["application_id"].template as<std::string>()))) {
            fill_application_period(const_it);
            application_periods_ids.insert(const_it["application_id"].template as<std::string>());
        }

        if (impact && !const_it["pattern_id"].is_null()
            && (!pattern_ids.count(const_it["pattern_id"].template as<std::string>()))) {
            fill_application_pattern(const_it);
            pattern_ids.insert(const_it["pattern_id"].template as<std::string>());
        }

        if (pattern && !const_it["time_slot_id"].is_null()
            && (!time_slot_ids.count(const_it["time_slot_id"].template as<std::string>()))) {
            fill_time_slot(const_it);
            time_slot_ids.insert(const_it["time_slot_id"].template as<std::string>());
        }

        // The ptobject is directly associated with impact for all the types except line_section and rail_section
        // Normally the ptobject is associated only once to the impact, use of ptobject_uri should work
        // For line_section and rail section there are more pt_object as children associated to the parent pt_object
        // and should be managed.
        // Use of ptobject_uri to distinguish parent ptobject doesn't work. ptobject_id should be used instead.
        // Final solution: use of pt_object_id should work for all types of pt_object
        if (impact && !const_it["ptobject_id"].is_null()) {
            auto pt_obj_it = map_ptobject.find(const_it["ptobject_id"].template as<std::string>());
            if (pt_obj_it != map_ptobject.end()) {
                pt_object = pt_obj_it->second;
            } else {
                pt_object = impact->add_informed_entities();
                fill_pt_object(const_it, pt_object);
                map_ptobject[const_it["ptobject_id"].template as<std::string>()] = pt_object;
            }
        }

        // We should use ptobject_id instead of ptobject_uri to attach children routes if present
        if (impact && !const_it["ls_route_uri"].is_null()) {
            std::tuple<std::string, std::string> line_section_route(
                const_it["ptobject_id"].template as<std::string>(),
                const_it["ls_route_uri"].template as<std::string>());
            if (!line_section_route_set.count(line_section_route)) {
                fill_associate_route(const_it, pt_object);
                line_section_route_set.insert(line_section_route);
            }
        }

        // We should use ptobject_id instead of ptobject_uri to attach children routes if present
        if (impact && !const_it["rs_route_uri"].is_null()) {
            std::tuple<std::string, std::string> rail_section_route(
                const_it["ptobject_id"].template as<std::string>(),
                const_it["rs_route_uri"].template as<std::string>());
            if (!rail_section_route_set.count(rail_section_route)) {
                fill_associate_route(const_it, pt_object);
                rail_section_route_set.insert(rail_section_route);
            }
        }

        if (impact && !const_it["message_id"].is_null()
            && (!message_ids.count(const_it["message_id"].template as<std::string>()))) {
            message = impact->add_messages();
            fill_message(const_it, message);
            channel = message->mutable_channel();
            fill_channel(const_it, channel);
            message_ids.insert(const_it["message_id"].template as<std::string>());
        }

        if (impact && channel && (channel->id() == const_it["channel_id"].template as<std::string>())
            && (last_channel_type_id != const_it["channel_type_id"].template as<std::string>())) {
            fill_channel_type(const_it, channel);
            last_channel_type_id = const_it["channel_type_id"].template as<std::string>();
        }
        // Fill tranlations related to this message
        if (impact && message && !const_it["translation_language"].is_null()) {
            std::string message_translate_id = const_it["message_id"].template as<std::string>()
                                               + const_it["translation_language"].template as<std::string>();
            if (!message_translate_ids.count(message_translate_id)) {
                auto* translation = message->add_translations();
                fill_translation(const_it, translation);
                message_translate_ids.insert(message_translate_id);
            }
        }
    }

    void finalize();

    template <typename T>
    void fill_disruption(T const_it) {
        if (disruption) {
            disruption_callback(*disruption, pt_data, meta);
        }
        disruption = std::make_unique<chaos::Disruption>();
        FILL_TIMESTAMPMIXIN(disruption)
        auto period = disruption->mutable_publication_period();
        auto start_date = const_it["disruption_start_publication_date"];
        if (!start_date.is_null()) {
            period->set_start(start_date.template as<uint64_t>());
        }

        auto contributor = const_it["contributor"];
        if (disruption && !contributor.is_null()) {
            disruption->set_contributor(contributor.template as<std::string>());
        }

        auto end_date = const_it["disruption_end_publication_date"];
        if (!end_date.is_null()) {
            period->set_end(end_date.template as<uint64_t>());
        }
        FILL_NULLABLE(disruption, note, std::string)
        FILL_NULLABLE(disruption, reference, std::string)
    }

    template <typename T>
    void fill_cause(T const_it) {
        auto cause = disruption->mutable_cause();
        FILL_TIMESTAMPMIXIN(cause)
        FILL_REQUIRED(cause, wording, std::string)
        fill_category(const_it);
    }

    template <typename T>
    void fill_category(T const_it) {
        if (const_it["category_id"].is_null()) {
            return;
        }
        auto category = disruption->mutable_cause()->mutable_category();
        FILL_TIMESTAMPMIXIN(category)
        FILL_REQUIRED(category, name, std::string)
    }

    template <typename T>
    void fill_tag(T const_it) {
        tag = disruption->add_tags();
        FILL_TIMESTAMPMIXIN(tag)
        FILL_NULLABLE(tag, name, std::string)
    }

    template <typename T>
    void fill_impact(T const_it) {
        impact = disruption->add_impacts();
        FILL_TIMESTAMPMIXIN(impact)
        auto severity = impact->mutable_severity();
        FILL_TIMESTAMPMIXIN(severity)
        FILL_REQUIRED(severity, wording, std::string)
        FILL_NULLABLE(severity, color, std::string)
        FILL_NULLABLE(severity, priority, uint32_t)
        if (!const_it["severity_effect"].is_null()) {
            const auto& effect = const_it["severity_effect"].template as<std::string>();
            if (effect == "blocking" || effect == "no_service") {
                severity->set_effect(transit_realtime::Alert::Effect::Alert_Effect_NO_SERVICE);
            } else if (effect == "reduced_service") {
                severity->set_effect(transit_realtime::Alert::Effect::Alert_Effect_REDUCED_SERVICE);
            } else if (effect == "significant_delays") {
                severity->set_effect(transit_realtime::Alert::Effect::Alert_Effect_SIGNIFICANT_DELAYS);
            } else if (effect == "detour") {
                severity->set_effect(transit_realtime::Alert::Effect::Alert_Effect_DETOUR);
            } else if (effect == "additional_service") {
                severity->set_effect(transit_realtime::Alert::Effect::Alert_Effect_ADDITIONAL_SERVICE);
            } else if (effect == "modified_service") {
                severity->set_effect(transit_realtime::Alert::Effect::Alert_Effect_MODIFIED_SERVICE);
            } else if (effect == "other_effect") {
                severity->set_effect(transit_realtime::Alert::Effect::Alert_Effect_OTHER_EFFECT);
            } else if (effect == "unknown_effect") {
                severity->set_effect(transit_realtime::Alert::Effect::Alert_Effect_UNKNOWN_EFFECT);
            } else if (effect == "stop_moved") {
                severity->set_effect(transit_realtime::Alert::Effect::Alert_Effect_STOP_MOVED);
            } else {
                severity->set_effect(transit_realtime::Alert::Effect::Alert_Effect_UNKNOWN_EFFECT);
            }
        } else {
            severity->set_effect(transit_realtime::Alert::Effect::Alert_Effect_UNKNOWN_EFFECT);
        }
    }

    template <typename T>
    void fill_application_period(T const_it) {
        auto period = impact->add_application_periods();
        if (!const_it["application_start_date"].is_null()) {
            period->set_start(const_it["application_start_date"].template as<uint64_t>());
        }
        if (!const_it["application_end_date"].is_null()) {
            period->set_end(const_it["application_end_date"].template as<uint64_t>());
        }
    }

    template <typename T>
    void fill_pt_object(T const_it, chaos::PtObject* ptobject) {
        FILL_NULLABLE(ptobject, updated_at, uint64_t)
        FILL_REQUIRED(ptobject, created_at, uint64_t)
        FILL_NULLABLE(ptobject, uri, std::string)
        if (!const_it["ptobject_type"].is_null()) {
            const auto& type_ = const_it["ptobject_type"].template as<std::string>();
            if (type_ == "line") {
                ptobject->set_pt_object_type(chaos::PtObject_Type_line);
            } else if (type_ == "network") {
                ptobject->set_pt_object_type(chaos::PtObject_Type_network);
            } else if (type_ == "route") {
                ptobject->set_pt_object_type(chaos::PtObject_Type_route);
            } else if (type_ == "stop_area") {
                ptobject->set_pt_object_type(chaos::PtObject_Type_stop_area);
            } else if (type_ == "stop_point") {
                ptobject->set_pt_object_type(chaos::PtObject_Type_stop_point);
            } else if (type_ == "line_section") {
                ptobject->set_pt_object_type(chaos::PtObject_Type_line_section);
                auto* ls = ptobject->mutable_pt_line_section();
                auto* ls_line = ls->mutable_line();
                ls_line->set_pt_object_type(chaos::PtObject_Type_line);
                FILL_NULLABLE(ls_line, updated_at, uint64_t)
                FILL_REQUIRED(ls_line, created_at, uint64_t)
                FILL_NULLABLE(ls_line, uri, std::string)
                auto* ls_start = ls->mutable_start_point();
                ls_start->set_pt_object_type(chaos::PtObject_Type_stop_area);
                FILL_NULLABLE(ls_start, updated_at, uint64_t)
                FILL_REQUIRED(ls_start, created_at, uint64_t)
                FILL_NULLABLE(ls_start, uri, std::string)
                auto* ls_end = ls->mutable_end_point();
                ls_end->set_pt_object_type(chaos::PtObject_Type_stop_area);
                FILL_NULLABLE(ls_end, updated_at, uint64_t)
                FILL_REQUIRED(ls_end, created_at, uint64_t)
                FILL_NULLABLE(ls_end, uri, std::string)
            } else if (type_ == "rail_section") {
                ptobject->set_pt_object_type(chaos::PtObject_Type_rail_section);
                auto* rs = ptobject->mutable_pt_rail_section();
                fill_blocked_stop_area(const_it, rs);
                if (!const_it["rs_line_id"].is_null()) {
                    auto* rs_line = rs->mutable_line();
                    rs_line->set_pt_object_type(chaos::PtObject_Type_line);
                    FILL_NULLABLE(rs_line, updated_at, uint64_t)
                    FILL_REQUIRED(rs_line, created_at, uint64_t)
                    FILL_NULLABLE(rs_line, uri, std::string)
                }
                auto* rs_start = rs->mutable_start_point();
                rs_start->set_pt_object_type(chaos::PtObject_Type_stop_area);
                FILL_NULLABLE(rs_start, updated_at, uint64_t)
                FILL_REQUIRED(rs_start, created_at, uint64_t)
                FILL_NULLABLE(rs_start, uri, std::string)
                auto* rs_end = rs->mutable_end_point();
                rs_end->set_pt_object_type(chaos::PtObject_Type_stop_area);
                FILL_NULLABLE(rs_end, updated_at, uint64_t)
                FILL_REQUIRED(rs_end, created_at, uint64_t)
                FILL_NULLABLE(rs_end, uri, std::string)
            } else {
                ptobject->set_pt_object_type(chaos::PtObject_Type_unkown_type);
            }
        }
    }

    template <typename T>
    void fill_associate_route(T const_it, chaos::PtObject* ptobject) {
        if (!const_it["ptobject_type"].is_null()) {
            const auto& type_ = const_it["ptobject_type"].template as<std::string>();
            if (type_ == "line_section") {
                if (!const_it["ls_route_id"].is_null()) {
                    auto* ls_route = ptobject->mutable_pt_line_section()->add_routes();
                    ls_route->set_pt_object_type(chaos::PtObject_Type_route);
                    FILL_NULLABLE(ls_route, updated_at, uint64_t)
                    FILL_REQUIRED(ls_route, created_at, uint64_t)
                    FILL_NULLABLE(ls_route, uri, std::string)
                }
            } else if (type_ == "rail_section") {
                if (!const_it["rs_route_id"].is_null()) {
                    auto* rs_route = ptobject->mutable_pt_rail_section()->add_routes();
                    rs_route->set_pt_object_type(chaos::PtObject_Type_route);
                    FILL_NULLABLE(rs_route, updated_at, uint64_t)
                    FILL_REQUIRED(rs_route, created_at, uint64_t)
                    FILL_NULLABLE(rs_route, uri, std::string)
                }
            }
        }
    }

    template <typename T>
    void fill_blocked_stop_area(T const_it, chaos::RailSection* rail_section) {
        if (!const_it["rs_blocked_sa"].is_null()) {
            auto json_blocked_sa = const_it["rs_blocked_sa"].template as<std::string>();
            std::stringstream ss;
            ss << json_blocked_sa;
            boost::property_tree::ptree pt;
            boost::property_tree::read_json(ss, pt);
            for (const auto& it : pt) {
                auto* ordered_pt = rail_section->add_blocked_stop_areas();
                ordered_pt->set_uri(it.second.get_child("id").get_value<std::string>());
                ordered_pt->set_order(it.second.get_child("order").get_value<uint32_t>());
            }
        }
    }

    template <typename T>
    void fill_message(T const_it, chaos::Message* message) {
        FILL_REQUIRED(message, text, std::string)
        FILL_NULLABLE(message, created_at, uint64_t)
        FILL_NULLABLE(message, updated_at, uint64_t)
    }

    template <typename T>
    void fill_translation(T const_it, chaos::Translation* translation) {
        FILL_REQUIRED(translation, text, std::string)
        FILL_NULLABLE(translation, language, std::string)
        FILL_NULLABLE(translation, url_audio, std::string)
    }

    template <typename T>
    void fill_channel(T const_it, chaos::Channel* channel) {
        FILL_TIMESTAMPMIXIN(channel)
        FILL_REQUIRED(channel, id, std::string)
        FILL_REQUIRED(channel, name, std::string)
        FILL_NULLABLE(channel, content_type, std::string)
        FILL_NULLABLE(channel, max_size, uint32_t)
    }

    template <typename T>
    void fill_channel_type(T const_it, chaos::Channel* channel) {
        const auto& type_ = const_it["channel_type"].template as<std::string>();
        // Here we have to verify existance of Channel Type in the channel.
        if (type_ == "web") {
            channel->add_types(chaos::Channel_Type_web);
        } else if (type_ == "sms") {
            channel->add_types(chaos::Channel_Type_sms);
        } else if (type_ == "email") {
            channel->add_types(chaos::Channel_Type_email);
        } else if (type_ == "mobile") {
            channel->add_types(chaos::Channel_Type_mobile);
        } else if (type_ == "notification") {
            channel->add_types(chaos::Channel_Type_notification);
        } else if (type_ == "twitter") {
            channel->add_types(chaos::Channel_Type_twitter);
        } else if (type_ == "facebook") {
            channel->add_types(chaos::Channel_Type_facebook);
        } else if (type_ == "title") {
            channel->add_types(chaos::Channel_Type_title);
        } else if (type_ == "beacon") {
            channel->add_types(chaos::Channel_Type_beacon);
        } else {
            channel->add_types(chaos::Channel_Type_unkown_type);
        }
    }

    template <typename T>
    void fill_property(T const_it) {
        property = disruption->add_properties();
        FILL_REQUIRED(property, key, std::string)
        FILL_REQUIRED(property, type, std::string)
        FILL_REQUIRED(property, value, std::string)
    }

    template <typename T>
    void fill_application_pattern(T const_it) {
        pattern = impact->add_application_patterns();
        auto start_date = navitia::to_int_date(
            boost::gregorian::from_string(const_it["pattern_start_date"].template as<std::string>()));
        auto end_date = navitia::to_int_date(
            boost::gregorian::from_string(const_it["pattern_end_date"].template as<std::string>()));
        pattern->set_start_date(start_date);
        pattern->set_end_date(end_date);
        auto week_str = const_it["pattern_weekly_pattern"].template as<std::string>();
        std::reverse(week_str.begin(), week_str.end());
        const auto& week = std::bitset<7>{week_str};
        auto* week_pattern = pattern->mutable_week_pattern();
        week_pattern->set_monday(week[navitia::Monday]);
        week_pattern->set_tuesday(week[navitia::Tuesday]);
        week_pattern->set_wednesday(week[navitia::Wednesday]);
        week_pattern->set_thursday(week[navitia::Thursday]);
        week_pattern->set_friday(week[navitia::Friday]);
        week_pattern->set_saturday(week[navitia::Saturday]);
        week_pattern->set_sunday(week[navitia::Sunday]);
    }

    template <typename T>
    void fill_time_slot(T const_it) {
        chaos::TimeSlot* time_slot = pattern->add_time_slots();
        FILL_REQUIRED(time_slot, begin, int32_t)
        FILL_REQUIRED(time_slot, end, int32_t)
    }
};

void fill_disruption_from_database(const std::string& connection_string,
                                   const boost::gregorian::date_period& production_date,
                                   DisruptionDatabaseReader& reader,
                                   const std::vector<std::string>& contributors,
                                   int batch_size);

}  // namespace navitia
