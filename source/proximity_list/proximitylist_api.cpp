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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "proximitylist_api.h"

#include "type/pb_converter.h"
#include "utils/paginate.h"

namespace navitia {
namespace proximitylist {
/**
 * se charge de remplir l'objet protocolbuffer autocomplete passé en paramètre
 *
 */
using t_result = std::tuple<nt::idx_t, nt::GeographicalCoord, float, nt::Type_e>;
using idx_coord_distance = std::tuple<nt::idx_t, nt::GeographicalCoord, float>;
using vector_idx_coord_distance = std::vector<idx_coord_distance>;

static void make_pb(navitia::PbCreator& pb_creator,
                    const std::vector<t_result>& result,
                    uint32_t depth,
                    const nt::Data& data,
                    type::GeographicalCoord coord) {
    for (auto result_item : result) {
        pbnavitia::PtObject* place = pb_creator.add_places_nearby();
        auto idx = std::get<0>(result_item);
        auto coord_item = std::get<1>(result_item);
        auto distance = sqrt(std::get<2>(result_item));
        auto type = std::get<3>(result_item);
        switch (type) {
            case nt::Type_e::StopArea:
                pb_creator.fill(data.pt_data->stop_areas[idx], place, depth);
                place->set_distance(distance);
                break;
            case nt::Type_e::StopPoint:
                pb_creator.fill(data.pt_data->stop_points[idx], place, depth);
                place->set_distance(distance);
                break;
            case nt::Type_e::POI:
                pb_creator.fill(data.geo_ref->pois[idx], place, depth);
                place->set_distance(distance);
                break;
            case nt::Type_e::Address: {
                const auto& way_coord = navitia::WayCoord(data.geo_ref->ways[idx], coord,
                                                          data.geo_ref->ways[idx]->nearest_number(coord).first);
                pb_creator.fill(&way_coord, place, depth);
                place->set_distance(coord.distance_to(coord_item));
                break;
            }
            default:
                break;
        }
    }
}

static void cut(vector_idx_coord_distance& list, const size_t end_pagination, const nt::GeographicalCoord& coord) {
    const auto nb_sort = std::min(list.size(), end_pagination);
    list.resize(nb_sort);
}

void find(navitia::PbCreator& pb_creator,
          const type::GeographicalCoord& coord,
          const double distance,
          const std::vector<nt::Type_e>& types,
          const std::string& filter,
          const uint32_t depth,
          const uint32_t count,
          const uint32_t start_page,
          const type::Data& data) {
    int total_result = 0;
    std::vector<t_result> result;
    auto end_pagination = (start_page + 1) * count;
    for (nt::Type_e type : types) {
        if (type == nt::Type_e::Address) {
            // for addresses we use the street network
            try {
                auto nb_w = pb_creator.data->geo_ref->nearest_addr(coord);
                // we'll regenerate the good number in make_pb
                result.emplace_back(nb_w.second->idx, coord, 0, type);
                ++total_result;
            } catch (const proximitylist::NotFound&) {
            }
            continue;
        }

        vector_idx_coord_distance list;
        type::Indexes indexes;
        if (!filter.empty()) {
            try {
                indexes = ptref::make_query(type, filter, *pb_creator.data);
            } catch (const ptref::parsing_error& parse_error) {
                pb_creator.fill_pb_error(pbnavitia::Error::unable_to_parse,
                                         "Problem while parsing the query:" + parse_error.more);
                return;
            } catch (const ptref::ptref_error& pt_error) {
                pb_creator.fill_pb_error(pbnavitia::Error::bad_filter, "ptref : " + pt_error.more);
                return;
            }
        }
        // We have to find all objects within distance, then apply the filter
        int search_count = -1;
        switch (type) {
            case nt::Type_e::StopArea:
                list = pb_creator.data->pt_data->stop_area_proximity_list.find_within<IndexCoordDistance>(
                    coord, distance, search_count);
                break;
            case nt::Type_e::StopPoint:
                list = pb_creator.data->pt_data->stop_point_proximity_list.find_within<IndexCoordDistance>(
                    coord, distance, search_count);
                break;
            case nt::Type_e::POI:
                list = pb_creator.data->geo_ref->poi_proximity_list.find_within<IndexCoordDistance>(coord, distance,
                                                                                                    search_count);
                break;
            default:
                break;
        }

        vector_idx_coord_distance final_list;
        if (filter.empty()) {
            final_list = list;
        } else {
            for (const auto& element : list) {
                auto idx = std::get<0>(element);
                if (indexes.find(idx) != indexes.cend()) {
                    final_list.push_back(element);
                }
            }
        }
        total_result += final_list.size();
        cut(final_list, end_pagination, coord);
        for (const auto& e : final_list) {
            auto idx = std::get<0>(e);
            auto coord = std::get<1>(e);
            auto distance = std::get<2>(e);
            result.emplace_back(idx, std::move(coord), distance, type);
        }
    }
    result = paginate(result, count, start_page);
    make_pb(pb_creator, result, depth, data, coord);
    pb_creator.make_paginate(total_result, start_page, count, result.size());
}
}  // namespace proximitylist
}  // namespace navitia
