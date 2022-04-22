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

#include "geographical_coord.h"

#include "type/serialization.h"

#include <iomanip>

namespace navitia {
namespace type {

template <class Archive>
void GeographicalCoord::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& _lon& _lat;
}
SERIALIZABLE(GeographicalCoord)

double GeographicalCoord::distance_to(const GeographicalCoord& other) const {
    double longitudeArc = (this->lon() - other.lon()) * N_DEG_TO_RAD;
    double latitudeArc = (this->lat() - other.lat()) * N_DEG_TO_RAD;
    double latitudeH = sin(latitudeArc * 0.5);
    latitudeH *= latitudeH;
    double lontitudeH = sin(longitudeArc * 0.5);
    lontitudeH *= lontitudeH;
    double tmp = cos(this->lat() * N_DEG_TO_RAD) * cos(other.lat() * N_DEG_TO_RAD);
    return EARTH_RADIUS_IN_METERS * 2.0 * asin(sqrt(latitudeH + tmp * lontitudeH));
}

bool operator==(const GeographicalCoord& a, const GeographicalCoord& b) {
    return a.distance_to(b) < 0.1;  // soit 0.1m
}

template <typename F>
std::pair<GeographicalCoord, float> GeographicalCoord::project_common(const GeographicalCoord& segment_start,
                                                                      const GeographicalCoord& segment_end,
                                                                      F f) const {
    std::pair<GeographicalCoord, float> result;
    double dlon = segment_end._lon - segment_start._lon;
    double dlat = segment_end._lat - segment_start._lat;
    double length_sqr = dlon * dlon + dlat * dlat;
    double u;
    // On gère le cas où le segment est particulièrement court, et donc ça peut poser des problèmes (à cause de la
    // division par length²)
    if (length_sqr < 1e-11) {  // moins de un mètre, on projette sur une extrémité
        if (f(*this, segment_start) < f(*this, segment_end)) {
            u = 0;
        } else {
            u = 1;
        }
    } else {
        u = ((this->_lon - segment_start._lon) * dlon + (this->_lat - segment_start._lat) * dlat) / length_sqr;
    }

    // Les deux cas où le projeté tombe en dehors
    if (u < 0) {
        result = std::make_pair(segment_start, f(*this, segment_start));
    } else if (u > 1) {
        result = std::make_pair(segment_end, f(*this, segment_end));
    } else {
        result.first._lon = segment_start._lon + u * dlon;
        result.first._lat = segment_start._lat + u * dlat;
        result.second = f(*this, result.first);
    }

    return result;
}

std::pair<GeographicalCoord, float> GeographicalCoord::project(const GeographicalCoord& segment_start,
                                                               const GeographicalCoord& segment_end) const {
    auto dist = [](const GeographicalCoord& source, const GeographicalCoord& target) {
        return source.distance_to(target);
    };
    return project_common(segment_start, segment_end, dist);
}

std::pair<GeographicalCoord, float> GeographicalCoord::approx_project(const GeographicalCoord& segment_start,
                                                                      const GeographicalCoord& segment_end,
                                                                      double coslat) const {
    auto dist = [coslat](const GeographicalCoord& source, const GeographicalCoord& target) {
        return sqrt(source.approx_sqr_distance(target, coslat));
    };
    return project_common(segment_start, segment_end, dist);
}

std::ostream& operator<<(std::ostream& os, const GeographicalCoord& coord) {
    os << coord.lon() << ";" << coord.lat();
    return os;
}

GeographicalCoord project(const LineString& line, const GeographicalCoord& p) {
    if (line.empty()) {
        return p;
    }

    // project the p on the way
    GeographicalCoord projected = line.front();
    float min_dist = p.distance_to(projected);
    GeographicalCoord prev = line.front();
    auto cur = line.begin();
    for (++cur; cur != line.end(); prev = *cur, ++cur) {
        auto projection = p.project(prev, *cur);
        if (projection.second < min_dist) {
            min_dist = projection.second;
            projected = projection.first;
        }
    }

    return projected;
}

GeographicalCoord project(const MultiLineString& multiline, const GeographicalCoord& p) {
    if (multiline.empty()) {
        return p;
    }

    GeographicalCoord projected;
    double min_dist = std::numeric_limits<double>::infinity();
    for (const auto& line : multiline) {
        const auto projection = project(line, p);
        const auto cur_dist = projection.distance_to(p);
        if (cur_dist < min_dist) {
            min_dist = cur_dist;
            projected = projection;
        }
    }

    return projected;
}

LineString split_line_at_point(const LineString& ls, const GeographicalCoord& blade, bool end_of_geom) {
    LineString result;
    if (ls.size() > 1) {
        for (auto coord = ls.begin(); coord < ls.end() - 1; coord++) {
            /* Check if blade is between the current chunk of geometry
               We have a chunk a---------b, we want to know if c is in the segment
               We compute the distances ab, ac, and cb.
               There are three possibilities:
               - The 3 points form a triangle => ac+bc > ab
               - They are collinear and c is outside the ab segment => ac+bc > ab
               - They are collinear and c is inside the ab segment => ac+bc = ab
            */
            float ab = coord->distance_to(*(coord + 1));
            float ac = blade.distance_to(*coord);
            float bc = blade.distance_to(*(coord + 1));
            if (std::abs(ac + bc - ab) < 0.1f) {
                if (end_of_geom) {
                    result.push_back(blade);
                    result.insert(result.end(), coord + 1, ls.end());
                } else {
                    result.insert(result.begin(), ls.begin(), coord + 1);
                    result.push_back(blade);
                }
                break;
            }
        }
    }
    return result;
}

double real_distance_from_extremity(const LineString& ls, const GeographicalCoord& c, bool distance_from_target) {
    LineString splitted_geom(split_line_at_point(ls, c, distance_from_target));

    double distance(0);
    if (splitted_geom.size() > 1) {
        for (auto it = splitted_geom.begin(); it < splitted_geom.end() - 1; it++) {
            distance += fabs(it->distance_to(*(it + 1)));
        }
    }

    return distance;
}

}  // namespace type
}  // namespace navitia

