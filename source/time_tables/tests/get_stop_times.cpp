#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_ed
#include <boost/test/unit_test.hpp>
#include "time_tables/get_stop_times.h"
#include "ed/build_helper.h"
using namespace navitia::timetables;

BOOST_AUTO_TEST_CASE(test1){
    ed::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 8100,8150);
    b.data->pt_data->index();
    b.data->build_raptor();

    std::vector<navitia::type::idx_t> rps;
    for(auto jpp : b.data->pt_data->journey_pattern_points)
        rps.push_back(jpp->idx);
    auto result = get_stop_times(rps, navitia::DateTimeUtils::min, navitia::DateTimeUtils::inf, 1, *b.data, false);
    BOOST_REQUIRE_EQUAL(result.size(), 1);

}
