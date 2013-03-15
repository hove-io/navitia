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


BOOST_AUTO_TEST_CASE(impact_vj_0){
    navimake::builder b("201303011T1739");
    b.generate_dummy_basis();
    b.vj("A", "1111111", "", true, "vj1")("stop1", 8000,8050)("stop2", 8200,8250);
    b.vj("A", "1110011", "", true, "vj2")("stop1", 9000,9050)("stop2", 9200,9250);

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

    VehicleJourney* vj = b.data.vehicle_journeys[0];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1");
    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));

    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1111110"));

    vj = b.data.vehicle_journeys[1];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj2");
    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1110011"));

    BOOST_CHECK(vj->adapted_validity_pattern == NULL);
    BOOST_CHECK_EQUAL(b.data.vehicle_journeys.size(), 2);
}

BOOST_AUTO_TEST_CASE(impact_vj_1){
    navimake::builder b("201303011T1739");
    b.generate_dummy_basis();
    b.vj("A", "1111111", "", true, "vj1")("stop1", "8:50","9:00")("stop2", "11:00", "11:02");
    b.vj("A", "1110011", "", true, "vj2")("stop1", "10:50","11:00")("stop2","13:00","13:02");

    b.data.normalize_uri();

    std::map<std::string, std::vector<nt::Message>> messages;
    nt::Message m;
    m.object_type = nt::Type_e::VehicleJourney;
    m.object_uri = "vehicle_journey:vj1";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 00:00:00"), pt::time_from_string("2013-03-03 23:59:00"));
    m.active_days = nt::Lun | nt::Mar | nt::Mer | nt::Jeu | nt::Ven | nt::Sam | nt::Dim;
    m.application_daily_start_hour = pt::duration_from_string("00:00");
    m.application_daily_end_hour = pt::duration_from_string("23:59");

    messages[m.object_uri].push_back(m);

    AtAdaptedLoader loader;
    loader.apply(messages, b.data);

    VehicleJourney* vj = b.data.vehicle_journeys[0];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1");
    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));

    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1111000"));

    vj = b.data.vehicle_journeys[1];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj2");
    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1110011"));

    BOOST_CHECK(vj->adapted_validity_pattern == NULL);
    BOOST_CHECK_EQUAL(b.data.vehicle_journeys.size(), 2);
}

BOOST_AUTO_TEST_CASE(impact_vj_2){
    navimake::builder b("201303011T1739");
    b.generate_dummy_basis();
    b.vj("A", "1111111", "", true, "vj1")("stop1", "8:50","9:00")("stop2", "11:00", "11:02");
    b.vj("A", "1110011", "", true, "vj2")("stop1", "10:50","11:00")("stop2","13:00","13:02");

    b.data.normalize_uri();

    std::map<std::string, std::vector<nt::Message>> messages;
    nt::Message m;
    m.object_type = nt::Type_e::VehicleJourney;
    m.object_uri = "vehicle_journey:vj1";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 10:00:00"), pt::time_from_string("2013-03-03 23:59:00"));
    m.active_days = nt::Lun | nt::Mar | nt::Mer | nt::Jeu | nt::Ven | nt::Sam | nt::Dim;
    m.application_daily_start_hour = pt::duration_from_string("00:00");
    m.application_daily_end_hour = pt::duration_from_string("09:30");

    messages[m.object_uri].push_back(m);


    m.object_uri = "vehicle_journey:vj2";
    messages[m.object_uri].push_back(m);

    AtAdaptedLoader loader;
    loader.apply(messages, b.data);

    VehicleJourney* vj = b.data.vehicle_journeys[0];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1");
    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));

    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1111001"));

    vj = b.data.vehicle_journeys[1];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj2");
    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1110011"));
    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1110011"));

    BOOST_CHECK_EQUAL(b.data.vehicle_journeys.size(), 2);

}

BOOST_AUTO_TEST_CASE(impact_line_0){
    navimake::builder b("201303011T1739");
    b.generate_dummy_basis();
    b.vj("A", "1111111", "", true, "vj1")("stop1", "8:50","9:00")("stop2", "11:00", "11:02");
    b.vj("A", "1110011", "", true, "vj2")("stop1", "10:50","11:00")("stop2","13:00","13:02");
    b.vj("B", "1111111", "", true, "vj3")("stop1", "10:50","11:00")("stop2","13:00","13:02");

    b.data.normalize_uri();

    std::map<std::string, std::vector<nt::Message>> messages;
    nt::Message m;
    m.object_type = nt::Type_e::Line;
    m.object_uri = "line:A";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 00:00:00"), pt::time_from_string("2013-03-03 23:59:00"));
    m.active_days = nt::Lun | nt::Mar | nt::Mer | nt::Jeu | nt::Ven | nt::Sam | nt::Dim;
    m.application_daily_start_hour = pt::duration_from_string("00:00");
    m.application_daily_end_hour = pt::duration_from_string("23:59");

    messages[m.object_uri].push_back(m);

    AtAdaptedLoader loader;
    loader.apply(messages, b.data);

    VehicleJourney* vj = b.data.vehicle_journeys[0];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1");
    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));
    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1111000"));

    vj = b.data.vehicle_journeys[1];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj2");
    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1110011"));
    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1110000"));

    vj = b.data.vehicle_journeys[2];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj3");
    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));
    BOOST_CHECK(vj->adapted_validity_pattern == NULL);

    BOOST_CHECK_EQUAL(b.data.vehicle_journeys.size(), 3);
}
