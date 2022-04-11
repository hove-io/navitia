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
#include <utility>

#include "fare/fare.h"
#include "ed/data.h"

namespace ed {
namespace connectors {

/**
 * Parser for fare files
 *
 * There are 3 fare files:
 *  - price.csv: description of all the different prices
 *  - fares.csv: description of the transition for the state machine
 *  - od_fares.csv: description of the fare by od (not mandatory)
 */
struct fare_parser {
    Data& data;

    const std::string state_transition_filename;
    const std::string prices_filename;
    const std::string od_filename;

    fare_parser(Data& data_, std::string state, std::string prices, const std::string od)
        : data(data_),
          state_transition_filename(std::move(state)),
          prices_filename(std::move(prices)),
          od_filename(od) {}

    void load();

private:
    void load_transitions();

    void load_prices();

    void load_od();

    bool is_valid(const navitia::fare::State&);
    bool is_valid(const navitia::fare::Condition&);

    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
};

}  // namespace connectors
}  // namespace ed
