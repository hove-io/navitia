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

#pragma once

#include "type/geographical_coord.h"
#include "utils/exception.h"
#include "utils/logger.h"

#include <memory>
#include <vector>

// Forward declaration
namespace flann {
template <typename T>
class Index;

template <typename T>
class L2;
}  // namespace flann

namespace navitia {
namespace proximitylist {

using type::GeographicalCoord;
using index_t = flann::Index<flann::L2<float>>;

struct NotFound : public recoverable_exception {
    NotFound() = default;
    NotFound(const NotFound&) = default;
    NotFound& operator=(const NotFound&) = default;
    ~NotFound() noexcept override;
};

// find_within Dispatch Tag
struct IndexOnly {};
struct IndexCoord {};
struct IndexCoordDistance {};

template <typename T, typename Tag>
struct ReturnTypeTrait;

template <typename T>
struct ReturnTypeTrait<T, IndexOnly> {
    using ValueType = T;
};

template <typename T>
struct ReturnTypeTrait<T, IndexCoord> {
    using ValueType = std::pair<T, GeographicalCoord>;
};

template <typename T>
struct ReturnTypeTrait<T, IndexCoordDistance> {
    using ValueType = std::tuple<T, GeographicalCoord, float>;
};
/* A structure allows to find K Nearest Neighbours with a given radius.
 *
 * The Item contains T(in practice, the Idx of the wanted object) and the coord of the object.
 *
 * This structure is used to do projection and find features(POI, stop_points, etc) nearby a wanted place.
 * An internal structure, KD-tree from flann is used to have a good perfomance.
 *
 * The coord is projected into 3D space so that we can performan a euclidean distance which can be highly optimized.
 *
 * */
template <class T>
struct ProximityList {
    struct Item {
        GeographicalCoord coord;
        T element;
        Item() = default;
        Item(GeographicalCoord coord, T element) : coord(coord), element(element) {}
        template <class Archive>
        void serialize(Archive& ar, const unsigned int) {
            ar& coord& element;
        }
    };

    /// Contient toutes les coordonnées de manière à trouver rapidement
    std::vector<Item> items;
    std::vector<float> NN_data;
    std::shared_ptr<index_t> NN_index = nullptr;

    /// Rajoute un nouvel élément. Attention, il faut appeler build avant de pouvoir utiliser la structure
    void add(GeographicalCoord coord, T element) { items.push_back(Item(coord, element)); }
    void clear() {
        items.clear();
        NN_data.clear();
    }

    // build the Nearest Neighbours data from items, then the index
    void build();

    /*
     * This method can return three types of result
     *
     * When Tag is IndexCorrd, the method returns a vector of Index and Coord, which is useful for searching
     * features nearby a wanted place.

     * When Tag is IndexCorrdDistance, the method returns a vector of Index, Coord and the Distance.
     *
     * If Tag is IndexOnly, the method returns a vector of Index, which is useful for coord projections.
     *
     * */
    template <typename Tag = IndexCoord>
    auto find_within(const GeographicalCoord& coord, double radius = 500, int size = -1) const
        -> std::vector<typename ReturnTypeTrait<T, Tag>::ValueType> {
        if (!NN_index || !size || !radius)
            return {};
        return find_within_impl(coord, radius, size, Tag{});
    }

    /// Fonction de confort pour retrouver l'élément le plus proche dans l'indexe
    T find_nearest(double lon, double lat) const { return find_nearest(GeographicalCoord(lon, lat)); }

    /// Retourne l'élément le plus proche dans tout l'indexe
    T find_nearest(const GeographicalCoord& coord, double max_dist = 500) const {
        auto temp = find_within<IndexOnly>(coord, max_dist, 1);
        if (temp.empty())
            throw NotFound();
        else
            return temp.front();
    }

    /** Fonction qui permet de sérialiser (aka binariser la structure de données
     *
     * Elle est appelée par boost et pas directement
     */
    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& items;
    }

private:
    /*
     * This implementation is used for /places_nearby
     *
     * Auto-sized container is used, NN_index will return ALL elements meet the criteira
     *
     * Note that this implementation returns the indices AND the coords of all the nearest elements
     * */
    auto find_within_impl(const GeographicalCoord& coord, const double radius, const int size, IndexCoord) const
        -> std::vector<typename ReturnTypeTrait<T, IndexCoord>::ValueType>;

    auto find_within_impl(const GeographicalCoord& coord, const double radius, const int size, IndexCoordDistance) const
        -> std::vector<typename ReturnTypeTrait<T, IndexCoordDistance>::ValueType>;

    /*
     * This implementation is used for edge projection
     *
     * A small size-fixed std::array container is used to boost the performance, the NN_index will stop when
     * the small size-fixed container is full.
     * .
     * Note that this implementation returns ONLY indices of nearest elements
     * */
    auto find_within_impl(const GeographicalCoord& coord, const double radius, const int size, IndexOnly) const
        -> std::vector<typename ReturnTypeTrait<T, IndexOnly>::ValueType>;
};

}  // namespace proximitylist
}  // namespace navitia
