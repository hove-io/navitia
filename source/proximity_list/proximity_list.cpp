#include "proximity_list.h"
#include "type/type.h"
#include "utils/logger.h"

#include <cmath>
#include <array>
#include <exception>
#include <flann/flann.hpp>

namespace navitia {
namespace proximitylist {
using type::GeographicalCoord;

static std::array<float, 3> project_coord(const type::GeographicalCoord& coord) {
    std::array<float, 3> res;
    float lat_grad = coord.lat() * GeographicalCoord::N_DEG_TO_RAD;
    float lon_grad = coord.lon() * GeographicalCoord::N_DEG_TO_RAD;
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
 * (1%), the approximation gives quite good result.( sin(0.01) - 0.01 < 1.7e-7 ! )
 *
 * The correction factor is computed only when the search radius > 1%*Earth's radius
 *
 * */
static float search_radius_correction_factor(const double& radius) {
    return radius < GeographicalCoord::EARTH_RADIUS_IN_METERS * 0.01
               ? 1.f
               : 2 * GeographicalCoord::EARTH_RADIUS_IN_METERS
                     * asin(radius / (2.f * GeographicalCoord::EARTH_RADIUS_IN_METERS)) / radius;
}

template <class T>
void ProximityList<T>::build() {
    // TODO build the Flann index here
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
    LOG4CPLUS_INFO(logger, "Building Proximitylist's NN index with " << items.size() << " items");

    // clean NN index
    NN_data.clear();
    NN_index.reset();

    if (items.empty()) {
        LOG4CPLUS_INFO(logger, "No items for building the index");
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

// struct IndexAndCoord;
// struct IndexOnly;
//
// template<typename T, typename Tag>
// struct ReturnTrait;
//
// template<typename T>
// struct ReturnTrait<T, IndexAndCoord> {
//    typedef std::vector<std::pair<T, GeographicalCoord>> ReturnType;
//};
//
// template<typename T>
// struct ReturnTrait<T, IndexOnly> {
//    typedef std::vector<T> ReturnType;
//};
//
// struct AutoResize;
// struct FixedSize;
//
// template<typename T>
// struct NNQueryTrait;
//
// template<>
// struct NNQueryTrait<AutoResize> {
//    typedef std::vector<std::vector<int>> IndiceType;
//    typedef std::vector<std::vector<navitia::proximitylist::index_t::DistanceType>> DistanceType;
//};
//
// template<>
// struct NNQueryTrait<FixedSize> {
//    const static std::size_t max_size = 200;
//    typedef std::array<int, max_size> QueryType;
//};

template <typename IndexType, typename DistanceType>
int find_within_impl(const std::shared_ptr<index_t>& NN_index,
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
                                          pow(radius * factor, 2), search_param);

    LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("log"),
                    "" << nb_found << " point found for the coord: " << coord.lon() << " " << coord.lat());

    return nb_found;
}

template <class T>
std::vector<std::pair<T, GeographicalCoord>> ProximityList<T>::find_within(const GeographicalCoord& coord,
                                                                           double radius,
                                                                           int size) const {
    std::vector<std::pair<T, GeographicalCoord>> result;
    if (!NN_index || !size || !radius) {
        return result;
    }
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");

    std::vector<std::vector<int>> indices;
    std::vector<std::vector<index_t::DistanceType>> distances;

    int nb_found = find_within_impl(NN_index, coord, radius, size, indices, distances);

    if (!nb_found) {
        LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("log"),
                        "0 point found for the coord: " << coord.lon() << " " << coord.lat());
        return result;
    }

    assert(indices.size() == 1);
    assert(distances.size() == 1);

    for (int i = 0; i < nb_found; ++i) {
        int res_ind = indices[0][i];
        if (res_ind < 0 || res_ind >= static_cast<int>(items.size())) {
            continue;
        }
        LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("log"), "Distance for the coord: " << coord.lon() << " "
                                                                                          << coord.lat() << " is "
                                                                                          << distances[0][res_ind]);
        result.emplace_back(items[res_ind].element, items[res_ind].coord);
    }
    return result;
}

template <typename T>
std::vector<T> ProximityList<T>::find_within_index_only(const GeographicalCoord& coord, double radius, int size) const {
    std::vector<T> result;
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");

    if (!NN_index || !size || !radius) {
        return result;
    }

    const static std::size_t max_size = 200;

    std::array<int, max_size> indices_data;
    flann::Matrix<int> indices(&indices_data[0], 1, size == -1 ? max_size : size);

    std::array<index_t::DistanceType, max_size> distances_data;
    flann::Matrix<index_t::DistanceType> distances(&distances_data[0], 1, size == -1 ? max_size : size);

    int nb_found = find_within_impl(NN_index, coord, radius, size, indices, distances);

    if (!nb_found) {
        LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("log"),
                        "0 point found for the coord: " << coord.lon() << " " << coord.lat());
        return result;
    }

    for (int i = 0; i < nb_found; ++i) {
        int res_ind = indices_data[i];
        if (res_ind < 0 || res_ind >= static_cast<int>(items.size())) {
            continue;
        }
        LOG4CPLUS_TRACE(log4cplus::Logger::getInstance("log"),
                        "Distance for the coord: " << coord.lon() << " " << coord.lat() << " is " << indices_data[i]);
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
