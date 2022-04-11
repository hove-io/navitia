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

#include "utils/exception.h"

namespace navitia {
namespace data {

// Wrong data version
struct wrong_version : public navitia::recoverable_exception {
    wrong_version(const std::string& msg) : navitia::recoverable_exception(msg) {}
    wrong_version(const wrong_version&) = default;
    wrong_version& operator=(const wrong_version&) = default;
    ~wrong_version() noexcept override;
};

// Data loading exceptions handler
struct data_loading_error : public navitia::recoverable_exception {
    data_loading_error(const std::string& msg) : navitia::recoverable_exception(msg) {}
    data_loading_error(const data_loading_error&) = default;
    data_loading_error& operator=(const data_loading_error&) = default;
    ~data_loading_error() noexcept override;
};

// Disruptions exceptions handler. Broken connection
struct disruptions_broken_connection : public navitia::recoverable_exception {
    disruptions_broken_connection(const std::string& msg) : navitia::recoverable_exception(msg) {}
    disruptions_broken_connection(const disruptions_broken_connection&) = default;
    disruptions_broken_connection& operator=(const disruptions_broken_connection&) = default;
    ~disruptions_broken_connection() noexcept override;
};

// Disruptions exceptions handler. Loading error
struct disruptions_loading_error : public navitia::recoverable_exception {
    disruptions_loading_error(const std::string& msg) : navitia::recoverable_exception(msg) {}
    disruptions_loading_error(const disruptions_loading_error&) = default;
    disruptions_loading_error& operator=(const disruptions_loading_error&) = default;
    ~disruptions_loading_error() noexcept override;
};

// Raptor building exceptions handler
struct raptor_building_error : public navitia::recoverable_exception {
    raptor_building_error(const std::string& msg) : navitia::recoverable_exception(msg) {}
    raptor_building_error(const raptor_building_error&) = default;
    raptor_building_error& operator=(const raptor_building_error&) = default;
    ~raptor_building_error() noexcept override;
};

}  // namespace data
}  // namespace navitia
