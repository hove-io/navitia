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

#include "equipment_api.h"

#include "utils/paginate.h"

#include <boost/range/algorithm.hpp>

#include <algorithm>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace navitia {
namespace equipment {

namespace {
const std::string build_ptref_line_filter(const std::string& line_uri, const std::string& filter) {
    std::string sa_filter = "line.uri=" + line_uri;
    if (!filter.empty()) {
        sa_filter += " and (" + filter + ")";
    }
    return sa_filter;
}

void fill_equipment_to_pb(const equipment::EquipmentReportList& reports, PbCreator& pb_creator, int depth) {
    for (const auto& report : reports) {
        const type::Line* line = std::get<0>(report);
        const auto& stop_areas_reports = std::get<1>(report);
        auto pb_report = pb_creator.add_equipment_reports();
        pb_creator.fill(line, pb_report->mutable_line(), depth);
        for (const auto& sa_report : stop_areas_reports) {
            const type::StopArea* sa = std::get<0>(sa_report);
            const auto& stop_points = std::get<1>(sa_report);
            auto sa_equip = pb_report->add_stop_area_equipments();
            auto pb_sa = sa_equip->mutable_stop_area();
            pb_creator.fill(sa, pb_sa, depth);
            for (const type::StopPoint* sp : stop_points) {
                auto pb_sp = pb_sa->add_stop_points();
                pb_creator.fill(sp, pb_sp, depth);
            }
        }
    }
}
}  // namespace

EquipmentReports::EquipmentReports(const type::Data& data,
                                   std::string filter,
                                   int count,
                                   int start_page,
                                   ForbiddenUris forbidden_uris)
    : data(data),
      filter(std::move(filter)),
      count(count),
      start_page(start_page),
      forbidden_uris(std::move(forbidden_uris)) {}

EquipmentReportList EquipmentReports::get_paginated_equipment_report_list() {
    const type::Indexes line_indices = ptref::make_query(type::Type_e::Line, filter, forbidden_uris, data);
    const auto lines = data.get_data<type::Line>(line_indices);

    total_lines = lines.size();
    const auto paginated_lines = paginate(lines, count, start_page);

    EquipmentReportList res;
    for (const auto line : paginated_lines) {
        const std::string line_filter = build_ptref_line_filter(line->uri, filter);
        const type::Indexes sp_indexes = ptref::make_query(type::Type_e::StopPoint, line_filter, forbidden_uris, data);
        const auto stop_points = data.get_data<type::StopPoint>(sp_indexes);

        std::map<type::StopArea*, std::set<type::StopPoint*>> sa_map;
        for (type::StopPoint* sp : stop_points) {
            sa_map[sp->stop_area].emplace(sp);
        }

        std::vector<StopAreaEquipment> sa_equipments;
        std::transform(sa_map.cbegin(), sa_map.cend(), std::back_inserter(sa_equipments),
                       [](const auto& p) { return StopAreaEquipment(p.first, p.second); });

        res.emplace_back(line, sa_equipments);
    }

    return res;
}

void equipment_reports(PbCreator& pb_creator,
                       const std::string& filter,
                       int count,
                       int depth,
                       int start_page,
                       const ForbiddenUris& forbidden_uris) {
    const type::Data& data = *pb_creator.data;
    EquipmentReports equipment_reports(data, filter, count, start_page, forbidden_uris);

    try {
        EquipmentReportList reports = equipment_reports.get_paginated_equipment_report_list();

        fill_equipment_to_pb(reports, pb_creator, depth);

        size_t total_lines = equipment_reports.get_total_lines();
        pb_creator.make_paginate(reports.size(), start_page, count, total_lines);
    } catch (const ptref::parsing_error& parse_error) {
        pb_creator.fill_pb_error(pbnavitia::Error::unable_to_parse, "Unable to parse filter" + parse_error.more);
        return;
    } catch (const ptref::ptref_error& ptref_error) {
        pb_creator.fill_pb_error(pbnavitia::Error::bad_filter, "ptref : " + ptref_error.more);
        return;
    }
}

}  // namespace equipment
}  // namespace navitia
