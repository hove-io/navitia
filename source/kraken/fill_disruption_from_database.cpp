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
#include "fill_disruption_from_database.h"
#include "utils/exception.h"
#include "utils/logger.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/format.hpp>
#include <pqxx/pqxx>

#include <functional>
#include <memory>

namespace navitia {

void fill_disruption_from_database(const std::string& connection_string,
                                   const boost::gregorian::date_period& production_date,
                                   DisruptionDatabaseReader& reader,
                                   const std::vector<std::string>& contributors,
                                   int batch_size) {
    auto conn = std::make_unique<pqxx::connection>(connection_string);

    pqxx::work work(*conn, "loading disruptions");

    size_t offset = 0, items_per_request = batch_size;
    pqxx::result result;
    std::string contributors_array = boost::algorithm::join(contributors, ", ");
    LOG4CPLUS_INFO(log4cplus::Logger::getInstance("Logger"), "Reading disruptions from database");
    do {
        std::string request =
            (boost::format(
                 "SELECT "
                 // Disruptions field
                 "     d.id as disruption_id, d.reference as disruption_reference, d.note as disruption_note,"
                 "     d.status as disruption_status,"
                 "     extract(epoch from d.start_publication_date  AT TIME ZONE 'UTC') :: bigint as "
                 "disruption_start_publication_date,"
                 "     extract(epoch from d.end_publication_date  AT TIME ZONE 'UTC') :: bigint as "
                 "disruption_end_publication_date,"
                 "     extract(epoch from d.created_at  AT TIME ZONE 'UTC') :: bigint as disruption_created_at,"
                 "     extract(epoch from d.updated_at  AT TIME ZONE 'UTC') :: bigint as disruption_updated_at,"
                 "     co.contributor_code as contributor,"
                 // Cause fields
                 "     c.id as cause_id, c.wording as cause_wording,"
                 "     c.is_visible as cause_visible,"
                 "     extract(epoch from c.created_at  AT TIME ZONE 'UTC') :: bigint as cause_created_at,"
                 "     extract(epoch from c.updated_at  AT TIME ZONE 'UTC') :: bigint as cause_updated_at,"
                 // Category fields
                 "     cat.name as category_name, cat.id as category_id,"
                 "     extract(epoch from c.created_at  AT TIME ZONE 'UTC') :: bigint as category_created_at,"
                 "     extract(epoch from c.updated_at  AT TIME ZONE 'UTC') :: bigint as category_updated_at,"
                 // Tag fields
                 "     t.id as tag_id, t.name as tag_name, t.is_visible as tag_is_visible,"
                 "     extract(epoch from t.created_at  AT TIME ZONE 'UTC') :: bigint as tag_created_at,"
                 "     extract(epoch from t.updated_at  AT TIME ZONE 'UTC') :: bigint as tag_updated_at,"
                 // Impact fields
                 "     i.id as impact_id, i.status as impact_status, i.disruption_id as impact_disruption_id,"
                 "     extract(epoch from i.created_at  AT TIME ZONE 'UTC') :: bigint as impact_created_at,"
                 "     extract(epoch from i.updated_at  AT TIME ZONE 'UTC') :: bigint as impact_updated_at,"
                 // Application period fields
                 "     a.id as application_id,"
                 "     extract(epoch from a.start_date  AT TIME ZONE 'UTC') :: bigint as application_start_date,"
                 "     extract(epoch from a.end_date  AT TIME ZONE 'UTC') :: bigint as application_end_date,"
                 // Severity fields
                 "     s.id as severity_id, s.wording as severity_wording, s.color as severity_color,"
                 "     s.is_visible as severity_is_visible, s.priority as severity_priority,"
                 "     s.effect as severity_effect,"
                 "     extract(epoch from s.created_at  AT TIME ZONE 'UTC') :: bigint as severity_created_at,"
                 "     extract(epoch from s.updated_at  AT TIME ZONE 'UTC') :: bigint as severity_updated_at,"
                 // Ptobject fields
                 "     p.id as ptobject_id, p.type as ptobject_type, p.uri as ptobject_uri,"
                 "     extract(epoch from p.created_at  AT TIME ZONE 'UTC') :: bigint as ptobject_created_at,"
                 "     extract(epoch from p.updated_at  AT TIME ZONE 'UTC') :: bigint as ptobject_updated_at,"
                 // Ptobject line_section optional fields
                 "     ls_line.uri as ls_line_uri,"
                 "     extract(epoch from ls_line.created_at  AT TIME ZONE 'UTC') :: bigint as ls_line_created_at,"
                 "     extract(epoch from ls_line.updated_at  AT TIME ZONE 'UTC') :: bigint as ls_line_updated_at,"
                 "     ls_start.uri as ls_start_uri,"
                 "     extract(epoch from ls_start.created_at  AT TIME ZONE 'UTC') :: bigint as ls_start_created_at,"
                 "     extract(epoch from ls_start.updated_at  AT TIME ZONE 'UTC') :: bigint as ls_start_updated_at,"
                 "     ls_end.uri as ls_end_uri,"
                 "     extract(epoch from ls_end.created_at  AT TIME ZONE 'UTC') :: bigint as ls_end_created_at,"
                 "     extract(epoch from ls_end.updated_at  AT TIME ZONE 'UTC') :: bigint as ls_end_updated_at,"
                 "     ls_route.id AS ls_route_id,"
                 "     ls_route.uri AS ls_route_uri,"
                 "     extract(epoch from ls_route.created_at  AT TIME ZONE 'UTC') :: bigint as ls_route_created_at,"
                 "     extract(epoch from ls_route.updated_at  AT TIME ZONE 'UTC') :: bigint as ls_route_updated_at,"
                 // Ptobject rail_section optional fields
                 "     rs_line.id as rs_line_id,"
                 "     rs_line.uri as rs_line_uri,"
                 "     extract(epoch from rs_line.created_at  AT TIME ZONE 'UTC') :: bigint as rs_line_created_at,"
                 "     extract(epoch from rs_line.updated_at  AT TIME ZONE 'UTC') :: bigint as rs_line_updated_at,"
                 "     rs_start.uri as rs_start_uri,"
                 "     extract(epoch from rs_start.created_at  AT TIME ZONE 'UTC') :: bigint as rs_start_created_at,"
                 "     extract(epoch from rs_start.updated_at  AT TIME ZONE 'UTC') :: bigint as rs_start_updated_at,"
                 "     rs_end.uri as rs_end_uri,"
                 "     extract(epoch from rs_end.created_at  AT TIME ZONE 'UTC') :: bigint as rs_end_created_at,"
                 "     extract(epoch from rs_end.updated_at  AT TIME ZONE 'UTC') :: bigint as rs_end_updated_at,"
                 "     rs_route.id AS rs_route_id,"
                 "     rs_route.uri AS rs_route_uri,"
                 "     extract(epoch from rs_route.created_at  AT TIME ZONE 'UTC') :: bigint as rs_route_created_at,"
                 "     extract(epoch from rs_route.updated_at  AT TIME ZONE 'UTC') :: bigint as rs_route_updated_at,"
                 "     rail_section.blocked_stop_areas as rs_blocked_sa,"
                 // Message fields
                 "     m.id as message_id, m.text as message_text,"
                 "     extract(epoch from m.created_at  AT TIME ZONE 'UTC') :: bigint as message_created_at,"
                 "     extract(epoch from m.updated_at  AT TIME ZONE 'UTC') :: bigint as message_updated_at,"
                 // Channel fields
                 "     ch.id as channel_id, ch.name as channel_name,"
                 "     ch.content_type as channel_content_type, ch.max_size as channel_max_size,"
                 "     extract(epoch from ch.created_at  AT TIME ZONE 'UTC') :: bigint as channel_created_at,"
                 "     extract(epoch from ch.updated_at  AT TIME ZONE 'UTC') :: bigint as channel_updated_at,"
                 "     cht.id as channel_type_id, cht.name as channel_type,"
                 // Property & Associate property fields
                 "     adp.value as property_value, pr.key as property_key, pr.type as property_type,"
                 // Pattern and time slots
                 "  pt.start_date as pattern_start_date,"
                 "  pt.end_date as pattern_end_date,"
                 "  pt.weekly_pattern as pattern_weekly_pattern,"
                 "  pt.id as pattern_id,"
                 "  extract(epoch from ts.begin ) ::int as time_slot_begin,"
                 "  extract(epoch from ts.end ) ::int as time_slot_end,"
                 "  ts.id as time_slot_id "
                 // Request
                 "     FROM disruption AS d"
                 "     JOIN contributor AS co ON d.contributor_id = co.id"
                 "     JOIN cause AS c ON (c.id = d.cause_id)"
                 "     LEFT JOIN category AS cat ON cat.id=c.category_id"
                 "     LEFT JOIN associate_disruption_tag ON associate_disruption_tag.disruption_id = d.id"
                 "     LEFT JOIN tag AS t ON associate_disruption_tag.tag_id = t.id"
                 "     JOIN impact AS i ON i.disruption_id = d.id"
                 "     JOIN application_periods AS a ON a.impact_id = i.id"
                 "     JOIN severity AS s ON s.id = i.severity_id"
                 "     JOIN associate_impact_pt_object ON associate_impact_pt_object.impact_id = i.id"
                 "     JOIN pt_object AS p ON associate_impact_pt_object.pt_object_id = p.id"
                 "     LEFT JOIN line_section ON p.id = line_section.object_id"
                 "     LEFT JOIN pt_object AS ls_line ON line_section.line_object_id = ls_line.id"
                 "     LEFT JOIN pt_object AS ls_start ON line_section.start_object_id = ls_start.id"
                 "     LEFT JOIN pt_object AS ls_end ON line_section.end_object_id = ls_end.id"
                 "     LEFT JOIN associate_line_section_route_object"
                 "         ON associate_line_section_route_object.line_section_id = line_section.id"
                 "     LEFT JOIN pt_object AS ls_route"
                 "         ON associate_line_section_route_object.route_object_id = ls_route.id"
                 "     LEFT JOIN rail_section ON p.id = rail_section.object_id"
                 "     LEFT JOIN pt_object AS rs_line ON rail_section.line_object_id = rs_line.id"
                 "     LEFT JOIN pt_object AS rs_start ON rail_section.start_object_id = rs_start.id"
                 "     LEFT JOIN pt_object AS rs_end ON rail_section.end_object_id = rs_end.id"
                 "     LEFT JOIN associate_rail_section_route_object"
                 "         ON associate_rail_section_route_object.rail_section_id = rail_section.id"
                 "     LEFT JOIN pt_object AS rs_route"
                 "         ON associate_rail_section_route_object.route_object_id = rs_route.id"
                 "     LEFT JOIN message AS m ON m.impact_id = i.id"
                 "     LEFT JOIN channel AS ch ON m.channel_id = ch.id"
                 "     LEFT JOIN channel_type as cht on ch.id = cht.channel_id"
                 "     LEFT JOIN associate_disruption_property adp ON adp.disruption_id = d.id"
                 "     LEFT JOIN property pr ON pr.id = adp.property_id"
                 "     LEFT JOIN pattern AS pt ON pt.impact_id = i.id"
                 "     LEFT JOIN time_slot AS ts ON ts.pattern_id = pt.id"
                 "     WHERE "
                 "     (NOT (d.start_publication_date >= '%s' OR d.end_publication_date <= '%s')"
                 "     OR (d.start_publication_date<='%s' and d.end_publication_date IS NULL))"
                 "     AND co.contributor_code = ANY('{%s}')"  // it's like a "IN" but won't crash if empty"
                 "     AND d.status = 'published'"
                 "     AND i.status = 'published'"
                 // Warning : Any change in this order may produce error while charging in
                 // fill_disruption_from_database.h/DisruptionDatabaseReader"
                 "     ORDER BY d.id, c.id, t.id, i.id, m.id, ch.id, cht.id"
                 "     LIMIT %i OFFSET %i"
                 " ;")
             % production_date.end() % production_date.begin() % production_date.end() % contributors_array
             % items_per_request % offset)
                .str();
        result = work.exec(request);
        for (auto res : result) {
            reader(res);
        }

        offset += result.size();
        LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("sql"), request);
    } while (!result.empty());

