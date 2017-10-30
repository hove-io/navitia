
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
#include <string>
#include <memory>
#include "type/pt_data.h"
#include "pqxx/result.hxx"
#include "type/chaos.pb.h"
#include "make_disruption_from_chaos.h"


namespace navitia {
#define FILL_NULLABLE_(var_name, arg_name, col_name, type_name)\
   if (!const_it[#col_name].is_null()) \
        var_name->set_##arg_name(const_it[#col_name].template as<type_name>());

#define FILL_NULLABLE(table_name, arg_name, type_name)\
            FILL_NULLABLE_(table_name, arg_name, table_name##_##arg_name, type_name)

#define FILL_REQUIRED(table_name, arg_name, type_name)\
        table_name->set_##arg_name(const_it[#table_name"_"#arg_name].template as<type_name>());

#define FILL_TIMESTAMPMIXIN(table_name)\
        FILL_REQUIRED(table_name, id, std::string)\
        FILL_REQUIRED(table_name, created_at, uint64_t)\
        FILL_NULLABLE(table_name, updated_at, uint64_t)

    void fill_disruption_from_database(const std::string& connection_string,
            navitia::type::PT_Data& pt_data, navitia::type::MetaData &meta, const std::vector<std::string>& contributors);

    struct DisruptionDatabaseReader {
        DisruptionDatabaseReader(type::PT_Data& pt_data, const type::MetaData& meta) : pt_data(pt_data), meta(meta) {}

        std::unique_ptr<chaos::Disruption> disruption = nullptr;
        chaos::Impact* impact = nullptr;
        chaos::Tag* tag = nullptr;
        chaos::Message* message = nullptr;
        chaos::Channel* channel = nullptr;
        chaos::PtObject* pt_object = nullptr;

        std::string last_period_id = "",
                    last_channel_type_id = "";

        std::set<std::string> message_ids;
        std::set<std::string> pt_object_ids;
        std::set<std::string> associate_objects_ids;
        type::PT_Data& pt_data;
        const type::MetaData& meta;

        // This function and all others below are templated so they can be tested
        template<typename T>
        void operator() (T const_it) {
            if (!disruption || disruption->id() !=
                    const_it["disruption_id"].template as<std::string>()) {
                fill_disruption(const_it);
                fill_cause(const_it);
                tag = nullptr;
                impact = nullptr;
            }

            if (disruption && !const_it["tag_id"].is_null() && (!tag ||
                    tag->id() != const_it["tag_id"].template as<std::string>())) {
                fill_tag(const_it);
            }

            if (disruption && (!impact ||
                    impact->id() != const_it["impact_id"].template as<std::string>())) {
                fill_impact(const_it);
                last_period_id = "";
                last_channel_type_id = "";
                message = nullptr;
                channel = nullptr;
                pt_object = nullptr;
                pt_object_ids.clear();
                message_ids.clear();
            }

            if (impact && (last_period_id != const_it["application_id"].template as<std::string>())) {
                fill_application_period(const_it);
            }
            if (impact && (!pt_object_ids.count(const_it["ptobject_id"].template as<std::string>()))) {
                pt_object = impact->add_informed_entities();
                fill_pt_object(const_it, pt_object);
                associate_objects_ids.clear();
                pt_object_ids.insert(const_it["ptobject_id"].template as<std::string>());
            }
            if (impact && !const_it["ls_route_id"].is_null() &&
                    (!associate_objects_ids.count(const_it["ls_route_id"].template as<std::string>()))) {
                fill_associate_route(const_it, pt_object);
                associate_objects_ids.insert(const_it["ls_route_id"].template as<std::string>());
            }
            if (impact && !const_it["message_id"].is_null() &&
                    (!message_ids.count(const_it["message_id"].template as<std::string>()))) {
                message = impact->add_messages();
                fill_message(const_it, message);
                channel = message->mutable_channel();
                fill_channel(const_it, channel);
                message_ids.insert(const_it["message_id"].template as<std::string>());
            }
            if (impact && channel &&
                    (last_channel_type_id != const_it["channel_type_id"].template as<std::string>())) {
                fill_channel_type(const_it, channel);
                last_channel_type_id = const_it["channel_type_id"].template as<std::string>();
            }
        }

        void finalize();

        template<typename T>
        void fill_disruption(T const_it) {
            if (disruption) {
                make_and_apply_disruption(*disruption, pt_data, meta);
            }
            disruption = std::make_unique<chaos::Disruption>();
            FILL_TIMESTAMPMIXIN(disruption)
            auto period = disruption->mutable_publication_period();
            auto start_date = const_it["disruption_start_publication_date"];
            if (!start_date.is_null()) {
                period->set_start(start_date.template as<uint64_t>());
            }

            auto contributor = const_it["contributor"];
            if (disruption && !contributor.is_null()){
                disruption->set_contributor(contributor.template as<std::string>());
            }

            auto end_date = const_it["disruption_end_publication_date"];
            if (!end_date.is_null()) {
                period->set_end(end_date.template as<uint64_t>());
            }
            FILL_NULLABLE(disruption, note, std::string)
            FILL_NULLABLE(disruption, reference, std::string)
        }

        template<typename T>
        void fill_cause(T const_it) {
            auto cause = disruption->mutable_cause();
            FILL_TIMESTAMPMIXIN(cause)
            FILL_REQUIRED(cause, wording, std::string)
            fill_category(const_it);
        }

        template<typename T>
        void fill_category(T const_it) {
            if (const_it["category_id"].is_null()) {
                return;
            }
            auto category = disruption->mutable_cause()->mutable_category();
            FILL_TIMESTAMPMIXIN(category)
            FILL_REQUIRED(category, name, std::string)
        }

        template<typename T>
        void fill_tag(T const_it) {
            tag = disruption->add_tags();
            FILL_TIMESTAMPMIXIN(tag)
            FILL_NULLABLE(tag, name, std::string)
        }


        template<typename T>
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

        template<typename T>
        void fill_application_period(T const_it) {
            auto period = impact->add_application_periods();
            if (!const_it["application_start_date"].is_null()) {
                period->set_start(const_it["application_start_date"].template as<uint64_t>());
            }
            if (!const_it["application_end_date"].is_null()) {
                period->set_end(const_it["application_end_date"].template as<uint64_t>());
            }
            last_period_id = const_it["application_id"].template as<std::string>();
        }

        template<typename T>
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
                } else {
                    ptobject->set_pt_object_type(chaos::PtObject_Type_unkown_type);
                }
            }
        }

        template<typename T>
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
                }
            }
        }

        template<typename T>
        void fill_message(T const_it, chaos::Message* message) {
            FILL_REQUIRED(message, text, std::string)
            FILL_NULLABLE(message, created_at, uint64_t)
            FILL_NULLABLE(message, updated_at, uint64_t)
        }

        template<typename T>
        void fill_channel(T const_it, chaos::Channel* channel) {
            FILL_TIMESTAMPMIXIN(channel)
            FILL_REQUIRED(channel, id, std::string)
            FILL_REQUIRED(channel, name, std::string)
            FILL_NULLABLE(channel, content_type, std::string)
            FILL_NULLABLE(channel, max_size, uint32_t)
        }

        template<typename T>
        void fill_channel_type(T const_it, chaos::Channel* channel) {
            const auto& type_ = const_it["channel_type"].template as<std::string>();
            //Here we have to verify existance of Channel Type in the channel.
            if (type_ == "web"){
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

    };
}
