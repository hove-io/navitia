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

#include "utils/serialization_vector.h"

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/linestring.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/multi/geometries/multi_polygon.hpp>
#include <boost/geometry/multi/geometries/multi_linestring.hpp>
#include <boost/geometry/multi/geometries/register/multi_linestring.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/functional/hash.hpp>

namespace navitia {
namespace type {

/** Coordonnées géographiques en WGS84
 */
struct GeographicalCoord {
    GeographicalCoord() : _lon(0), _lat(0) {}
    GeographicalCoord(double lon, double lat) : _lon(lon), _lat(lat) {}
    GeographicalCoord(const GeographicalCoord& coord) = default;
    GeographicalCoord& operator=(const GeographicalCoord& coord) = default;
    GeographicalCoord(double x, double y, bool) { set_xy(x, y); }

    double lon() const { return _lon; }
    double lat() const { return _lat; }
    std::string uri() const {
        return boost::lexical_cast<std::string>(_lon) + ";" + boost::lexical_cast<std::string>(_lat);
    }

    void set_lon(double lon) { this->_lon = lon; }
    void set_lat(double lat) { this->_lat = lat; }
    void set_xy(double x, double y) {
        this->set_lon(x * N_M_TO_DEG);
        this->set_lat(y * N_M_TO_DEG);
    }

    constexpr static double coord_epsilon = 1e-15;
    /// Ordre des coordonnées utilisé par ProximityList
    bool operator<(const GeographicalCoord& other) const {
        if (lon() < other.lon()) {
            return true;
        }
        if (other.lon() < lon()) {
            return false;
        }
        return lat() < other.lat();
    }
    bool operator!=(const GeographicalCoord& other) const {
        return fabs(lon() - other.lon()) > coord_epsilon || fabs(lat() - other.lat()) > coord_epsilon;
    }

    constexpr static double N_DEG_TO_RAD = 0.01745329238;
    constexpr static double N_M_TO_DEG = 1.0 / 111319.9;
    constexpr static double EARTH_RADIUS_IN_METERS = 6372797.560856;

    /** Calculate the distance between two points
     *
     * We use the Haversine frmula
     * http://en.wikipedia.org/wiki/Law_of_haversines
     *
     * If the coordinate is not in degree, thus we use euclidean distance
     */
    double distance_to(const GeographicalCoord& other) const;

    /** Projette un point sur un segment

       Retourne les coordonnées projetées et la distance au segment
       Si le point projeté tombe en dehors du segment, alors ça tombe sur le nœud le plus proche
       htCommercialtp://paulbourke.net/geometry/pointline/
       */

    template <typename F>
    std::pair<GeographicalCoord, float> project_common(const GeographicalCoord& segment_start,
                                                       const GeographicalCoord& segment_end,
                                                       F f) const;

    std::pair<type::GeographicalCoord, float> project(const GeographicalCoord& segment_start,
                                                      const GeographicalCoord& segment_end) const;

    std::pair<GeographicalCoord, float> approx_project(const GeographicalCoord& segment_start,
                                                       const GeographicalCoord& segment_end,
                                                       double coslat) const;

    /** Calcule la distance au carré grand arc entre deux points de manière approchée

        Cela sert essentiellement lorsqu'il faut faire plein de comparaisons de distances à un point (par exemple pour
       proximity list)
    */
    double approx_sqr_distance(const GeographicalCoord& other, double coslat) const {
        static const double EARTH_RADIUS_IN_METERS_SQUARE = 40612548751652.183023;
        double latitudeArc = (this->lat() - other.lat()) * N_DEG_TO_RAD;
        double longitudeArc = (this->lon() - other.lon()) * N_DEG_TO_RAD;
        double tmp = coslat * longitudeArc;
        return EARTH_RADIUS_IN_METERS_SQUARE * (latitudeArc * latitudeArc + tmp * tmp);
    }

    bool is_initialized() const { return distance_to(GeographicalCoord()) > 1; }

    bool is_valid() const {
        return this->lon() >= -180 && this->lon() <= 180 && this->lat() >= -90 && this->lat() <= 90;
    }

    template <class Archive>
    void serialize(Archive& ar, const unsigned int);

private:
    double _lon;
    double _lat;
};

std::ostream& operator<<(std::ostream&, const GeographicalCoord&);

/** 2 points are considered equals if a.distance_to(b) < 0.1m */
bool operator==(const GeographicalCoord& a, const GeographicalCoord& b);

// Used to modelize the shapes of the different objects
typedef std::vector<GeographicalCoord> LineString;
using MultiLineString = std::vector<LineString>;

// Project a point on a line.  The returned point is the nearest point on the line.
GeographicalCoord project(const LineString&, const GeographicalCoord&);

// Project a point on a multiline.  The returned point is the nearest point on the multiline.
GeographicalCoord project(const MultiLineString&, const GeographicalCoord&);

// Split a LineString at a certain point which is on the LineString. Return the preceding / following LineString.
LineString split_line_at_point(const LineString&, const GeographicalCoord&, bool end_of_geom = false);

double real_distance_from_extremity(const LineString&, const GeographicalCoord&, bool);

}  // namespace type
}  // namespace navitia

// Registering GeographicalCoord as a boost::geometry::point
BOOST_GEOMETRY_REGISTER_POINT_2D_GET_SET(navitia::type::GeographicalCoord,
                                         double,
                                         boost::geometry::cs::cartesian,
                                         lon,
                                         lat,
                                         set_lon,
                                         set_lat)
BOOST_GEOMETRY_REGISTER_LINESTRING(navitia::type::LineString)
BOOST_GEOMETRY_REGISTER_MULTI_LINESTRING(navitia::type::MultiLineString)

namespace navitia {
namespace type {
using Polygon = boost::geometry::model::polygon<GeographicalCoord>;
using MultiPolygon = boost::geometry::model::multi_polygon<Polygon>;
}  // namespace type
}  // namespace navitia

namespace std {
template <>
struct hash<navitia::type::GeographicalCoord> {
    std::size_t operator()(const navitia::type::GeographicalCoord& coord) const {
        std::size_t seed = 0;
        boost::hash_combine(seed, coord.lon());
        boost::hash_combine(seed, coord.lat());
        return seed;
    }
};
}  // namespace std

namespace boost {
namespace serialization {
template <class Archive>
void serialize(Archive& ar, navitia::type::Polygon& poly, const unsigned int) {
    ar& boost::serialization::make_nvp("outer", poly.outer());
    ar& boost::serialization::make_nvp("inners", poly.inners());
}
template <class Archive>
void serialize(Archive& ar, navitia::type::MultiPolygon& multipoly, const unsigned int) {
    std::vector<navitia::type::Polygon>& impl = multipoly;
    ar& boost::serialization::make_nvp("polygon", impl);
}
template <class Archive>
void serialize(Archive& ar, boost::geometry::model::ring<navitia::type::GeographicalCoord>& ring, const unsigned int) {
    std::vector<navitia::type::GeographicalCoord>& impl = ring;
    ar& boost::serialization::make_nvp("ring", impl);
}
}  // namespace serialization
}  // namespace boost

navitia::type::GeographicalCoord in_the_right_interval(double lon, double lat);

namespace boost {
namespace geometry {
namespace model {

std::ostream& operator<<(std::ostream& os, const navitia::type::Polygon& points);

std::ostream& operator<<(std::ostream& os, const navitia::type::MultiPolygon& polygons);

}  // namespace model
}  // namespace geometry
}  // namespace boost
