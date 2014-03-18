#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE principal_destination_test
#include <boost/test/unit_test.hpp>
#include "type/type.h"
#include "ed/build_helper.h"

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

struct principal_destination_fixture {
    ed::builder b;
    principal_destination_fixture() : b("20140301") {

        b.vj("network:R", "line:A", "1", "", true, "VJA")("stop1", 10 * 3600, 10 * 3600 + 10 * 60)("stop2", 12 * 3600, 12 * 3600 + 10 * 60);
        b.vj("network:R", "line:A", "1", "", true, "VJB")("stop1", 11 * 3600, 11 * 3600 + 10 * 60)("stop2", 14 * 3600, 14 * 3600 + 10 * 60);
        b.vj("network:R", "line:A", "1", "", true, "VJD")("stop1", 12 * 3600, 12 * 3600 + 10 * 60)("stop2", 18 * 3600, 18 * 3600 + 10 * 60)
                                                         ("stop3", 19 * 3600, 19 * 3600 + 10 * 60);
        b.vj("network:R", "line:A", "1", "", true, "VJE")("stop1", 13 * 3600, 13 * 3600 + 10 * 60)("stop2", 20 * 3600, 20 * 3600 + 10 * 60)
                                                         ("stop3", 21 * 3600, 21 * 3600 + 10 * 60);
        b.vj("network:R", "line:A", "1", "", true, "VJF")("stop1", 14 * 3600, 14 * 3600 + 10 * 60)("stop2", 22 * 3600, 22 * 3600 + 10 * 60)
                                                         ("stop3", 22 * 3600, 22 * 3600 + 10 * 60);
        b.vj("network:K", "line:B", "1", "", true, "VJJ");
        b.data.build_uri();
        b.data.build_uri();
        b.data.pt_data.index();
    }
};

BOOST_FIXTURE_TEST_CASE(principal_destination_found, principal_destination_fixture) {
    navitia::type::Route* route = b.data.pt_data.routes_map["line:A:0"];
    BOOST_CHECK_EQUAL(b.data.pt_data.stop_points[route->principal_destination()]->uri, "stop3");
}

BOOST_FIXTURE_TEST_CASE(principal_destination_not_found, principal_destination_fixture) {
    navitia::type::Route* route = b.data.pt_data.routes_map["line:B:1"];
    BOOST_CHECK_EQUAL(route->principal_destination(), navitia::type::invalid_idx);
}
