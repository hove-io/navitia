#include "proximity_list.h"
#include "type/type.h"
#include "utils/logger.h"
#include "flann/flann.hpp"

#include <cmath>
#include <array>
#include <exception>

namespace navitia {
namespace proximitylist {
using type::GeographicalCoord;
using index_t = flann::Index<flann::L2<float>>;

template <class T>
std::vector<std::pair<T, GeographicalCoord>> ProximityList<T>::find_within(const GeographicalCoord& coord,
                                                                           double radius,
                                                                           int size) const {
    std::vector<std::pair<T, GeographicalCoord>> result;

    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");

    if (!index) {
        flann::Matrix<float> points{&NN_data[0], NN_data.size() / 3, 3};
        index = std::make_shared<index_t>(points, flann::KDTreeSingleIndexParams(10));

        LOG4CPLUS_INFO(logger, " Building NN Index!!!!!!!!!!");

        index->buildIndex();
    }
    float x = GeographicalCoord::EARTH_RADIUS_IN_METERS * cos(coord.lat() * GeographicalCoord::N_DEG_TO_RAD)
              * sin(coord.lon() * GeographicalCoord::N_DEG_TO_RAD);
    float y = GeographicalCoord::EARTH_RADIUS_IN_METERS * cos(coord.lat() * GeographicalCoord::N_DEG_TO_RAD)
              * cos(coord.lon() * GeographicalCoord::N_DEG_TO_RAD);
    float z = GeographicalCoord::EARTH_RADIUS_IN_METERS * sin(coord.lat() * GeographicalCoord::N_DEG_TO_RAD);

    // TODO: use std::array in c++17?
    // Acoording to the doc, std::array satisfies the requirements of ContiguousContainer(since C++17)
    const static std::size_t max_size = 5000;

    std::array<int, max_size> indices_data;
    flann::Matrix<int> indices(&indices_data[0], 1, size == -1 ? max_size : size);

    std::array<float, max_size> distances_data;
    flann::Matrix<index_t::DistanceType> distances(&distances_data[0], 1, size == -1 ? max_size : size);

    float query_data[3] = {x, y, z};
    auto search_param = flann::SearchParams{};
    search_param.max_neighbors = size;  // -1 -> unlimited

    int nb_found = index->radiusSearch({query_data, 1, 3}, indices, distances, radius * radius * 1.1025, search_param);
    if (!nb_found) {
        LOG4CPLUS_INFO(log4cplus::Logger::getInstance("log"),
                       "0 point found for the coord: " << coord.lon() << " " << coord.lat());
        return result;
    }

    for (int i = 0; i <= nb_found; ++i) {
        int res_ind = indices_data[i];
        if (res_ind < 0 || res_ind >= items.size()) {
            continue;
        }
        result.emplace_back(items[res_ind].element, items[res_ind].coord);
    }

    return result;
}

template <typename T>
std::vector<T> ProximityList<T>::find_within_index_only(const GeographicalCoord& coord, double radius, int size) const {
    std::vector<T> result;
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");

    if (!index) {
        flann::Matrix<float> points{&NN_data[0], NN_data.size() / 3, 3};
        index = std::make_shared<index_t>(points, flann::KDTreeSingleIndexParams(10));

        LOG4CPLUS_INFO(logger, " Building NN Index!!!!!!!!!!");

        index->buildIndex();
    }
    float x = GeographicalCoord::EARTH_RADIUS_IN_METERS * cos(coord.lat() * GeographicalCoord::N_DEG_TO_RAD)
              * sin(coord.lon() * GeographicalCoord::N_DEG_TO_RAD);
    float y = GeographicalCoord::EARTH_RADIUS_IN_METERS * cos(coord.lat() * GeographicalCoord::N_DEG_TO_RAD)
              * cos(coord.lon() * GeographicalCoord::N_DEG_TO_RAD);
    float z = GeographicalCoord::EARTH_RADIUS_IN_METERS * sin(coord.lat() * GeographicalCoord::N_DEG_TO_RAD);

    // TODO: use std::array in c++17?
    // Acoording to the doc, std::array satisfies the requirements of ContiguousContainer(since C++17)
    const static std::size_t max_size = 200;

    std::array<int, max_size> indices_data;
    flann::Matrix<int> indices(&indices_data[0], 1, size == -1 ? max_size : size);

    std::array<float, max_size> distances_data;
    flann::Matrix<index_t::DistanceType> distances(&distances_data[0], 1, size == -1 ? max_size : size);

    float query_data[3] = {x, y, z};
    auto search_param = flann::SearchParams{};
    search_param.max_neighbors = size;  // -1 -> unlimited

    int nb_found = index->radiusSearch({query_data, 1, 3}, indices, distances, radius * radius * 1.1025, search_param);
    if (!nb_found) {
        LOG4CPLUS_INFO(log4cplus::Logger::getInstance("log"),
                       "0 point found for the coord: " << coord.lon() << " " << coord.lat());
        return result;
    }

    for (int i = 0; i <= nb_found; ++i) {
        int res_ind = indices_data[i];
        if (res_ind < 0 || res_ind >= items.size()) {
            continue;
        }
        result.emplace_back(items[res_ind].element);
    }

    return result;
}

template class ProximityList<unsigned int>;
template class ProximityList<unsigned long>;

}  // namespace proximitylist
}  // namespace navitia
