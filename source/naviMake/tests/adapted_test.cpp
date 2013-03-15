#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_adapted
#include "naviMake/data.h"
#include <boost/test/unit_test.hpp>
#include <string>
#include "config.h"
#include "naviMake/build_helper.h"
#include "naviMake/adapted.h"

namespace pt = boost::posix_time;

using namespace navimake;
using namespace navimake::types;
namespace nt = navitia::type;

namespace navimake{ namespace types{

    std::ostream& operator<<(std::ostream& cout, const ValidityPattern& vp){
        cout << "[" << vp.beginning_date << " / " << vp.days << "]";
        return cout;
    }
}}


BOOST_AUTO_TEST_CASE(imapct_vj){
    navimake::builder b("201303011T1739");
    b.generate_dummy_basis();
    b.vj("A", "1111111", "", true, "vj1")("stop1", 8000,8050)("stop2", 8200,8250);
    //b.vj("B")("stop3", 9000,9050)("stop4", 9200,9250);

    b.data.normalize_uri();

    std::map<std::string, std::vector<nt::Message>> messages;
    nt::Message m;
    m.object_type = nt::Type_e::VehicleJourney;
    m.object_uri = "vehicle_journey:vj1";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 00:00:00"), pt::time_from_string("2013-03-03 23:59:00"));
    m.active_days = nt::Ven;
    m.application_daily_start_hour = pt::duration_from_string("00:00");
    m.application_daily_end_hour = pt::duration_from_string("23:59");

    messages[m.object_uri].push_back(m);

    AtAdaptedLoader loader;
    loader.apply(messages, b.data);

    BOOST_CHECK_EQUAL(*b.data.vehicle_journeys.front()->validity_pattern,  ValidityPattern(b.begin, "1111111"));

    BOOST_CHECK_EQUAL(*b.data.vehicle_journeys.front()->adapted_validity_pattern,  ValidityPattern(b.begin, "1111110"));




}

