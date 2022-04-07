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

#include "type/pb_converter.h"

#include <string>
#include <vector>
#include <set>
#include <tuple>

namespace navitia {
namespace equipment {

using StopAreaEquipment = std::tuple<type::StopArea*, std::set<type::StopPoint*>>;
using EquipmentReport = std::tuple<type::Line*, std::vector<StopAreaEquipment>>;
using EquipmentReportList = std::vector<EquipmentReport>;
using ForbiddenUris = std::vector<std::string>;

class EquipmentReports {
public:
    EquipmentReports(const type::Data& data,
                     std::string filter,
                     int count = 25,
                     int start_page = 0,
                     ForbiddenUris forbidden_uris = {});

    EquipmentReportList get_paginated_equipment_report_list();
    size_t get_total_lines() const { return total_lines; }

private:
    const type::Data& data;
    const std::string filter;
    const int count;
    const int start_page;
    const ForbiddenUris forbidden_uris;
    size_t total_lines = 0;
};

void equipment_reports(PbCreator& pb_creator,
                       const std::string& filter,
                       int count,
                       int depth = 0,
                       int start_page = 0,
                       const ForbiddenUris& forbidden_uris = {});

}  // namespace equipment
}  // namespace navitia
