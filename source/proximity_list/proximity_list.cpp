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
std::vector<std::pair<T, GeographicalCoord>> ProximityList<T>::find_within(GeographicalCoord coord,
                                                                           double radius) const {
    std::vector<std::pair<T, GeographicalCoord>> result;

    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");

    if (!index) {
        flann::Matrix<float> points{&data[0], data.size() / 3, 3};
        index = std::make_shared<index_t>(points, flann::KDTreeSingleIndexParams(10));

        LOG4CPLUS_INFO(logger, " Building NN Index!!!!!!!!!!");

        index->buildIndex();
    }
    float x = GeographicalCoord::EARTH_RADIUS_IN_METERS * cos(coord.lat() * GeographicalCoord::N_DEG_TO_RAD)
              * sin(coord.lon() * GeographicalCoord::N_DEG_TO_RAD);
    float y = GeographicalCoord::EARTH_RADIUS_IN_METERS * cos(coord.lat() * GeographicalCoord::N_DEG_TO_RAD)
              * cos(coord.lon() * GeographicalCoord::N_DEG_TO_RAD);
    float z = GeographicalCoord::EARTH_RADIUS_IN_METERS * sin(coord.lat() * GeographicalCoord::N_DEG_TO_RAD);

    static const size_t nb_nearest_neighbours = 200;
    // TODO: use std::array in c++17?
    // Acoording to the doc, std::array satisfies the requirements of ContiguousContainer(since C++17)
    std::vector<int> indices_data(nb_nearest_neighbours, -1);
    flann::Matrix<int> indices(&indices_data[0], 1, nb_nearest_neighbours);

    std::vector<index_t::DistanceType> distances_data(nb_nearest_neighbours, -1);
    flann::Matrix<index_t::DistanceType> distances(&distances_data[0], 1, nb_nearest_neighbours);

    float query_data[3] = {x, y, z};

    int nb_found =
        index->radiusSearch({query_data, 1, 3}, indices, distances, radius * radius * 1.4, flann::SearchParams());
    if (!nb_found) {
        LOG4CPLUS_INFO(log4cplus::Logger::getInstance("log"),
                       "0 point found for the coord: " << coord.lon() << " " << coord.lat());
        return result;
    }

    for (int res_ind : indices_data) {
        if (res_ind > static_cast<int>(items.size()) || res_ind < 0) {
            continue;
        }
        result.push_back(std::make_pair(items[res_ind].element, GeographicalCoord{}));
    }

    return result;
}

template class ProximityList<unsigned int>;
template class ProximityList<unsigned long>;

}  // namespace proximitylist
}  // namespace navitia
