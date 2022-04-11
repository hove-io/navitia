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

#include "position_api.h"
#include "utils/paginate.h"

namespace navitia {
namespace position {

void vehicle_positions(PbCreator& pb_creator,
                       const std::string& filter,
                       size_t count,
                       int depth,
                       size_t start_page,
                       const std::vector<std::string>& forbidden_uris) {
    const type::Data& data = *pb_creator.data;

    type::Indexes idx_vjs;
    try {
        idx_vjs = ptref::make_query(type::Type_e::VehicleJourney, filter, forbidden_uris, type::OdtLevel_e::all,
                                    boost::none, boost::none, type::RTLevel::RealTime, data, pb_creator.now);
    } catch (const ptref::parsing_error& parse_error) {
        pb_creator.fill_pb_error(pbnavitia::Error::unable_to_parse, "Unable to parse filter" + parse_error.more);
        return;
    } catch (const ptref::ptref_error& ptref_error) {
        pb_creator.fill_pb_error(pbnavitia::Error::bad_filter, "ptref : " + ptref_error.more);
        return;
    }

    const auto& vjs = data.get_data<type::VehicleJourney>(idx_vjs);
    VehiclePositionList result;

    for (const auto& vj : vjs) {
        result[vj->route->line].emplace(vj);
    }

    size_t total_result = result.size();
    const auto paginated_result = paginate(result, count, start_page);

    for (const auto& res : paginated_result) {
        auto* pb_vehicle_positions = pb_creator.add_vehicle_positions();
        auto* pb_line = pb_vehicle_positions->mutable_line();
        pb_creator.fill(res.first, pb_line, depth - 1, DumpMessage::No);
        for (const auto& v : res.second) {
            auto* pb_vj_positions = pb_vehicle_positions->add_vehicle_journey_positions();
            auto* pb_vehicle_journey = pb_vj_positions->mutable_vehicle_journey();
            pb_creator.fill(v, pb_vehicle_journey, depth - 1, DumpMessage::No);
        }
    }
    pb_creator.make_paginate(total_result, start_page, count, pb_creator.vehicle_positions_size());
}

}  // namespace position
}  // namespace navitia
