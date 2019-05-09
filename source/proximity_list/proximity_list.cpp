#include "proximity_list.h"
#include "type/type.h"
#include "utils/logger.h"
#include "flann/flann.hpp"

#include <cmath>
#include <array>
#include <exception>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <eos_portable_archive/portable_iarchive.hpp>
#include <eos_portable_archive/portable_oarchive.hpp>

namespace navitia {
namespace proximitylist {
using type::GeographicalCoord;

constexpr size_t max_node_search_tree = 10;
template <class T>
void ProximityList<T>::build() {
    // TODO build the Flann index here
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");

    if (items.empty()) {
        LOG4CPLUS_INFO(logger, "No items for building the index");
        return;
    }
    NN_data.clear();
    NN_data.reserve(items.size() * 3);
    for (const auto& i : items) {
        const auto& coord = i.coord;
        float x = GeographicalCoord::EARTH_RADIUS_IN_METERS * cos(coord.lat() * GeographicalCoord::N_DEG_TO_RAD)
                  * sin(coord.lon() * GeographicalCoord::N_DEG_TO_RAD);
        float y = GeographicalCoord::EARTH_RADIUS_IN_METERS * cos(coord.lat() * GeographicalCoord::N_DEG_TO_RAD)
                  * cos(coord.lon() * GeographicalCoord::N_DEG_TO_RAD);
        float z = GeographicalCoord::EARTH_RADIUS_IN_METERS * sin(coord.lat() * GeographicalCoord::N_DEG_TO_RAD);
        NN_data.push_back(x);
        NN_data.push_back(y);
        NN_data.push_back(z);
    }
    flann::Matrix<float> points{&NN_data[0], NN_data.size() / 3, 3};

    // index = std::make_shared<index_t>(points, flann::KDTreeIndexParams(4));
    index = std::make_shared<index_t>(points, flann::KDTreeSingleIndexParams(10));

    index->buildIndex();
}

template <class T>
std::vector<std::pair<T, GeographicalCoord>> ProximityList<T>::find_within(const GeographicalCoord& coord,
                                                                           double radius,
                                                                           int size) const {
    if (NN_data.empty()) {
        return {};
    }

    std::vector<std::pair<T, GeographicalCoord>> result;

    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");

    if (!index) {
        flann::Matrix<float> points{&NN_data[0], NN_data.size() / 3, 3};
        index = std::make_shared<index_t>(points, flann::KDTreeSingleIndexParams(max_node_search_tree));

        LOG4CPLUS_INFO(logger, " find_within Building NN Index!!!!!!!!!!");

        index->buildIndex();
    }
    float x = GeographicalCoord::EARTH_RADIUS_IN_METERS * cos(coord.lat() * GeographicalCoord::N_DEG_TO_RAD)
              * sin(coord.lon() * GeographicalCoord::N_DEG_TO_RAD);
    float y = GeographicalCoord::EARTH_RADIUS_IN_METERS * cos(coord.lat() * GeographicalCoord::N_DEG_TO_RAD)
              * cos(coord.lon() * GeographicalCoord::N_DEG_TO_RAD);
    float z = GeographicalCoord::EARTH_RADIUS_IN_METERS * sin(coord.lat() * GeographicalCoord::N_DEG_TO_RAD);

    // TODO: use std::array in c++17?
    // Acoording to the doc, std::array satisfies the requirements of ContiguousContainer(since C++17)

    //    std::array<int, max_size> indices_data;
    //    flann::Matrix<int> indices(&indices_data[0], 1, size == -1 ? max_size : size);

    std::vector<std::vector<int>> indices(1);
    if (size > 0) {
        indices[0].reserve(size);
    }
    //    std::array<float, max_size> distances_data;
    //    flann::Matrix<index_t::DistanceType> distances(&distances_data[0], 1, size == -1 ? max_size : size);
    std::vector<std::vector<index_t::DistanceType>> distances(1);
    if (size > 0) {
        distances[0].reserve(size);
    }

    float query_data[3] = {x, y, z};
    auto search_param = flann::SearchParams{};
    search_param.max_neighbors = size;  // -1 -> unlimited
    search_param.sorted = true;         // -1 -> unlimited

    float coeff = radius < GeographicalCoord::EARTH_RADIUS_IN_METERS * 0.01
                      ? 1.f
                      : 2 * GeographicalCoord::EARTH_RADIUS_IN_METERS
                            * asin(radius / (2.f * GeographicalCoord::EARTH_RADIUS_IN_METERS)) / radius;

    int nb_found = index->radiusSearch(flann::Matrix<float>{query_data, 1, 3}, indices, distances,
                                       pow(radius * coeff, 2), search_param);
    LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("log"),
                    "" << nb_found << " point found for the coord: " << coord.lon() << " " << coord.lat());

