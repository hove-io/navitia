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
#include "conf.h"

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>

#include <string>

namespace navitia {
namespace type {

struct MetaData {
    boost::gregorian::date_period production_date;
    boost::posix_time::ptime publication_date;

    std::string shape;

    std::string publisher_name;
    std::string publisher_url;
    std::string license;
    std::string instance_name;
    boost::posix_time::ptime dataset_created_at;
    std::string poi_source;
    std::string street_network_source;

    MetaData() : production_date(boost::gregorian::date(), boost::gregorian::date()) {}

    /** Fonction qui permet de sérialiser (aka binariser la structure de données
     *
     * Elle est appelée par boost et pas directement
     */
    template <class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar& production_date& publication_date& shape& publisher_name& publisher_url& license& instance_name&
            dataset_created_at& poi_source& street_network_source;
    }
    boost::posix_time::time_period production_period() const {
        namespace pt = boost::posix_time;
        const auto begin = pt::ptime(production_date.begin(), pt::minutes(0));
        const auto end = pt::ptime(production_date.end(), pt::minutes(0));
        return {begin, end};
    }
};
}  // namespace type
}  // namespace navitia
