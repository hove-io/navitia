#include "proximity_list.h"
#include "type/type.h"
#include "utils/logger.h"
#include <cmath>
namespace navitia {
namespace proximitylist {
using type::GeographicalCoord;

template <class T>
std::vector<std::pair<T, GeographicalCoord> > ProximityList<T>::find_within(GeographicalCoord coord,
                                                                            double distance) const {
    double distance_degree = distance / 111320;

    double coslat = ::cos(coord.lat() * type::GeographicalCoord::N_DEG_TO_RAD);

    auto begin = std::lower_bound(items.begin(), items.end(), coord.lon() - distance_degree / coslat,
                                  [](const Item& i, double min) { return i.coord.lon() < min; });
    auto end = std::upper_bound(begin, items.end(), coord.lon() + distance_degree / coslat,
                                [](double max, const Item& i) { return max < i.coord.lon(); });
    std::vector<std::pair<T, GeographicalCoord> > result;
    double max_dist = distance * distance;

    const float max_lat = coord.lat() + distance_degree;
    const float min_lat = coord.lat() - distance_degree;
    for (; begin != end; ++begin) {
        const auto& tmp = begin->coord;

        if (std::signbit(tmp.lat() - min_lat) == std::signbit(tmp.lat() - max_lat)) {
            continue;
        }

        if (tmp.approx_sqr_distance(coord, coslat) <= max_dist)
            result.push_back(std::make_pair(begin->element, tmp));
    }
    std::sort(result.begin(), result.end(),
              [&coord, &coslat](const std::pair<T, GeographicalCoord>& a, const std::pair<T, GeographicalCoord>& b) {
                  return a.second.approx_sqr_distance(coord, coslat) < b.second.approx_sqr_distance(coord, coslat);
              });
    return result;
}

template class ProximityList<unsigned int>;
template class ProximityList<unsigned long>;

}  // namespace proximitylist
}  // namespace navitia
