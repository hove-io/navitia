/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
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

#include "conv_coord.h"
#include "utils/exception.h"

namespace ed {
namespace connectors {

Projection::Projection(const std::string& name, const std::string& num_epsg, bool is_degree) {
#ifdef PROJ_API_VERSION_MAJOR_6
    this->definition = "EPSG:" + num_epsg;
#else
    this->definition = "+init=epsg:" + num_epsg;
#endif
    this->name = name;
    this->is_degree = is_degree;
    this->custom_pj_init_plus();
}
void Projection::custom_pj_init_plus() {
#ifdef PROJ_API_VERSION_MAJOR_4
    proj_pj = pj_init_plus(definition.c_str());
    if (!proj_pj) {
        throw navitia::exception("invalid projection system: definition: " + definition);
    }
#endif
}
void Projection::custom_pj_free() {
#ifdef PROJ_API_VERSION_MAJOR_4
    pj_free(proj_pj);
#endif
}
Projection::Projection(const Projection& other) {
    name = other.name;
    is_degree = other.is_degree;
    definition = other.definition;
    // we allocate a new proj
    this->custom_pj_init_plus();
}
Projection& Projection::operator=(const Projection& other) {
    name = other.name;
    is_degree = other.is_degree;
    definition = other.definition;
    // we allocate a new proj
    this->custom_pj_init_plus();
    return *this;
}
Projection& Projection::operator=(Projection&& other) {
    this->custom_pj_free();

    name = other.name;
    is_degree = other.is_degree;
    definition = other.definition;
// we allocate a new proj
#ifdef PROJ_API_VERSION_MAJOR_4
    proj_pj = other.proj_pj;
    other.proj_pj = nullptr;
#endif
    return *this;
}

Projection::Projection(Projection&& other)
    : name(other.name), definition(other.definition), is_degree(other.is_degree) {
// we got the proj4 ptr, no need for another allocation
#ifdef PROJ_API_VERSION_MAJOR_4
    proj_pj = other.proj_pj;
    other.proj_pj = nullptr;
#endif
}
Projection::~Projection() {
    this->custom_pj_free();
}

#ifdef PROJ_API_VERSION_MAJOR_4
navitia::type::GeographicalCoord ConvCoord::proj_lib4_convert_to(navitia::type::GeographicalCoord coord) const {
    if (this->origin.is_degree) {
        coord.set_lon(coord.lon() * DEG_TO_RAD);
        coord.set_lat(coord.lat() * DEG_TO_RAD);
    }
    double x = coord.lon();
    double y = coord.lat();
    pj_transform(this->origin.proj_pj, this->destination.proj_pj, 1, 1, &x, &y, nullptr);
    coord.set_lon(x);
    coord.set_lat(y);
    if (this->destination.is_degree) {
        coord.set_lon(coord.lon() * RAD_TO_DEG);
        coord.set_lat(coord.lat() * RAD_TO_DEG);
    }
    return coord;
}
#endif
#ifdef PROJ_API_VERSION_MAJOR_6
navitia::type::GeographicalCoord ConvCoord::proj_lib6_convert_to(navitia::type::GeographicalCoord coord) const {
    PJ_COORD tmp_coord = proj_coord(coord.lon(), coord.lat(), .0, HUGE_VAL);
    PJ_COORD out_coord = proj_trans(this->p_for_gis, PJ_FWD, tmp_coord);
    coord.set_lon(out_coord.xy.x);
    coord.set_lat(out_coord.xy.y);
    return coord;
}
#endif
navitia::type::GeographicalCoord ConvCoord::convert_to(navitia::type::GeographicalCoord coord) const {
    if (this->origin.definition == this->destination.definition) {
        return coord;
    }
#ifdef PROJ_API_VERSION_MAJOR_6
    return this->proj_lib6_convert_to(coord);
#else
    return this->proj_lib4_convert_to(coord);
#endif
}
void ConvCoord::init_proj_for_gis() {
#ifdef PROJ_API_VERSION_MAJOR_6
    PJ* prj = proj_create_crs_to_crs(0, this->origin.definition.c_str(), this->destination.definition.c_str(), 0);
    if (!prj) {
        throw navitia::exception("invalid projection system");
    }
    this->p_for_gis = proj_normalize_for_visualization(PJ_DEFAULT_CTX, prj);
    if (this->p_for_gis) {
        proj_destroy(prj);
    }
#endif
}

ConvCoord::ConvCoord(const ConvCoord& other) {
    this->destination = other.destination;
    this->origin = other.origin;
#ifdef PROJ_API_VERSION_MAJOR_6
    init_proj_for_gis();
#endif
}
ConvCoord& ConvCoord::operator=(const ConvCoord& other) {
    this->destination = other.destination;
    this->origin = other.origin;
#ifdef PROJ_API_VERSION_MAJOR_6
    init_proj_for_gis();
#endif
    return *this;
}
ConvCoord::ConvCoord(Projection origin, Projection destination)
    : origin(std::move(origin)), destination(std::move(destination)) {
#ifdef PROJ_API_VERSION_MAJOR_6
    init_proj_for_gis();
#endif
}
ConvCoord::~ConvCoord() {
#ifdef PROJ_API_VERSION_MAJOR_6
    proj_destroy(this->p_for_gis);
#endif
}
}  // namespace connectors
}  // namespace ed