    if (!nb_found) {
        return result;
    }

    for (int i = 0; i < nb_found; ++i) {
        int res_ind = indices[0][i];
        if (res_ind < 0 || res_ind >= items.size()) {
            continue;
        }
        LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("log"), "distances " << distances[0][res_ind]);

        result.emplace_back(items[res_ind].element, items[res_ind].coord);
    }

    return result;
}

template <typename T>
std::vector<T> ProximityList<T>::find_within_index_only(const GeographicalCoord& coord, double radius, int size) const {
    std::vector<T> result;
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");

    if (NN_data.empty()) {
        return {};
    }
    if (!index) {
        flann::Matrix<float> points{&NN_data[0], NN_data.size() / 3, 3};
        index = std::make_shared<index_t>(points, flann::KDTreeSingleIndexParams(max_node_search_tree));

        LOG4CPLUS_INFO(logger, " find_within_index_only Building NN Index!!!!!!!!!!");

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

    std::array<index_t::DistanceType, max_size> distances_data;
    flann::Matrix<index_t::DistanceType> distances(&distances_data[0], 1, size == -1 ? max_size : size);

    float query_data[3] = {x, y, z};
    auto search_param = flann::SearchParams{};
    search_param.max_neighbors = size;  // -1 -> unlimited

    // Why the search raius is equals to pow((radius * 1.05), 2) ?
    // Keep in mind that this is an approximate way of calculating distance between to coordinates
    // In theory the largest error of the real distance and the approximate one is 36% (1- PI/2) (when two coordinates
    // fall on opposite poles of a sphere) ex (0, -90)  (0, 90) However, when one coordinate falls on a south pole, and
    // another falls on equator, ex (0, -90, (0, 0) the error is about 5% (1 - sqrt(2) / (PI/2)) In fact, in practice,
    // the error is quite small between two coordiantes situated on the same continent.

    int nb_found = index->radiusSearch(flann::Matrix<float>{query_data, 1, 3}, indices, distances,
                                       pow((radius * 1.05), 2), search_param);
    if (!nb_found) {
        LOG4CPLUS_INFO(log4cplus::Logger::getInstance("log"),
                       "0 point found for the coord: " << coord.lon() << " " << coord.lat());
        return result;
    }

    for (int i = 0; i < nb_found; ++i) {
        int res_ind = indices_data[i];
        if (res_ind < 0 || res_ind >= items.size()) {
            continue;
        }
        result.emplace_back(items[res_ind].element);
    }

    return result;
}

///** Fonction qui permet de sérialiser (aka binariser la structure de données
// *
// * Elle est appelée par boost et pas directement
// */
// template<typename T>
// template <class Archive>
// void ProximityList<T>::serialize(Archive& ar, const unsigned int) {
//    ar& items & NN_data;
//}

template class ProximityList<unsigned int>;

// template <> template<>
// void ProximityList<unsigned int>::serialize<boost::archive::binary_oarchive>(boost::archive::binary_oarchive& ar ,
// const unsigned int) {
//    ar& items & NN_data & index;
//};
// template <> template<>
// void ProximityList<unsigned int>::serialize<eos::portable_iarchive>(eos::portable_iarchive&, const unsigned int);
// template <> template<>
// void ProximityList<unsigned int>::serialize<boost::archive::binary_oarchive>(boost::archive::binary_oarchive&, const
// unsigned int);
//
//
template class ProximityList<unsigned long>;
//
// template <> template<>
// void ProximityList<unsigned long>::serialize<boost::archive::binary_oarchive>(boost::archive::binary_oarchive& ar,
// const unsigned int) {
//    ar& items & NN_data & index;
//};
// template <> template<>
// void ProximityList<unsigned long>::serialize<eos::portable_iarchive>(eos::portable_iarchive&, const unsigned int);
// template <> template<>
// void ProximityList<unsigned long>::serialize<boost::archive::binary_oarchive>(boost::archive::binary_oarchive&, const
// unsigned int);

}  // namespace proximitylist
}  // namespace navitia
