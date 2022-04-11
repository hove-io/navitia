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

#include "type/datetime.h"
#include "type/calendar.h"
#include "type/network.h"
#include "type/base_pt_objects.h"
#include "type/commercial_mode.h"
#include "type/meta_vehicle_journey.h"

#include <set>

namespace navitia {
namespace vptranslator {

struct BlockPattern {
    navitia::type::WeekPattern week;
    std::set<boost::gregorian::date_period> validity_periods;
    std::set<navitia::type::ExceptionDate> exceptions;

    // Simplifies validity_period by concatenating adjacent date_period
    void canonize_validity_periods();

    // Returns the number of weeks (from Monday to Sunday) intersected
    // by the validity_period
    unsigned nb_weeks() const;

    // Add another block pattern to this one, by adding
    // validity_period, excluding and including.  Throws
    // std::invalid_argument if the given block pattern do not have
    // empty excluding and including.
    void add_from(const BlockPattern&);
};
std::ostream& operator<<(std::ostream&, const BlockPattern&);

// Returns one block describing the given ValidityPattern.
BlockPattern translate_one_block(const navitia::type::ValidityPattern&);

// Returns a vector of block patterns sorted by begining of the
// validity_period describing the given ValidityPattern.  The blocks
// have empty including and excluding.
std::vector<BlockPattern> translate_no_exception(const navitia::type::ValidityPattern&);

// Returns a vector of block patterns sorted by begining of the
// validity_period describing the given ValidityPattern.
std::vector<BlockPattern> translate(const navitia::type::ValidityPattern&);

}  // namespace vptranslator
}  // namespace navitia
