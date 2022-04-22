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

#include "proximitylist_api.h"

#include "type/pb_converter.h"
#include "georef/georef.h"
#include "utils/paginate.h"

namespace navitia {
namespace proximitylist {
/**
 * se charge de remplir l'objet protocolbuffer autocomplete passé en paramètre
 *
 */
using t_result =
    std::tuple<nt::idx_t, nt::GeographicalCoord, float, nt::Type_e, std::vector<std::tuple<nt::idx_t, float>>>;
using idx_coord_distance = std::tuple<nt::idx_t, nt::GeographicalCoord, float>;
using Vector_idx_coord_distance = std::vector<idx_coord_distance>;

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
        auto stop_points_nearby_idx_distance = std::get<4>(result_item);
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
                const auto& ng_address = navitia::georef::Address(data.geo_ref->ways[idx], coord,
                                                                  data.geo_ref->ways[idx]->nearest_number(coord).first);
                pb_creator.fill(&ng_address, place, depth);
                place->set_distance(coord.distance_to(coord_item));
                break;
            }
            default:
                break;
        }
        // add stop points nearby (PtObject) into response
        for (const auto& sp_idx_distance : stop_points_nearby_idx_distance) {
            pbnavitia::PtObject* pt_obj = place->add_stop_points_nearby();
            pb_creator.fill(data.pt_data->stop_points[std::get<0>(sp_idx_distance)], pt_obj, 0);
            pt_obj->set_distance(sqrt(std::get<1>(sp_idx_distance)));
        }
    }
}

static void cut(Vector_idx_coord_distance& idx_coord_distance, const size_t end_pagination) {
    const auto nb_sort = std::min(idx_coord_distance.size(), end_pagination);
    idx_coord_distance.resize(nb_sort);
}

void find(navitia::PbCreator& pb_creator,
          const type::GeographicalCoord& coord,
          const double limit,
          const std::vector<nt::Type_e>& types,
          const std::vector<std::string>& forbidden_uris,
          const std::string& filter,
          const uint32_t depth,
          const uint32_t count,
          const uint32_t start_page,
          const type::Data& data,
          const double stop_points_nearby_radius) {
    int total_result = 0;
    std::vector<t_result> result;
    auto end_pagination = (start_page + 1) * count;
    for (nt::Type_e type : types) {
        if (type == nt::Type_e::Address) {
            // for addresses we use the street network
            try {
                auto nb_w = pb_creator.data->geo_ref->nearest_addr(coord);
                // we'll regenerate the good number in make_pb
                result.emplace_back(nb_w.second->idx, std::move(coord), 0, type,
                                    std::vector<std::tuple<nt::idx_t, float>>());
                ++total_result;
            } catch (const proximitylist::NotFound&) {
            }
            continue;
        }

        Vector_idx_coord_distance vector_idx_coord_distance;
        type::Indexes indexes;
        if (!filter.empty()) {
            try {
                indexes = ptref::make_query(type, filter, forbidden_uris, *pb_creator.data);
            } catch (const ptref::parsing_error& parse_error) {
                pb_creator.fill_pb_error(pbnavitia::Error::unable_to_parse,
                                         "Problem while parsing the query:" + parse_error.more);
                return;
            } catch (const std::exception&) {
            }
        }
        // We have to find all objects within distance, then apply the filter
        int search_count = -1;
        switch (type) {
            case nt::Type_e::StopArea:
                vector_idx_coord_distance =
                    pb_creator.data->pt_data->stop_area_proximity_list.find_within<IndexCoordDistance>(coord, limit,
                                                                                                       search_count);
                break;
            case nt::Type_e::StopPoint:
                vector_idx_coord_distance =
                    pb_creator.data->pt_data->stop_point_proximity_list.find_within<IndexCoordDistance>(coord, limit,
                                                                                                        search_count);
                break;
            case nt::Type_e::POI:
                vector_idx_coord_distance =
                    pb_creator.data->geo_ref->poi_proximity_list.find_within<IndexCoordDistance>(coord, limit,
                                                                                                 search_count);
                break;
            default:
                break;
        }

        Vector_idx_coord_distance final_vector_idx_coord_distance;
        if (filter.empty()) {
            final_vector_idx_coord_distance = vector_idx_coord_distance;
        } else {
            for (const auto& element : vector_idx_coord_distance) {
                auto idx = std::get<0>(element);
                if (indexes.find(idx) != indexes.cend()) {
                    final_vector_idx_coord_distance.push_back(element);
                }
            }
        }

        total_result += final_vector_idx_coord_distance.size();
        cut(final_vector_idx_coord_distance, end_pagination);
        for (const auto& elem : final_vector_idx_coord_distance) {
            auto idx = std::get<0>(elem);
            auto coord = std::get<1>(elem);
            auto distance = std::get<2>(elem);
            std::vector<std::tuple<nt::idx_t, float>> stop_points_nearby_idx_distance;
            // for each result found, we perform a new proximity research for stop point type.
            if (stop_points_nearby_radius > 0) {
                auto stop_points_nearby_idx_coord_distance =
                    pb_creator.data->pt_data->stop_point_proximity_list.find_within<IndexCoordDistance>(
                        std::get<1>(elem), stop_points_nearby_radius, search_count);
                stop_points_nearby_idx_distance.reserve(stop_points_nearby_idx_coord_distance.size());
                for (const auto& sp_idx : stop_points_nearby_idx_coord_distance) {
                    stop_points_nearby_idx_distance.emplace_back(std::get<0>(sp_idx), std::get<2>(sp_idx));
                }
            }
            result.emplace_back(idx, std::move(coord), distance, type, std::move(stop_points_nearby_idx_distance));
        }
    }
    result = paginate(result, count, start_page);
    make_pb(pb_creator, result, depth, data, coord);
    pb_creator.make_paginate(total_result, start_page, count, result.size());
}

}  // namespace proximitylist
}  // namespace navitia
