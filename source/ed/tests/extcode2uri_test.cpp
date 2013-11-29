#include "ed/connectors/extcode2uri.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_extcode2uri
#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_CASE(required_files) {
    ed::connectors::ExtCode2Uri connection("host=localhost db=0 password=navitia port=6379");
    connection.connected();
    connection.set("bb", "abderrahim");
    BOOST_CHECK(true);

}
