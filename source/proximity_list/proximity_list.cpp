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

#include "proximity_list.h"

#include "type/geographical_coord.h"

#include <flann/flann.hpp>

#include <array>
#include <cmath>
#include <exception>

namespace navitia {
namespace proximitylist {
using type::GeographicalCoord;

static std::array<float, 3> project_coord(const type::GeographicalCoord& coord) {
    std::array<float, 3> res{};
    double lat_grad = coord.lat() * GeographicalCoord::N_DEG_TO_RAD;
    double lon_grad = coord.lon() * GeographicalCoord::N_DEG_TO_RAD;
    res[0] = GeographicalCoord::EARTH_RADIUS_IN_METERS * cos(lat_grad) * sin(lon_grad);
    res[1] = GeographicalCoord::EARTH_RADIUS_IN_METERS * cos(lat_grad) * cos(lon_grad);
    res[2] = GeographicalCoord::EARTH_RADIUS_IN_METERS * sin(lat_grad);
    return res;
}

/*
 * This factor is computed depending on the ratio between the search radius and
 * the earth's radius.
 *
 * In theory, mathematically, according the Taylor's theorem, if the search radius is far less than the earth's radius
 * (1%), the approximation gives quite good result.( (sin(0.01rad) - 0.01) < 1.7e-7 ! )
 *
 * Otherwise, the correction factor is computed (only when the search radius > 1%*Earth's radius)
 *
 * */
static float search_radius_correction_factor(const double& radius) {
    return radius < GeographicalCoord::EARTH_RADIUS_IN_METERS * 0.01
               ? 1.f
               : 2 * GeographicalCoord::EARTH_RADIUS_IN_METERS
                     * asin(radius / (2. * GeographicalCoord::EARTH_RADIUS_IN_METERS)) / radius;
}

template <class T>
void ProximityList<T>::build() {
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
    LOG4CPLUS_INFO(logger, "Building Proximitylist's NN index with " << items.size() << " items");

    // clean NN index
    NN_data.clear();
    NN_index.reset();

    if (items.empty()) {
        LOG4CPLUS_WARN(logger, "No items for building the index");
        return;
    }

    for (const auto& i : items) {
        auto projected = project_coord(i.coord);
        std::copy(projected.begin(), projected.end(), std::back_inserter(NN_data));
    }
    auto points = flann::Matrix<float>{&NN_data[0], NN_data.size() / 3, 3};
    NN_index = std::make_shared<navitia::proximitylist::index_t>(points, flann::KDTreeSingleIndexParams(10));
    NN_index->buildIndex();
}

template <typename T, typename Items, typename Indices, typename Distances, typename Out, typename F>
static void make_result(const type::GeographicalCoord& coord,
                        const Items& items,
                        const Indices& indices,
                        const Distances& distances,
                        const int nb_found,
                        Out& out,
                        F op) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");

    if (!nb_found) {
        LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("log"),
                        "0 point found for the coord: " << coord.lon() << " " << coord.lat());
        return;
    }
    for (int i = 0; i < nb_found; ++i) {
        int res_ind = indices.at(i);
        if (res_ind < 0 || res_ind >= static_cast<int>(items.size())) {
            continue;
        }
        LOG4CPLUS_TRACE(
            log4cplus::Logger::getInstance("log"),
            "Distance(squared) from the coord: " << coord.lon() << " " << coord.lat() << " is " << distances.at(i));
        const auto& item = items[res_ind];
        out.push_back(op(item, distances.at(i)));
    }
}

template <typename IndexType, typename DistanceType>
static int radius_search(const std::shared_ptr<index_t>& NN_index,
                         const GeographicalCoord& coord,
                         double radius,
                         int size,
                         IndexType& indices,
                         DistanceType& distances) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");

    auto query = project_coord(coord);

    auto search_param = flann::SearchParams{};
    search_param.max_neighbors = size;  // -1 -> unlimited

    radius = std::min(radius, 2 * GeographicalCoord::EARTH_RADIUS_IN_METERS);

    float factor = search_radius_correction_factor(radius);
    int nb_found = NN_index->radiusSearch(flann::Matrix<float>{&query[0], 1, 3}, indices, distances,
                                          pow(radius * static_cast<double>(factor), 2), search_param);

    LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("log"),
                    "" << nb_found << " point found for the coord: " << coord.lon() << " " << coord.lat());

    return nb_found;
}

template <class T>
auto ProximityList<T>::find_within_impl(const GeographicalCoord& coord,
                                        const double radius,
                                        const int size,
                                        IndexCoord /*unused*/) const
    -> std::vector<typename ReturnTypeTrait<T, IndexCoord>::ValueType> {
    // Containers are auto-sized by NN_index, Flann will return all objects inside of the given radius
    std::vector<std::vector<int>> indices;
    std::vector<std::vector<index_t::DistanceType>> distances;
    int nb_found = radius_search(NN_index, coord, radius, size, indices, distances);
    assert(indices.size() == 1);
    assert(distances.size() == 1);

    std::vector<typename ReturnTypeTrait<T, IndexCoord>::ValueType> res;
    auto op = [](const Item& item, float /*unused*/) { return std::make_pair(item.element, item.coord); };
    make_result<T>(coord, items, indices[0], distances[0], nb_found, res, op);

    return res;
}

template <typename T>
auto ProximityList<T>::find_within_impl(const GeographicalCoord& coord,
                                        const double radius,
                                        const int size,
                                        IndexCoordDistance /*unused*/) const
    -> std::vector<typename ReturnTypeTrait<T, IndexCoordDistance>::ValueType> {
    // Containers are auto-sized by NN_index, Flann will return all objects inside of the given radius
    std::vector<std::vector<int>> indices;
    std::vector<std::vector<index_t::DistanceType>> distances;
    int nb_found = radius_search(NN_index, coord, radius, size, indices, distances);
    assert(indices.size() == 1);
    assert(distances.size() == 1);

    std::vector<typename ReturnTypeTrait<T, IndexCoordDistance>::ValueType> res;
    auto op = [](const Item& item, float distance) { return std::make_tuple(item.element, item.coord, distance); };
    make_result<T>(coord, items, indices[0], distances[0], nb_found, res, op);

    return res;
}

template <class T>
auto ProximityList<T>::find_within_impl(const GeographicalCoord& coord,
                                        const double radius,
                                        const int size,
                                        IndexOnly /*unused*/) const
    -> std::vector<typename ReturnTypeTrait<T, IndexOnly>::ValueType> {
    // Using small sized std::array will avoid heap allocation and limit the research
    const static std::size_t max_size = 100;
    std::array<int, max_size> indices_data{};
    flann::Matrix<int> indices(&indices_data[0], 1, size == -1 ? max_size : size);
    std::array<index_t::DistanceType, max_size> distances_data{};
    flann::Matrix<index_t::DistanceType> distances(&distances_data[0], 1, size == -1 ? max_size : size);
    int nb_found = radius_search(NN_index, coord, radius, size, indices, distances);

    std::vector<typename ReturnTypeTrait<T, IndexOnly>::ValueType> res;
    auto op = [](const Item& item, float /*unused*/) { return item.element; };
    make_result<T>(coord, items, indices_data, distances_data, nb_found, res, op);

    return res;
}

NotFound::~NotFound() noexcept = default;

template struct ProximityList<unsigned int>;
template struct ProximityList<unsigned long>;

}  // namespace proximitylist
}  // namespace navitia
