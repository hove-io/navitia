/* Copyright © 2001-2019, Canal TP and/or its affiliates. All rights reserved.

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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "access_point_api.h"
#include "type/access_point.h"

#include "utils/paginate.h"

#include <boost/range/algorithm.hpp>

namespace navitia {
namespace access_point {

namespace {

void fill_access_point_to_pb(const access_point::AccessPointList& ap_list, PbCreator& pb_creator, int depth) {
    for (const auto& ap : ap_list) {
        auto pb_ap = pb_creator.add_access_points();
        pb_creator.fill(ap.second, pb_ap, depth);
    }
}

}  // namespace

void access_points(PbCreator& pb_creator,
                   const std::string& filter,
                   int count,
                   int depth,
                   int start_page,
                   const ForbiddenUris& forbidden_uris) {
    const type::Data& data = *pb_creator.data;
    AccessPointList ap_list;
    try {
        const type::Indexes stop_point_indexes =
            ptref::make_query(type::Type_e::StopPoint, filter, forbidden_uris, data);
        const auto stop_points = data.get_data<type::StopPoint>(stop_point_indexes);

        // make Acess Point unique in to the response
        for (auto& _sp : stop_points) {
            for (auto& ap : _sp->access_points) {
                auto it = ap_list.find(ap.name);
                if (it == ap_list.end()) {
                    ap_list[ap.name] = ap;
                }
            }
        }

        const auto paginated_result = paginate(ap_list, count, start_page);
        fill_access_point_to_pb(paginated_result, pb_creator, depth);

        pb_creator.make_paginate(ap_list.size(), start_page, count, paginated_result.size());

    } catch (const ptref::parsing_error& parse_error) {
        pb_creator.fill_pb_error(pbnavitia::Error::unable_to_parse, "Unable to parse filter" + parse_error.more);
        return;
    } catch (const ptref::ptref_error& ptref_error) {
        pb_creator.fill_pb_error(pbnavitia::Error::bad_filter, "ptref : " + ptref_error.more);
        return;
    }
}

}  // namespace access_point
}  // namespace navitia
