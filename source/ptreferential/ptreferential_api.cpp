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

#include "ptreferential_api.h"

#include "ptreferential.h"
#include "routing/dataraptor.h"
#include "type/data.h"
#include "type/meta_data.h"
#include "type/pb_converter.h"
#include "type/pt_data.h"
#include "utils/paginate.h"

using navitia::type::Type_e;

namespace navitia {
namespace ptref {
/// build the protobuf response of a pt ref query

static void extract_data(navitia::PbCreator& pb_creator,
                         const type::Data& data,
                         const type::Type_e requested_type,
                         const type::Indexes& rows,
                         const int depth) {
    pb_creator.action_period = data.meta->production_period();
    auto with_line_sections = DumpMessageOptions{DumpMessage::Yes, DumpLineSectionMessage::Yes};
    with_line_sections.dump_rail_section = DumpRailSectionMessage::Yes;
    switch (requested_type) {
        case Type_e::ValidityPattern:
            pb_creator.pb_fill(data.get_data<nt::ValidityPattern>(rows), depth);
            return;
        case Type_e::Line:
            pb_creator.pb_fill(data.get_data<nt::Line>(rows), depth, with_line_sections);
            return;
        case Type_e::LineGroup:
            pb_creator.pb_fill(data.get_data<nt::LineGroup>(rows), depth);
            return;
        case Type_e::JourneyPattern: {
            for (const auto& idx : rows) {
                const auto& pair_jp = data.dataRaptor->jp_container.get_jps()[idx];
                auto* pb_jp = pb_creator.add_journey_patterns();
                pb_creator.fill(&pair_jp, pb_jp, depth);
            }
            return;
        }
        case Type_e::StopPoint:
            pb_creator.pb_fill(data.get_data<nt::StopPoint>(rows), depth, with_line_sections);
            return;
        case Type_e::StopArea:
            pb_creator.pb_fill(data.get_data<nt::StopArea>(rows), depth);
            return;
        case Type_e::Network:
            pb_creator.pb_fill(data.get_data<nt::Network>(rows), depth);
            return;
        case Type_e::PhysicalMode:
            pb_creator.pb_fill(data.get_data<nt::PhysicalMode>(rows), depth);
            return;
        case Type_e::CommercialMode:
            pb_creator.pb_fill(data.get_data<nt::CommercialMode>(rows), depth);
            return;
        case Type_e::JourneyPatternPoint: {
            for (const auto& idx : rows) {
                const auto& pair_jpp = data.dataRaptor->jp_container.get_jpps()[idx];
                auto* pb_jpp = pb_creator.add_journey_pattern_points();
                pb_creator.fill(&pair_jpp, pb_jpp, depth);
            }
            return;
        }
        case Type_e::Company:
            pb_creator.pb_fill(data.get_data<nt::Company>(rows), depth);
            return;
        case Type_e::Route:
            pb_creator.pb_fill(data.get_data<nt::Route>(rows), depth);
            return;
        case Type_e::POI:
            pb_creator.pb_fill(data.get_data<georef::POI>(rows), depth);
            return;
        case Type_e::POIType:
            pb_creator.pb_fill(data.get_data<georef::POIType>(rows), depth);
            return;
        case Type_e::Connection:
            pb_creator.pb_fill(data.get_data<nt::StopPointConnection>(rows), depth);
            return;
        case Type_e::VehicleJourney:
            pb_creator.pb_fill(data.get_data<nt::VehicleJourney>(rows), depth, with_line_sections);
            return;
        case Type_e::Calendar:
            pb_creator.pb_fill(data.get_data<nt::Calendar>(rows), depth);
            return;
        case Type_e::MetaVehicleJourney: {
            for (const auto& idx : rows) {
                const auto* meta_vj = data.pt_data->meta_vjs[Idx<type::MetaVehicleJourney>(idx)];
                auto* pb_trip = pb_creator.add_trips();
                pb_creator.fill(meta_vj, pb_trip, depth);
            }
            return;
        }
        case Type_e::Impact: {
            for (const auto& idx : rows) {
                auto impact = data.pt_data->disruption_holder.get_weak_impacts()[idx].lock();
                auto* pb_impact = pb_creator.add_impacts();
                pb_creator.fill(impact.get(), pb_impact, depth);
            }
            return;
        }
        case Type_e::Contributor:
            pb_creator.pb_fill(data.get_data<nt::Contributor>(rows), depth);
            return;
        case Type_e::Dataset:
            pb_creator.pb_fill(data.get_data<nt::Dataset>(rows), depth);
            return;
        default:
            return;
    }
}

void query_pb(navitia::PbCreator& pb_creator,
              const type::Type_e requested_type,
              const std::string& request,
              const std::vector<std::string>& forbidden_uris,
              const type::OdtLevel_e odt_level,
              const int depth,
              const int startPage,
              const int count,
              const boost::optional<boost::posix_time::ptime>& since,
              const boost::optional<boost::posix_time::ptime>& until,
              const type::RTLevel rt_level,
              const type::Data& data) {
    type::Indexes final_indexes;
    int total_result;
    try {
        final_indexes = make_query(requested_type, request, forbidden_uris, odt_level, since, until, rt_level, data);
    } catch (const parsing_error& parse_error) {
        pb_creator.fill_pb_error(pbnavitia::Error::unable_to_parse, "Unable to parse :" + parse_error.more);
        return;
    } catch (const ptref_error& pt_error) {
        pb_creator.fill_pb_error(pbnavitia::Error::bad_filter, "ptref : " + pt_error.more);
        return;
    }
    total_result = final_indexes.size();
    final_indexes = paginate(final_indexes, count, startPage);

    extract_data(pb_creator, data, requested_type, final_indexes, depth);
    auto pagination = pb_creator.mutable_pagination();
    pagination->set_totalresult(total_result);
    pagination->set_startpage(startPage);
    pagination->set_itemsperpage(count);
    pagination->set_itemsonpage(final_indexes.size());
}

/**
 * From a line, a start stoppoint and a destination get all the routes of the line that would match
 * start -> destination
 *
 * this function is used when we got a destination from an external system with data different than ours
 * and we need to find the routes (hopefully only one) that match that destination
 */
std::vector<const type::Route*> get_matching_routes(const type::Data* data,
                                                    const type::Line* line,
                                                    const type::StopPoint* start,
                                                    const std::pair<std::string, std::string>& destination_code) {
    const auto possible_stop_points =
        data->pt_data->codes.get_objs<nt::StopPoint>(destination_code.first, destination_code.second);

    const auto possible_stop_areas =
        data->pt_data->codes.get_objs<nt::StopArea>(destination_code.first, destination_code.second);

    if (possible_stop_points.empty() && possible_stop_areas.empty()) {
        LOG4CPLUS_WARN(log4cplus::Logger::getInstance("logger"), "no stop matching the code "
                                                                     << destination_code.second
                                                                     << " (key = " << destination_code.first << ")"
                                                                     << " impossible to find matching routes");
        return {};
    }
    std::set<const nt::Route*> routes;
    const auto& data_raptor = data->dataRaptor;
    for (const auto& jpp : data_raptor->jpps_from_sp[routing::SpIdx(start->idx)]) {
        const auto& jp = data_raptor->jp_container.get(jpp.jp_idx);
        const auto* r = data->get_data<nt::Route>()[jp.route_idx.val];
        if (r->line != line) {
            continue;
        }
        for (const auto& next_jpp_idx : boost::make_iterator_range(jp.jpps_begin() + jpp.order, jp.jpps_end())) {
            const auto next_jpp = data_raptor->jp_container.get(next_jpp_idx);
            const auto sp = data->get_data<nt::StopPoint>()[next_jpp.sp_idx.val];
            if (contains(possible_stop_points, sp) || contains(possible_stop_areas, sp->stop_area)) {
                routes.insert(r);
                break;
            }
        }
    }

    return std::vector<const type::Route*>(routes.begin(), routes.end());
}

void fill_matching_routes(navitia::PbCreator& pb_creator,
                          const type::Data* data,
                          const type::Line* line,
                          const type::StopPoint* start,
                          const std::pair<std::string, std::string>& destination_code) {
    const auto routes = get_matching_routes(data, line, start, destination_code);
    // we don't need any detail, we just add routes with a 0 depth
    pb_creator.pb_fill(routes, 0);
}
}  // namespace ptref
}  // namespace navitia
