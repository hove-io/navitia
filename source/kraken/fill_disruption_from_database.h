
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
#include "type/pt_data.h"
#include "pqxx/result.hxx"
#include "type/chaos.pb.h"
#include "fill_disruption_from_chaos.h"


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
            navitia::type::PT_Data& pt_data);

    struct DisruptionDatabaseReader {
        DisruptionDatabaseReader(type::PT_Data& pt_data) : pt_data(pt_data) {}

        chaos::Disruption* disruption = nullptr;
        chaos::Impact* impact = nullptr;
        chaos::Tag* tag = nullptr;

        std::string last_message_id = "",
                    last_ptobject_id = "",
                    last_period_id = "";
        navitia::type::PT_Data& pt_data;

        template<typename T>
        void operator() (T const_it) {
            if (!disruption || disruption->id() !=
                    const_it["disruption_id"].template as<std::string>()) {
                if (disruption) {
                    add_disruption(pt_data, *disruption);
                    disruption->Clear();
                }
                fill_disruption(const_it);
                fill_cause(const_it);
                tag = nullptr;
                impact = nullptr;
            }

            if (disruption && (!tag ||
                    tag->id() != const_it["tag_id"].template as<std::string>())) {
                fill_tag(const_it);
            }

            if (disruption && (!impact ||
                    impact->id() != const_it["impact_id"].template as<std::string>())) {
                fill_impact(const_it);
                last_period_id = "";
                last_ptobject_id = "";
                last_message_id = "";
            }

            if (impact && (last_period_id != const_it["application_id"].template as<std::string>())) {
                fill_application_period(const_it);
            }
            if (impact && (last_ptobject_id != const_it["ptobject_id"].template as<std::string>())) {
                fill_pt_object(const_it);
            }
            if (impact && (last_message_id != const_it["message_id"].template as<std::string>())) {
                fill_message(const_it);
            }
        }

        void finalize();

        template<typename T>
        void fill_disruption(T const_it) {
            disruption = new chaos::Disruption();
            FILL_TIMESTAMPMIXIN(disruption)
            auto period = disruption->mutable_publication_period();
            auto start_date = const_it["disruption_start_publication_date"];
            if (!start_date.is_null()) {
                period->set_start(start_date.template as<uint64_t>());
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
                if (effect == "blocking") {
                    severity->set_effect(transit_realtime::Alert::Effect::Alert_Effect_NO_SERVICE);
                }
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
        void fill_pt_object(T const_it) {
            auto ptobject = impact->add_informed_entities();
            FILL_REQUIRED(ptobject, updated_at, uint64_t)
            FILL_NULLABLE(ptobject, created_at, uint64_t)
            FILL_REQUIRED(ptobject, uri, std::string)
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
                } else if (type_ == "line_section") {
                    ptobject->set_pt_object_type(chaos::PtObject_Type_line_section);
                } else {
                    ptobject->set_pt_object_type(chaos::PtObject_Type_unkown_type);
                }
            }
            last_ptobject_id = const_it["ptobject_id"].template as<std::string>();
        }

        template<typename T>
        void fill_message(T const_it) {
            auto message = impact->add_messages();
            FILL_REQUIRED(message, text, std::string)
            FILL_NULLABLE(message, created_at, uint64_t)
            FILL_NULLABLE(message, updated_at, uint64_t)
            fill_channel(const_it, message->mutable_channel());
            last_message_id = const_it["message_id"].template as<std::string>();
        }

        template<typename T>
        void fill_channel(T const_it, chaos::Channel* channel) {
            FILL_TIMESTAMPMIXIN(channel)
            FILL_REQUIRED(channel, name, std::string)
            FILL_NULLABLE(channel, content_type, std::string)
            FILL_NULLABLE(channel, max_size, uint32_t)
        }

    };
}
