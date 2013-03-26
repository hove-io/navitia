#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_navimake
#include <boost/test/unit_test.hpp>
#include "time_tables/get_stop_times.h"
#include "naviMake/build_helper.h"
using namespace navitia::timetables;

BOOST_AUTO_TEST_CASE(test1){
    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000, 8050)("stop2", 8100,8150);
    navitia::type::Data data;
    b.build(data.pt_data);
    data.build_raptor();

    navitia::type::PT_Data & d = data.pt_data;

    std::vector<navitia::type::idx_t> rps;
    for(auto rp : d.journey_pattern_points)
        rps.push_back(rp.idx);
    auto result = get_stop_times(rps, navitia::type::DateTime(0,0), navitia::type::DateTime(), 1, data);
    BOOST_REQUIRE_EQUAL(result.size(), 1);

}