    // counting disruptions & impacts in order to get real numbers
    pqxx::result count;
    int impact_count = 0;
    std::string request =
        (boost::format("SELECT DISTINCT ON (d.id) d.id, count(i)"
                       "   FROM disruption d"
                       "   LEFT JOIN impact i ON d.id = i.disruption_id"
                       "   JOIN contributor AS co ON d.contributor_id = co.id"
                       "   WHERE"
                       "     (NOT (d.start_publication_date >= '%s' OR d.end_publication_date <= '%s')"
                       "     OR (d.start_publication_date<='%s' and d.end_publication_date IS NULL))"
                       "     AND co.contributor_code = ANY('{%s}')"  // it's like a "IN" but won't crash if empty"
                       "     AND d.status = 'published'"
                       "     AND i.status = 'published'"
                       "   GROUP BY d.id"
                       " ;")
         % production_date.end() % production_date.begin() % production_date.end() % contributors_array)
            .str();
    count = work.exec(request);
    for (auto ct : count) {
        impact_count += ct[1].as<int>();
    }

    LOG4CPLUS_INFO(log4cplus::Logger::getInstance("Logger"),
                   "Loading " << count.size() << " disruption(s) with " << impact_count << " impact(s)");
    reader.finalize();
    LOG4CPLUS_INFO(log4cplus::Logger::getInstance("Logger"), count.size() << " disruption(s) loaded");
}

void DisruptionDatabaseReader::finalize() {
    if (disruption && !disruption->id().empty()) {
        disruption_callback(*disruption, pt_data, meta);
    }
    pt_data.clean_weak_impacts();
}

}  // namespace navitia
