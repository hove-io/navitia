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
#include <bitset>

namespace navitia {
namespace type {

struct hasOdtProperties {
    using OdtProperties = std::bitset<2>;
    static const uint8_t ESTIMATED_ODT = 0;
    static const uint8_t ZONAL_ODT = 1;
    OdtProperties odt_properties;

    hasOdtProperties() { odt_properties.reset(); }

    void operator|=(const type::hasOdtProperties& other) { odt_properties |= other.odt_properties; }

    void reset() { odt_properties.reset(); }
    void set_estimated(const bool val = true) { odt_properties.set(ESTIMATED_ODT, val); }
    void set_zonal(const bool val = true) { odt_properties.set(ZONAL_ODT, val); }

    bool is_scheduled() const { return odt_properties.none(); }
    bool is_with_stops() const { return !odt_properties[ZONAL_ODT]; }
    bool is_estimated() const { return odt_properties[ESTIMATED_ODT]; }
    bool is_zonal() const { return odt_properties[ZONAL_ODT]; }

    template <class Archive>
    void serialize(Archive& ar, const unsigned int);
};
}  // namespace type
}  // namespace navitia
