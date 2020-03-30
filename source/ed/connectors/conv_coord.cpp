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
    definition = "+init=epsg:" + num_epsg;
    proj_pj = pj_init_plus(definition.c_str());
    if (!proj_pj) {
        throw navitia::exception("invalid projection system: " + name + " definition: " + definition);
    }
    this->name = name;
    this->is_degree = is_degree;
}

Projection::Projection(const Projection& other) {
    name = other.name;
    is_degree = other.is_degree;
    definition = other.definition;
    // we allocate a new proj
    proj_pj = pj_init_plus(definition.c_str());
}

Projection& Projection::operator=(const Projection& other) {
    name = other.name;
    is_degree = other.is_degree;
    definition = other.definition;
    // we allocate a new proj
    proj_pj = pj_init_plus(definition.c_str());
    return *this;
}

Projection& Projection::operator=(Projection&& other) noexcept {
    pj_free(proj_pj);

    name = other.name;
    is_degree = other.is_degree;
    definition = other.definition;
    // we allocate a new proj
    proj_pj = other.proj_pj;
    other.proj_pj = nullptr;
    return *this;
}

Projection::Projection(Projection&& other) noexcept
    : name(other.name), definition(other.definition), is_degree(other.is_degree), proj_pj(other.proj_pj) {
    // we got the proj4 ptr, no need for another allocation
    other.proj_pj = nullptr;
}

Projection::~Projection() {
    pj_free(this->proj_pj);
}

navitia::type::GeographicalCoord ConvCoord::convert_to(navitia::type::GeographicalCoord coord) const {
    if (this->origin.definition == this->destination.definition) {
        return coord;
    }
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

}  // namespace connectors
}  // namespace ed
