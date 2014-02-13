#include "ed/connectors/conv_coord.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_convcoord
#include <boost/test/unit_test.hpp>


class CoordParams{
public:
    ed::connectors::Projection lambert2;
    ed::connectors::Projection wgs84;
    CoordParams(){
        this->lambert2 = ed::connectors::Projection("Lambert 2 étendu", "27572", false);
        this->wgs84 = ed::connectors::Projection("wgs84", "4326", true);
    }
};

BOOST_FIXTURE_TEST_CASE(lambertII2Wgs84, CoordParams) {

    navitia::type::GeographicalCoord coord_lambert2(537482.27, 2381791.96);
    ed::connectors::ConvCoord conv_coord(lambert2, wgs84);
    navitia::type::GeographicalCoord coord_wgs84 = conv_coord.convert_to(coord_lambert2);

    BOOST_REQUIRE_EQUAL(coord_wgs84 == navitia::type::GeographicalCoord(1.491911486572199, 48.431961616400599), true);
}

BOOST_FIXTURE_TEST_CASE(Wgs842lambertII, CoordParams) {

    navitia::type::GeographicalCoord coord_wgs84(1.491911486572199, 48.431961616400599);
    ed::connectors::ConvCoord conv_coord(wgs84, lambert2);
    navitia::type::GeographicalCoord coord_lambert2 = conv_coord.convert_to(coord_wgs84);

    BOOST_REQUIRE_EQUAL((coord_lambert2.lon() - 537482.27) <0.1, true);
    BOOST_REQUIRE_EQUAL((coord_lambert2.lon() - 2381791.96) <0.1, true);
}
