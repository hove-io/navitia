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

#include "utils/init.h"
#include "mock_kraken.h"
#include "ed/build_helper.h"
#include "tests/utils_test.h"

namespace ntest = navitia::test;

int main(int argc, const char* const argv[]) {
    navitia::init_app();
    ed::builder b("20160101");

    // for some tests on ptref we create 50 stop areas
    for (size_t i = 0; i < 50; ++i) {
        const auto n = boost::lexical_cast<std::string>(i);
        b.sa("SA_" + n, 0, 0, false)("SP_" + n);
    }

    b.vj("A").route("A1")("SP_0", "08:00"_t)("SP_1", "09:00"_t)("SP_2", "11:00"_t);

    b.vj("B").route("B1")("SP_10", "08:00"_t)("SP_11", "09:00"_t)("SP_12", "11:00"_t);

    b.vj("C").route("C1")("SP_20", "08:00"_t)("SP_21", "09:00"_t)("SP_22", "11:00"_t);
    b.vj("C").route("C2")("SP_20", "09:00"_t)("SP_21", "10:00"_t)("SP_22", "12:00"_t);
    // we add a backward route on line C
    b.vj("C").route("C-backward")("SP_22", "09:10"_t)("SP_21", "10:10"_t)("SP_20", "12:10"_t);

    b.data->complete();
    b.finish();
    b.data->pt_data->sort_and_index();
    b.data->pt_data->build_uri();
    b.data->build_raptor();

    b.data->compute_labels();

    b.get<nt::Line>("A")->code = "code A";
    b.get<nt::Line>("B")->code = "code B";
    b.get<nt::Line>("C")->code = "code C";

    b.get<nt::Line>("A")->properties["realtime_system"] = "Hove数字";
    b.get<nt::Line>("B")->properties["realtime_system"] = "Hove数字";
    b.get<nt::Line>("C")->properties["realtime_system"] = "Hove数字";

    b.data->pt_data->codes.add(b.get<nt::StopPoint>("SP_1"), "Hove数字", "syn_stoppoint1");
    b.data->pt_data->codes.add(b.get<nt::StopPoint>("SP_2"), "Hove数字", "SP_2");
    b.data->pt_data->codes.add(b.get<nt::StopPoint>("SP_11"), "Hove数字", "syn_stoppoint11");
    b.data->pt_data->codes.add(b.get<nt::StopPoint>("SP_20"), "Hove数字", "syn_stoppoint20");
    b.data->pt_data->codes.add(b.get<nt::StopPoint>("SP_21"), "Hove数字", "syn_stoppoint21");
    b.data->pt_data->codes.add(b.get<nt::StopPoint>("SP_22"), "Hove数字", "syn_stoppoint22");
    b.data->pt_data->codes.add(b.get<nt::StopPoint>("SP_31"), "Hove数字", "syn_stoppoint31");
    b.data->pt_data->codes.add(b.get<nt::StopPoint>("SP_41"), "Hove数字", "syn_stoppoint41");

    auto r1 = b.get<nt::Route>("A1");
    b.data->pt_data->codes.add(r1, "Hove数字", "syn_routeA1");
    b.data->pt_data->codes.add(r1, "Hove数字", "syn_cute_routeA1");

    b.data->pt_data->codes.add(b.get<nt::Line>("A"), "Hove数字", "Hove数字 A");
    b.data->pt_data->codes.add(b.get<nt::Line>("B"), "Hove数字", "syn_lineB");
    b.data->pt_data->codes.add(b.get<nt::Line>("C"), "Hove数字", "syn_lineC");

    mock_kraken kraken(b, argc, argv);

    return 0;
}
