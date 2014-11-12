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
#include <functional>

#include "fill_disruption_from_database.h"
#include <pqxx/pqxx>
#include "utils/exception.h"
#include "utils/logger.h"

#include <boost/format.hpp>


namespace navitia {



void fill_disruption_from_database(const std::string& connection_string, 
        navitia::type::PT_Data& pt_data) {
    std::unique_ptr<pqxx::connection> conn;
    try{
        conn = std::unique_ptr<pqxx::connection>(new pqxx::connection(connection_string));
    }catch(const pqxx::pqxx_exception& e){
        throw navitia::exception(e.base().what());
    }
    pqxx::work work(*conn, "loading disruptions");

    // Get the size of disruptions table
    std::string request_count = "SELECT COUNT(*) as count FROM disruption;";

    pqxx::result result_count = work.exec(request_count);
    auto result_it = result_count.begin();
    if (result_it == result_count.end()) {
        LOG4CPLUS_ERROR(log4cplus::Logger::getInstance("log"),
                "There is no disruption table in the database");
        return;
    }
    size_t total_count = result_it["count"].as<size_t>(),
           offset = 0,
           items_per_request = 100;
    if (total_count == 0) {
        return;
    }
    DisruptionDatabaseReader reader(pt_data);
    while(offset < total_count) {
        std::string request = (boost::format(
               "SELECT "
               // Disruptions field
               "     d.id as disruption_id, d.reference as disruption_reference, d.note as disruption_note,"
               "     d.status as disruption_status,"
               "     extract(epoch from d.start_publication_date) :: bigint as disruption_start_publication_date,"
               "     extract(epoch from d.end_publication_date) :: bigint as disruption_end_publication_date,"
               "     extract(epoch from d.created_at) :: bigint as disruption_created_at,"
               "     extract(epoch from d.updated_at) :: bigint as disruption_updated_at,"
               // Cause fields
               "     c.id as cause_id, c.wording as cause_wording,"
               "     c.is_visible as cause_visible,"
               "     extract(epoch from c.created_at) :: bigint as cause_created_at,"
               "     extract(epoch from c.updated_at) :: bigint as cause_updated_at,"
               // Tag fields
               "     t.id as tag_id, t.name as tag_name, t.is_visible as tag_is_visible,"
               "     extract(epoch from t.created_at) :: bigint as tag_created_at,"
               "     extract(epoch from t.updated_at) :: bigint as tag_updated_at,"
               // Impact fields
               "     i.id as impact_id, i.status as impact_status, i.disruption_id as impact_disruption_id,"
               "     extract(epoch from i.created_at) :: bigint as impact_created_at,"
               "     extract(epoch from i.updated_at) :: bigint as impact_updated_at,"
               // Application period fields
               "     a.id as application_id,"
               "     extract(epoch from a.start_date) :: bigint as application_start_date,"
               "     extract(epoch from a.end_date) :: bigint as application_end_date,"
               // Severity fields
               "     s.id as severity_id, s.wording as severity_wording, s.color as severity_color,"
               "     s.is_visible as severity_is_visible, s.priority as severity_priority,"
               "     s.effect as severity_effect,"
               "     extract(epoch from s.created_at) :: bigint as severity_created_at,"
               "     extract(epoch from s.updated_at) :: bigint as severity_updated_at,"
               // Ptobject fields
               "     p.id as ptobject_id, p.type as ptobject_type, p.uri as ptobject_uri,"
               "     extract(epoch from p.created_at) :: bigint as ptobject_created_at,"
               "     extract(epoch from p.updated_at) :: bigint as ptobject_updated_at,"
               // Message fields
               "     m.id as message_id, m.text as message_text,"
               "     extract(epoch from m.created_at) :: bigint as message_created_at,"
               "     extract(epoch from m.updated_at) :: bigint as message_updated_at,"
               // Channel fields
               "     ch.id as channel_id, ch.name as channel_name,"
               "     ch.content_type as channel_content_type, ch.max_size as channel_max_size,"
               "     extract(epoch from ch.created_at) :: bigint as channel_created_at,"
               "     extract(epoch from ch.updated_at) :: bigint as channel_updated_at"
               "     FROM disruption AS d"
               "     JOIN cause AS c ON (c.id = d.cause_id)"
               "     JOIN associate_disruption_tag ON associate_disruption_tag.disruption_id = d.id"
               "     JOIN tag AS t ON associate_disruption_tag.tag_id = t.id"
               "     JOIN impact AS i ON i.disruption_id = d.id"
               "     JOIN application_periods AS a ON a.impact_id = i.id"
               "     JOIN severity AS s ON s.id = i.severity_id"
               "     JOIN associate_impact_pt_object ON associate_impact_pt_object.impact_id = i.id"
               "     JOIN pt_object AS p ON associate_impact_pt_object.pt_object_id = p.id"
               "     JOIN message AS m ON m.impact_id = i.id"
               "     JOIN channel AS ch ON m.channel_id = ch.id"
               "     ORDER BY d.id, c.id, t.id, i.id, a.id, p.id, m.id, ch.id"
               "     LIMIT %i OFFSET %i"
               " ;") % items_per_request % offset).str();

        pqxx::result result = work.exec(request);
        std::for_each(result.begin(), result.end(), reader);

        offset += items_per_request;
    }
    reader.finalize();
}

void DisruptionDatabaseReader::finalize() {
    if (disruption->id() != "") {
        add_disruption(pt_data, *disruption);
    }
}

}
