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

#include "type/dataset.h"
#include "type/network.h"
#include "type/indexes.h"
#include "type/line.h"
#include "type/pt_data.h"
#include "type/serialization.h"

#include <boost/serialization/set.hpp>

namespace navitia {
namespace type {

template <class Archive>
void Network::serialize(Archive& ar, const unsigned int /*unused*/) {
    ar& idx& name& uri& address_name& address_number& address_type_name& mail& website& fax& sort& line_list& impacts&
        dataset_list;
}
SERIALIZABLE(Network)

bool Network::operator<(const Network& other) const {
    if (this->sort != other.sort) {
        return this->sort < other.sort;
    }
    if (this->name != other.name) {
        return this->name < other.name;
    }
    return this->uri < other.uri;
}

Indexes Network::get(Type_e type, const PT_Data& data) const {
    Indexes result;
    switch (type) {
        case Type_e::Line:
            return indexes(line_list);
        case Type_e::Impact:
            return data.get_impacts_idx(get_impacts());
        case Type_e::Dataset:
            return indexes(dataset_list);
        default:
            break;
    }
    return result;
}
}  // namespace type
}  // namespace navitia