navitia::type::GeographicalCoord in_the_right_interval(double lon, double lat) {
    if (fabs(lat) > 90) {
        lat = fmod(lat, 360);
        if (fabs(lat) > 90) {
            if (fabs(lat) > 270) {
                lat = (std::signbit(lat) ? 1 : -1) * (90 - fabs(fmod(lat, 90)));
            } else {
                lon = -lon;
                if (fabs(lat) > 180) {
                    lat = (std::signbit(lat) ? 1 : -1) * fabs(fmod(lat, 90));
                } else {
                    lat = (std::signbit(lat) ? -1 : 1) * (90 - fabs(fmod(lat, 90)));
                }
            }
        }
    }
    if (fabs(lon) > 180) {
        lon = fmod(lon, 360);
        if (fabs(lon) > 180) {
            lon = (std::signbit(lon) ? 1 : -1) * (180 - fabs(fmod(lon, 180)));
        }
    }
    return navitia::type::GeographicalCoord(lon, lat);
}

namespace boost {
namespace geometry {
namespace model {

struct GeoCoord {
    const navitia::type::GeographicalCoord& coord;
    explicit GeoCoord(const navitia::type::GeographicalCoord& coord) : coord(coord) {}
};

static std::ostream& operator<<(std::ostream& os, const GeoCoord& coord) {
    return os << std::setprecision(16) << "[" << coord.coord.lon() << "," << coord.coord.lat() << "]";
}

std::ostream& operator<<(std::ostream& os, const navitia::type::Polygon& points) {
    os << R"({"type":"Polygon","coordinates":[[)";
    os << GeoCoord(points.outer()[0]) << ",";
    for (unsigned j = 1; j < points.outer().size(); j++) {
        os << GeoCoord(points.outer()[j]);
        if (j == points.outer().size() - 1) {
            continue;
        }
        os << ",";
    }
    os << "]";
    for (const auto& k : points.inners()) {
        os << ",[";
        for (unsigned l = 0; l < k.size(); l++) {
            os << GeoCoord(k[l]);
            if (l == k.size() - 1) {
                continue;
            }
            os << ",";
        }
        os << "]";
    }
    return os << "]}";
}

std::ostream& operator<<(std::ostream& os, const navitia::type::MultiPolygon& polygons) {
    os << R"({"type":"MultiPolygon","coordinates":[)";
    for (unsigned i = 0; i < polygons.size(); i++) {
        os << "[[" << GeoCoord(polygons[i].outer()[0]);
        for (unsigned j = 1; j < polygons[i].outer().size(); j++) {
            os << GeoCoord(polygons[i].outer()[j]);
            if (j == polygons[i].outer().size() - 1) {
                continue;
            }
            os << ",";
        }
        os << "]";
        for (const auto& k : polygons[i].inners()) {
            os << ",[";
            for (unsigned l = 0; l < k.size(); l++) {
                os << GeoCoord(k[l]);
                if (l == k.size() - 1) {
                    continue;
                }
                os << ",";
            }
            os << "]";
        }
        if (i == polygons.size() - 1) {
            continue;
        }
        os << ",";
    }
    return os << "]}";
}
}  // namespace model
}  // namespace geometry
}  // namespace boost
