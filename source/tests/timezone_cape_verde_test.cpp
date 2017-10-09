/* Copyright © 2001-2017, Canal TP and/or its affiliates. All rights reserved.

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
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "utils/init.h"
#include "ed/build_helper.h"
#include "kraken/apply_disruption.h"
#include "mock_kraken.h"
#include "tests/utils_test.h"

namespace bg = boost::gregorian;
using btp = boost::posix_time::time_period;

int main(int argc, const char* const argv[]) {
    navitia::init_app();

    const auto date = "20170101";
    const auto begin = boost::gregorian::date_from_iso_string(date);
    const boost::gregorian::date_period production_date = {begin, begin + boost::gregorian::years(1)};
    navitia::type::TimeZoneHandler::dst_periods timezones = {{-3600, {production_date}}};
    ed::builder b("20170101", "canal tp", "Atlantic/Cape_Verde", timezones);

    b.vj("line:1", "000010")
        .uri("vj:1:1")
        ("A", "23:00"_t)
        ("B", "23:30"_t)
        ("C", "24:00"_t)
        ("D", "24:30"_t)
        ("E", "25:00"_t)
        ("F", "25:30"_t)
        ("G", "26:00"_t);

    b.vj("line:1", "000100")
        .uri("vj:1:2")
        ("C", "00:00"_t)
        ("D", "00:30"_t)
        ("E", "01:00"_t)
        ("F", "01:30"_t)
        ("G", "02:00"_t);

    b.generate_dummy_basis();
    b.finish();
    b.data->pt_data->index();
    b.data->build_raptor();
    b.data->build_uri();
    b.build_autocomplete();
    b.data->meta->production_date = bg::date_period(bg::date(2017, 1, 1), bg::days(30));

    mock_kraken kraken(b, "timezone_cape_verde_test", argc, argv);

    return 0;
}
