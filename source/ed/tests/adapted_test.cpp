/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
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

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_adapted
#include "ed/data.h"
#include <boost/test/unit_test.hpp>
#include <string>
#include "conf.h"
#include "ed/build_helper.h"
#include "ed/adapted.h"
#include "ed/types.h"
#include "boost/date_time/gregorian_calendar.hpp"

namespace pt = boost::posix_time;

enum Jours {
    Lun = 0x01,
    Mar = 0x02,
    Mer = 0x04,
    Jeu = 0x08,
    Ven = 0x10,
    Sam = 0x20,
    Dim = 0x40,
    Fer = 0x80
};

using namespace ed;

using namespace navitia::type;
namespace bg = boost::gregorian;
namespace bt = boost::date_time;
namespace navitia{ namespace type{

    std::ostream& operator<<(std::ostream& cout, const ValidityPattern& vp){
        cout << "[" << vp.beginning_date << " / " << vp.days << "]";
        return cout;
    }
}}
//						vendredi 01 mars                                                                    dimanche 03 mars
//                              |	 			            |	       					|      						|
//daily_start_hour              |			              	|---------------------------|---------------------------|
//                              |	       					|///////////////////////////|///////////////////////////|
//application_period            |---------------------------|///////////////////////////|///////////////////////////|
//                              |///////////////////////////|///////////////////////////|///////////////////////////|
//application_period            |///////////////////////////|///////////////////////////|---------------------------|
//                              |///////////////////////////|///////////////////////////|			              	|
//daily_end_hour                |---------------------------|---------------------------|                           |
//                              |					      	|			              	|	    					|


//le vj1 circule tous les jours du vendredi 01 mars au jeudi 07 mars
//le vj2 ne circule pas le lundi 04 mars et le mardi 05 mars

//le message s'applique sur le vj1 le vendredi 01 mars
BOOST_AUTO_TEST_CASE(impact_vj_0){
    ed::builder b("20130301T1739");
    bg::date end_date = bg::date_from_iso_string("20130308T1739");
    b.generate_dummy_basis();
    VehicleJourney* vj = b.vj("A", "", "", true, "vehicle_journey:vj1")("stop1", 8000,8050)("stop2", 8200,8250).vj;
    //construction du validityPattern du vj1: 1111111
    std::bitset<7> validedays;
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = true;
    validedays[navitia::Tuesday] = true;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);

    vj = b.vj("A", "", "", true, "vehicle_journey:vj2")("stop1", 9000,9050)("stop2", 9200,9250).vj;
    //construction du validityPattern du vj2: 1110011 (1er mars est un vendredi)
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = false;
    validedays[navitia::Tuesday] = false;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);


    std::vector<nt::AtPerturbation> messages;
    nt::AtPerturbation m;
    m.object_type = nt::Type_e::VehicleJourney;
    m.object_uri = "vehicle_journey:vj1";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 00:00:00"), pt::time_from_string("2013-03-03 23:59:00"));
    m.active_days = Jours::Ven;
    m.application_daily_start_hour = pt::duration_from_string("00:00");
    m.application_daily_end_hour = pt::duration_from_string("23:59");

    messages.push_back(m);

    AtAdaptedLoader loader;
    loader.apply(messages, *b.data->pt_data);

    vj = b.data->pt_data->vehicle_journeys[0];
    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1");

//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));
    bg::date testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

//    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1111110"));
    //le message s'applique sur le vj1 le vendredi 01 mars
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    vj = b.data->pt_data->vehicle_journeys[1];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj2");
//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1110011"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern, *vj->validity_pattern);
    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
}

//le vj1 circule tous les jours du vendredi 01 mars au jeudi 07 mars
//le vj2 ne circule pas le lundi 04 mars et le mardi 05 mars

//le message s'applique sur le vj1 le vendredi 01 mars, samedi 02 mars et le dimanche 03 mars.
BOOST_AUTO_TEST_CASE(impact_vj_1){
    ed::builder b("20130301T1739");
    bg::date end_date = bg::date_from_iso_string("20130308T1739");
    b.generate_dummy_basis();
    VehicleJourney* vj = b.vj("A", "", "", true, "vehicle_journey:vj1")("stop1", 8000,8050)("stop2", 8200,8250).vj;
    //construction du validityPattern du vj1: 1111111
    std::bitset<7> validedays;
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = true;
    validedays[navitia::Tuesday] = true;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);

    vj = b.vj("A", "", "", true, "vehicle_journey:vj2")("stop1", 9000,9050)("stop2", 9200,9250).vj;
    //construction du validityPattern du vj2: 1110011 (1er mars est un vendredi)
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = false;
    validedays[navitia::Tuesday] = false;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);


    std::vector<nt::AtPerturbation> messages;
    nt::AtPerturbation m;
    m.object_type = nt::Type_e::VehicleJourney;
    m.object_uri = "vehicle_journey:vj1";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 00:00:00"), pt::time_from_string("2013-03-03 23:59:00"));
    m.active_days = Jours::Lun | Jours::Mar | Jours::Mer | Jours::Jeu | Jours::Ven | Jours::Sam | Jours::Dim;
    m.application_daily_start_hour = pt::duration_from_string("00:00");
    m.application_daily_end_hour = pt::duration_from_string("23:59");

    messages.push_back(m);

    AtAdaptedLoader loader;
    loader.apply(messages, *b.data->pt_data);

    vj = b.data->pt_data->vehicle_journeys[0];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1");
//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));
    bg::date testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);


//    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1111000"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);


    vj = b.data->pt_data->vehicle_journeys[1];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj2");
//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1110011"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern, *vj->validity_pattern);

    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
}

//le vj1 circule tous les jours du vendredi 01 mars au jeudi 07 mars
//le vj2 ne circule pas le lundi 04 mars et le mardi 05 mars

//le message s'applique sur le vj1 le samedi 02 mars et le dimanche 03 mars. (pas le vendredi 01 car heures non valides)
//il ne s'applique pas sur le vj2 car heures non valides
BOOST_AUTO_TEST_CASE(impact_vj_2){
    ed::builder b("20130301T1739");
    bg::date end_date = bg::date_from_iso_string("20130308T1739");
    b.generate_dummy_basis();
    VehicleJourney* vj = b.vj("A", "", "", true, "vehicle_journey:vj1")("stop1", "8:50","9:00")("stop2", "11:00", "11:02").vj;
    //construction du validityPattern du vj1: 1111111
    std::bitset<7> validedays;
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = true;
    validedays[navitia::Tuesday] = true;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);

    vj = b.vj("A", "", "", true, "vehicle_journey:vj2")("stop1", "10:50","11:00")("stop2","13:00","13:02").vj;
    //construction du validityPattern du vj2: 1110011 (1er mars est un vendredi)
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = false;
    validedays[navitia::Tuesday] = false;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern = vj->validity_pattern;


    std::vector<nt::AtPerturbation> messages;
    nt::AtPerturbation m;
    m.object_type = nt::Type_e::VehicleJourney;
    m.object_uri = "vehicle_journey:vj1";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 10:00:00"), pt::time_from_string("2013-03-03 23:59:00"));
    m.active_days = Jours::Lun | Jours::Mar | Jours::Mer | Jours::Jeu | Jours::Ven | Jours::Sam | Jours::Dim;
    m.application_daily_start_hour = pt::duration_from_string("00:00");
    m.application_daily_end_hour = pt::duration_from_string("09:30");

    messages.push_back(m);


    m.object_uri = "vehicle_journey:vj2";
    m.uri = "2";
    messages.push_back(m);

    AtAdaptedLoader loader;
    loader.apply(messages, *b.data->pt_data);

    vj = b.data->pt_data->vehicle_journeys[0];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1");
//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));
    bg::date testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);


//    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1111001"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    vj = b.data->pt_data->vehicle_journeys[1];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj2");
//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1110011"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

//    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1110011"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);

}
//vj1 et vj2 ne circule pas le 01, 02 et 03 mars
//vj3 pas impactée
BOOST_AUTO_TEST_CASE(impact_line_0){
    ed::builder b("201303011T1739");
    bg::date end_date = bg::date_from_iso_string("20130308T1739");
    b.generate_dummy_basis();

    VehicleJourney* vj = b.vj("line:A", "1111111", "", true, "vehicle_journey:vj1")("stop1", "8:50","9:00")("stop2", "11:00", "11:02").vj;
    //construction du validityPattern du vj1: 1111111
    std::bitset<7> validedays;
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = true;
    validedays[navitia::Tuesday] = true;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);

    vj = b.vj("line:A", "1110011", "", true, "vehicle_journey:vj2")("stop1", "10:50","11:00")("stop2","13:00","13:02").vj;
    //construction du validityPattern du vj2: 1110011
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = false;
    validedays[navitia::Tuesday] = false;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);

    vj = b.vj("line:B", "1111111", "", true, "vehicle_journey:vj3")("stop1", "8:50","9:00")("stop2", "11:00", "11:02").vj;
    //construction du validityPattern du vj3: 1111111
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = true;
    validedays[navitia::Tuesday] = true;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);


    std::vector<nt::AtPerturbation> messages;
    nt::AtPerturbation m;
    m.object_type = nt::Type_e::Line;
    m.object_uri = "line:A";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 00:00:00"), pt::time_from_string("2013-03-03 23:59:00"));
    m.active_days = Jours::Lun | Jours::Mar | Jours::Mer | Jours::Jeu | Jours::Ven | Jours::Sam | Jours::Dim;
    m.application_daily_start_hour = pt::duration_from_string("00:00");
    m.application_daily_end_hour = pt::duration_from_string("23:59");

    messages.push_back(m);

    AtAdaptedLoader loader;
    loader.apply(messages, *b.data->pt_data);

    vj = b.data->pt_data->vehicle_journeys[0];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1");
//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));
    bg::date testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

//    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1111000"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    vj = b.data->pt_data->vehicle_journeys[1];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj2");
//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1110011"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

//    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1110000"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    vj = b.data->pt_data->vehicle_journeys[2];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj3");
//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern, *vj->validity_pattern);

    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 3);
}
//vj1 et vj2 concerné par le message
//vj1 impacté le vendredi 01
BOOST_AUTO_TEST_CASE(impact_line_1){
    ed::builder b("201303011T1739");
    bg::date end_date = bg::date_from_iso_string("20130308T1739");
    b.generate_dummy_basis();

    VehicleJourney* vj = b.vj("line:A", "1111111", "", true, "vehicle_journey:vj1")("stop1", "8:50","9:00")("stop2", "11:00", "11:02").vj;
    //construction du validityPattern du vj1: 1111111
    std::bitset<7> validedays;
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = true;
    validedays[navitia::Tuesday] = true;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);

    vj = b.vj("line:A", "1110011", "", true, "vehicle_journey:vj2")("stop1", "10:50","11:00")("stop2","13:00","13:02").vj;
    //construction du validityPattern du vj2: 1110011
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = false;
    validedays[navitia::Tuesday] = false;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);

    vj = b.vj("line:B", "1111111", "", true, "vehicle_journey:vj3")("stop1", "10:50","11:00")("stop2","13:00","13:02").vj;
    //construction du validityPattern du vj3: 1111111
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = true;
    validedays[navitia::Tuesday] = true;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);


    std::vector<nt::AtPerturbation> messages;
    nt::AtPerturbation m;
    m.object_type = nt::Type_e::Line;
    m.object_uri = "line:A";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 00:00:00"), pt::time_from_string("2013-03-03 23:59:00"));
    m.active_days = Jours::Ven;
    m.application_daily_start_hour = pt::duration_from_string("08:30");
    m.application_daily_end_hour = pt::duration_from_string("09:30");

    messages.push_back(m);

    AtAdaptedLoader loader;
    loader.apply(messages, *b.data->pt_data);

    vj = b.data->pt_data->vehicle_journeys[0];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1");
//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));
    bg::date testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

//    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1111110"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    vj = b.data->pt_data->vehicle_journeys[1];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj2");
//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1110011"));
    bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

//    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1110011"));
    bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    vj = b.data->pt_data->vehicle_journeys[2];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj3");
    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));

    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern, *vj->validity_pattern);

    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 3);
}

BOOST_AUTO_TEST_CASE(impact_network_0){
    ed::builder b("201303011T1739");
    bg::date end_date = bg::date_from_iso_string("20130308T1739");
    b.generate_dummy_basis();
    VehicleJourney* vj = b.vj("network:A", "line:A", "1111111", "", true, "vehicle_journey:vj1")("stop1", "8:50","9:00")("stop2", "11:00", "11:02").vj;
    //construction du validityPattern du vj1: 1111111
    std::bitset<7> validedays;
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = true;
    validedays[navitia::Tuesday] = true;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);

    vj = b.vj("network:A", "line:A", "1110011", "", true, "vehicle_journey:vj2")("stop1", "10:50","11:00")("stop2","13:00","13:02").vj;
    //construction du validityPattern du vj2: 1110011
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = false;
    validedays[navitia::Tuesday] = false;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);

    vj = b.vj("network:B", "line:B", "1111111", "", true, "vehicle_journey:vj3")("stop1", "10:50","11:00")("stop2","13:00","13:02").vj;
    //construction du validityPattern du vj3: 1111111
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = true;
    validedays[navitia::Tuesday] = true;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);



    std::vector<nt::AtPerturbation> messages;
    nt::AtPerturbation m;
    m.object_type = nt::Type_e::Network;
    m.object_uri = "network:A";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 00:00:00"), pt::time_from_string("2013-03-03 23:59:00"));
    m.active_days = Jours::Lun | Jours::Mar | Jours::Mer | Jours::Jeu | Jours::Ven | Jours::Sam | Jours::Dim;
    m.application_daily_start_hour = pt::duration_from_string("00:00");
    m.application_daily_end_hour = pt::duration_from_string("23:59");

    messages.push_back(m);

    AtAdaptedLoader loader;
    loader.apply(messages, *b.data->pt_data);

    vj = b.data->pt_data->vehicle_journeys[0];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1");
//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));
    bg::date testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

//    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1111000"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    vj = b.data->pt_data->vehicle_journeys[1];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj2");
//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1110011"));
    bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

//    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1110000"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    vj = b.data->pt_data->vehicle_journeys[2];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj3");
//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));
    bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern, *vj->validity_pattern);

    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 3);
}

BOOST_AUTO_TEST_CASE(impact_network_1){
    ed::builder b("201303011T1739");
    bg::date end_date = bg::date_from_iso_string("20130308T1739");
    b.generate_dummy_basis();

    VehicleJourney* vj = b.vj("network:A", "line:A", "1111111", "", true, "vehicle_journey:vj1")("stop1", "8:50","9:00")("stop2", "11:00", "11:02").vj;
    //construction du validityPattern du vj1: 1111111
    std::bitset<7> validedays;
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = true;
    validedays[navitia::Tuesday] = true;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);

    vj = b.vj("network:A", "line:A", "", "", true, "vehicle_journey:vj2")("stop1", "10:50","11:00")("stop2","13:00","13:02").vj;
    //construction du validityPattern du vj2: 1110011
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = false;
    validedays[navitia::Tuesday] = false;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);

    vj = b.vj("network:B", "line:B", "1111111", "", true, "vehicle_journey:vj3")("stop1", "10:50","11:00")("stop2","13:00","13:02").vj;
    //construction du validityPattern du vj3: 1111111
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = true;
    validedays[navitia::Tuesday] = true;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);



    std::vector<nt::AtPerturbation> messages;
    nt::AtPerturbation m;
    m.object_type = nt::Type_e::Network;
    m.object_uri = "network:A";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 00:00:00"), pt::time_from_string("2013-03-03 23:59:00"));
    m.active_days = Jours::Ven;
    m.application_daily_start_hour = pt::duration_from_string("08:30");
    m.application_daily_end_hour = pt::duration_from_string("09:30");

    messages.push_back(m);

    AtAdaptedLoader loader;
    loader.apply(messages, *b.data->pt_data);

    vj = b.data->pt_data->vehicle_journeys[0];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1");
//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));
    bg::date testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

//    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1111110"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    vj = b.data->pt_data->vehicle_journeys[1];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj2");
//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1110011"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

//    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1110011"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    vj = b.data->pt_data->vehicle_journeys[2];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj3");
//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern, *vj->validity_pattern);

    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 3);
}

BOOST_AUTO_TEST_CASE(impact_network_2){
    ed::builder b("20130301T1739");
    bg::date end_date = bg::date_from_iso_string("20130308T1739");
    b.generate_dummy_basis();

    VehicleJourney* vj = b.vj("network:A", "line:A", "1111111", "", true, "vehicle_journey:vj1")("stop1", "8:50","9:00")("stop2", "11:00", "11:02").vj;
    //construction du validityPattern du vj1: 1111111
    std::bitset<7> validedays;
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = true;
    validedays[navitia::Tuesday] = true;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);

    vj = b.vj("network:A", "line:A", "1110011", "", true, "vehicle_journey:vj2")("stop1", "10:50","11:00")("stop2","13:00","13:02").vj;
    //construction du validityPattern du vj2: 1110011
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = false;
    validedays[navitia::Tuesday] = false;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);

    vj = b.vj("network:B", "line:B", "1111111", "", true, "vehicle_journey:vj3")("stop1", "10:50","11:00")("stop2","13:00","13:02").vj;
    //construction du validityPattern du vj3: 1111111
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = true;
    validedays[navitia::Tuesday] = true;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);


    std::vector<nt::AtPerturbation> messages;
    nt::AtPerturbation m;
    m.object_type = nt::Type_e::Network;
    m.object_uri = "network:A";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 00:00:00"), pt::time_from_string("2013-03-03 23:59:00"));
    m.active_days = Jours::Ven;
    m.application_daily_start_hour = pt::duration_from_string("08:30");
    m.application_daily_end_hour = pt::duration_from_string("09:30");

    messages.push_back(m);

    m.uri = "2";
    m.object_type = nt::Type_e::VehicleJourney;
    m.object_uri = "vehicle_journey:vj1";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 00:00:00"), pt::time_from_string("2013-03-04 23:59:00"));
    m.active_days = Jours::Sam;
    m.application_daily_start_hour = pt::duration_from_string("08:30");
    m.application_daily_end_hour = pt::duration_from_string("09:30");

    messages.push_back(m);

    AtAdaptedLoader loader;
    loader.apply(messages, *b.data->pt_data);

    vj = b.data->pt_data->vehicle_journeys[0];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1");
//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));
    bg::date testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

//    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1111100"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    vj = b.data->pt_data->vehicle_journeys[1];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj2");
//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1110011"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);


//    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1110011"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    vj = b.data->pt_data->vehicle_journeys[2];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj3");
//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern, *vj->validity_pattern);

    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 3);
}

BOOST_AUTO_TEST_CASE(impact_network_3){
    ed::builder b("20130301T1739");
    bg::date end_date = bg::date_from_iso_string("20130308T1739");
    b.generate_dummy_basis();

    VehicleJourney* vj = b.vj("network:A", "lineA", "1111111", "", true, "vehicle_journey:vj1")("stop1", "8:50","9:00")("stop2", "11:00", "11:02").vj;
    //construction du validityPattern du vj1: 1111111
    std::bitset<7> validedays;
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = true;
    validedays[navitia::Tuesday] = true;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);

    vj = b.vj("network:A", "line:A", "1110011", "", true, "vehicle_journey:vj2")("stop1", "10:50","11:00")("stop2","13:00","13:02").vj;
    //construction du validityPattern du vj2: 1110011
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = false;
    validedays[navitia::Tuesday] = false;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);

    vj = b.vj("network:B", "line:B", "1111111", "", true, "vehicle_journey:vj3")("stop1", "10:50","11:00")("stop2","13:00","13:02").vj;
    //construction du validityPattern du vj3: 1111111
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = true;
    validedays[navitia::Tuesday] = true;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);


    std::vector<nt::AtPerturbation> messages;
    nt::AtPerturbation m;
    m.object_type = nt::Type_e::Network;
    m.object_uri = "network:A";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 00:00:00"), pt::time_from_string("2013-03-03 23:59:00"));
    m.active_days = Jours::Ven;
    m.application_daily_start_hour = pt::duration_from_string("08:30");
    m.application_daily_end_hour = pt::duration_from_string("09:30");

    messages.push_back(m);

    m.uri = "2";
    m.object_type = nt::Type_e::VehicleJourney;
    m.object_uri = "vehicle_journey:vj1";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 00:00:00"), pt::time_from_string("2013-03-03 23:59:00"));
    m.active_days = Jours::Ven;
    m.application_daily_start_hour = pt::duration_from_string("08:30");
    m.application_daily_end_hour = pt::duration_from_string("09:30");

    messages.push_back(m);

    AtAdaptedLoader loader;
    loader.apply(messages, *b.data->pt_data);

    vj = b.data->pt_data->vehicle_journeys[0];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1");
//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));
    bg::date testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

//    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1111110"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    vj = b.data->pt_data->vehicle_journeys[1];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj2");
//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1110011"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

//    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1110011"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);


    vj = b.data->pt_data->vehicle_journeys[2];

    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj3");
//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern, *vj->validity_pattern);

    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 3);
}

BOOST_AUTO_TEST_CASE(impact_stoppoint_0){
    ed::builder b("20130301T1739");
    bg::date end_date = bg::date_from_iso_string("20130308T1739");
    b.generate_dummy_basis();
    VehicleJourney* vj = b.vj("A", "", "", true, "vehicle_journey:vj1")("stop_point:stop1", -1,8050)("stop_point:stop2", 8200)("stop_point:stop3", 8200,8250).vj;
    //construction du validityPattern du vj1: 1111111
    std::bitset<7> validedays;
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = true;
    validedays[navitia::Tuesday] = true;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = false;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern = vj->validity_pattern;


    std::vector<nt::AtPerturbation> messages;
    nt::AtPerturbation m;
    m.object_type = nt::Type_e::StopPoint;
    m.object_uri = "stop_point:stop1";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 00:00:00"), pt::time_from_string("2013-03-03 23:59:00"));
    m.active_days = Jours::Ven | Jours::Sam | Jours::Dim;
    m.application_daily_start_hour = pt::duration_from_string("00:00");
    m.application_daily_end_hour = pt::duration_from_string("23:59");

    messages.push_back(m);

    BOOST_CHECK_EQUAL(b.data->pt_data->stop_times.size(), 3);

    AtAdaptedLoader loader;
    loader.apply(messages, *b.data->pt_data);

    BOOST_CHECK_EQUAL(b.data->pt_data->stop_times.size(), 5);

    vj = b.data->pt_data->vehicle_journeys[0];
    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1");

//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));
    bg::date testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

//    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1111110"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    BOOST_CHECK_EQUAL(vj->stop_time_list.size(), 3);





    vj = b.data->pt_data->vehicle_journeys[1];
    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1:adapted:0");
    BOOST_CHECK_EQUAL(vj->is_adapted,  true);

    BOOST_CHECK(vj->validity_pattern->days.none());

    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    BOOST_CHECK_EQUAL(vj->stop_time_list.size(), 2);
    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
}

//test : message supprimant le vj donc il n'est pas dupliqué
BOOST_AUTO_TEST_CASE(impact_stoppoint_1){
    ed::builder b("20130301T1739");
    bg::date end_date = bg::date_from_iso_string("20130308T1739");
    b.generate_dummy_basis();
    VehicleJourney* vj = b.vj("A", "", "", true, "vehicle_journey:vj1")("stop_point:stop1", 8000,8050)("stop_point:stop2", 8200,8250).vj;
    //construction du validityPattern du vj1: 1111111
    std::bitset<7> validedays;
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = true;
    validedays[navitia::Tuesday] = true;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);


    std::vector<nt::AtPerturbation> messages;
    nt::AtPerturbation m;
    m.object_type = nt::Type_e::StopPoint;
    m.object_uri = "stop_point:stop1";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 00:00:00"), pt::time_from_string("2013-03-03 23:59:00"));
    m.active_days = Jours::Ven | Jours::Sam | Jours::Dim;
    m.application_daily_start_hour = pt::duration_from_string("00:00");
    m.application_daily_end_hour = pt::duration_from_string("23:59");

    messages.push_back(m);

    m.uri = "2";
    m.object_type = nt::Type_e::VehicleJourney;
    m.object_uri = "vehicle_journey:vj1";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 00:00:00"), pt::time_from_string("2013-03-03 23:59:00"));
    m.active_days = Jours::Ven | Jours::Sam | Jours::Dim;
    m.application_daily_start_hour = pt::duration_from_string("00:00");
    m.application_daily_end_hour = pt::duration_from_string("23:59");

    messages.push_back(m);

    BOOST_CHECK_EQUAL(b.data->pt_data->stop_times.size(), 2);

    AtAdaptedLoader loader;
    loader.apply(messages, *b.data->pt_data);

    BOOST_CHECK_EQUAL(b.data->pt_data->stop_times.size(), 2);

    vj = b.data->pt_data->vehicle_journeys[0];
    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1");

//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));
    bg::date testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

//    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1111110"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    BOOST_CHECK_EQUAL(vj->stop_time_list.size(), 2);
    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 1);
}

//test 2 messages <> sur un meme vehiclejourney
BOOST_AUTO_TEST_CASE(impact_stoppoint_2){
    ed::builder b("20130301T1739");
    bg::date end_date = bg::date_from_iso_string("20130308T1739");
    b.generate_dummy_basis();
    VehicleJourney* vj = b.vj("A", "", "", true, "vehicle_journey:vj1")("stop_point:stop1", 8000,8050)("stop_point:stop2", 8200, 8250).vj;
    //construction du validityPattern du vj1: 1111111
    std::bitset<7> validedays;
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = true;
    validedays[navitia::Tuesday] = true;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);


    std::vector<nt::AtPerturbation> messages;
    nt::AtPerturbation m;
    m.object_type = nt::Type_e::StopPoint;
    m.object_uri = "stop_point:stop1";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 00:00:00"), pt::time_from_string("2013-03-03 23:59:00"));
    m.active_days = Jours::Ven | Jours::Sam | Jours::Dim;
    m.application_daily_start_hour = pt::duration_from_string("00:00");
    m.application_daily_end_hour = pt::duration_from_string("23:59");

    messages.push_back(m);

    m.uri = "3";
    m.object_type = nt::Type_e::StopPoint;
    m.object_uri = "stop_point:stop2";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 00:00:00"), pt::time_from_string("2013-03-03 23:59:00"));
    m.active_days = Jours::Ven | Jours::Sam | Jours::Dim;
    m.application_daily_start_hour = pt::duration_from_string("00:00");
    m.application_daily_end_hour = pt::duration_from_string("23:59");

    messages.push_back(m);

    BOOST_CHECK_EQUAL(b.data->pt_data->stop_times.size(), 2);
    AtAdaptedLoader loader;
    loader.apply(messages, *b.data->pt_data);
    BOOST_CHECK_EQUAL(b.data->pt_data->stop_times.size(), 3);

    vj = b.data->pt_data->vehicle_journeys[0];
    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1");

//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));
    bg::date testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

//    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1111110"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    BOOST_CHECK_EQUAL(vj->stop_time_list.size(), 2);


    //le premier vj adapté ne doit pas circuler, il correspond à un état intermédiaire
    vj = b.data->pt_data->vehicle_journeys[1];
    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1:adapted:0");
    BOOST_CHECK_EQUAL(vj->is_adapted,  true);
    BOOST_CHECK(vj->validity_pattern->days.none());
    BOOST_CHECK(vj->adapted_validity_pattern->days.none());
    BOOST_CHECK_EQUAL(vj->stop_time_list.size(), 1);


    vj = b.data->pt_data->vehicle_journeys[2];
    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1:adapted:1");
    BOOST_CHECK_EQUAL(vj->is_adapted,  true);

    BOOST_CHECK(vj->validity_pattern->days.none());

    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    BOOST_CHECK_EQUAL(vj->stop_time_list.size(), 0);
    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 3);
}

BOOST_AUTO_TEST_CASE(impact_stoppoint_passe_minuit){
    ed::builder b("20130301T1739");
    bg::date end_date = bg::date_from_iso_string("20130308T1739");
    b.generate_dummy_basis();
    VehicleJourney* vj = b.vj("A", "", "", true, "vehicle_journey:vj1")("stop_point:stop1", "23:30", "23:40")("stop_point:stop2", "25:00", "25:00")("stop_point:stop3", "26:00", "26:10").vj;
    //construction du validityPattern du vj1: 1111111
    std::bitset<7> validedays;
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = true;
    validedays[navitia::Tuesday] = true;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);


    std::vector<nt::AtPerturbation> messages;
    nt::AtPerturbation m;
    m.object_type = nt::Type_e::StopPoint;
    m.object_uri = "stop_point:stop2";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-02 00:00:00"), pt::time_from_string("2013-03-02 23:59:00"));
    m.active_days = Jours::Lun | Jours::Mar | Jours::Mer | Jours::Jeu | Jours::Ven | Jours::Sam | Jours::Dim;
    m.application_daily_start_hour = pt::duration_from_string("00:00");
    m.application_daily_end_hour = pt::duration_from_string("23:59");

    messages.push_back(m);


    AtAdaptedLoader loader;
    loader.apply(messages, *b.data->pt_data);

    vj = b.data->pt_data->vehicle_journeys[0];
    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1");

//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));
    bg::date testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

//    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1111110"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    BOOST_CHECK_EQUAL(vj->stop_time_list.size(), 3);

    vj = b.data->pt_data->vehicle_journeys[1];
    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1:adapted:0");
    BOOST_CHECK_EQUAL(vj->is_adapted,  true);
    BOOST_CHECK(vj->validity_pattern->days.none());

    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    BOOST_CHECK_EQUAL(vj->stop_time_list.size(), 2);
    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 2);
}

//test 2 messages <> sur un meme vehiclejourney, avec des dates d'application qui se chevauche mais qui ne sont pas égales
BOOST_AUTO_TEST_CASE(impact_stoppoint_3){
    ed::builder b("20130301T1739");
    bg::date end_date = bg::date_from_iso_string("20130308T1739");
    b.generate_dummy_basis();
    VehicleJourney* vj = b.vj("A", "", "", true, "vehicle_journey:vj1")("stop_point:stop1", 8000,8050)("stop_point:stop2", 8200,8250).vj;
    //construction du validityPattern du vj1: 1111111
    std::bitset<7> validedays;
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = true;
    validedays[navitia::Tuesday] = true;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);


    std::vector<nt::AtPerturbation> messages;
    nt::AtPerturbation m;
    m.object_type = nt::Type_e::StopPoint;
    m.object_uri = "stop_point:stop1";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 00:00:00"), pt::time_from_string("2013-03-03 23:59:00"));
    m.active_days = Jours::Lun | Jours::Mar | Jours::Mer | Jours::Jeu | Jours::Ven | Jours::Sam | Jours::Dim;
    m.application_daily_start_hour = pt::duration_from_string("00:00");
    m.application_daily_end_hour = pt::duration_from_string("23:59");

    messages.push_back(m);

    m.uri = "3";
    m.object_type = nt::Type_e::StopPoint;
    m.object_uri = "stop_point:stop2";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-02 00:00:00"), pt::time_from_string("2013-03-05 23:59:00"));
    m.active_days = Jours::Lun | Jours::Mar | Jours::Mer | Jours::Jeu | Jours::Ven | Jours::Sam | Jours::Dim;
    m.application_daily_start_hour = pt::duration_from_string("00:00");
    m.application_daily_end_hour = pt::duration_from_string("23:59");

    messages.push_back(m);

    AtAdaptedLoader loader;
    loader.apply(messages, *b.data->pt_data);

    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 4);
    vj = b.data->pt_data->vehicle_journeys[0];
    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1");

//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));
    bg::date testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

//    BOOST_CHECK_EQUAL(*vj->adapted_validity_pattern,  ValidityPattern(b.begin, "1111110"));
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    BOOST_CHECK_EQUAL(vj->stop_time_list.size(), 2);


    vj = b.data->pt_data->vehicle_journeys[1];
    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1:adapted:0");
    BOOST_CHECK_EQUAL(vj->is_adapted,  true);
    BOOST_CHECK(vj->validity_pattern->days.none());
    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    BOOST_CHECK_EQUAL(vj->stop_time_list.size(), 1);
    BOOST_CHECK_EQUAL(vj->stop_time_list.front()->journey_pattern_point->stop_point->uri, "stop_point:stop2");
    //check stopoint:2



    vj = b.data->pt_data->vehicle_journeys[2];
    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1:adapted:1");
    BOOST_CHECK_EQUAL(vj->is_adapted,  true);

    BOOST_CHECK(vj->validity_pattern->days.none());

    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    BOOST_CHECK_EQUAL(vj->stop_time_list.size(), 0);

    vj = b.data->pt_data->vehicle_journeys[3];
    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1:adapted:2");
    BOOST_CHECK_EQUAL(vj->is_adapted,  true);

    BOOST_CHECK(vj->validity_pattern->days.none());

    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    BOOST_CHECK_EQUAL(vj->stop_time_list.size(), 1);
    BOOST_CHECK_EQUAL(vj->stop_time_list.front()->journey_pattern_point->stop_point->uri, "stop_point:stop1");
}

BOOST_AUTO_TEST_CASE(impact_stoppoint_4){
    ed::builder b("20130301T1739");
    bg::date end_date = bg::date_from_iso_string("20130312T1739");
    b.generate_dummy_basis();
    VehicleJourney* vj = b.vj("A", "", "", true, "vehicle_journey:vj1")("stop_point:stop1", 8000,8050)("stop_point:stop2", 8200,8250).vj;
    //construction du validityPattern du vj1: 1111111
    std::bitset<7> validedays;
    validedays[navitia::Sunday] = true;
    validedays[navitia::Monday] = true;
    validedays[navitia::Tuesday] = true;
    validedays[navitia::Wednesday] = true;
    validedays[navitia::Thursday] = true;
    validedays[navitia::Friday] = true;
    validedays[navitia::Saturday] = true;
    vj->validity_pattern->add(vj->validity_pattern->beginning_date, end_date, validedays);
    vj->adapted_validity_pattern->add(vj->adapted_validity_pattern->beginning_date, end_date, validedays);


    std::vector<nt::AtPerturbation> messages;
    nt::AtPerturbation m;
    m.object_type = nt::Type_e::StopPoint;
    m.object_uri = "stop_point:stop1";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 00:00:00"), pt::time_from_string("2014-03-01 23:59:00"));
    m.active_days = Jours::Sam | Jours::Dim | Jours::Lun;
    m.application_daily_start_hour = pt::duration_from_string("00:00");
    m.application_daily_end_hour = pt::duration_from_string("23:59");

    messages.push_back(m);

    m.uri = "2";
    m.object_uri = "stop_point:stop2";
    m.application_period = pt::time_period(pt::time_from_string("2013-03-01 00:00:00"), pt::time_from_string("2013-03-15 23:59:00"));
    m.active_days = Jours::Ven | Jours::Lun;
    messages.push_back(m);

    BOOST_CHECK_EQUAL(b.data->pt_data->stop_times.size(), 2);
    AtAdaptedLoader loader;
    loader.apply(messages, *b.data->pt_data);
    BOOST_CHECK_EQUAL(b.data->pt_data->stop_times.size(), 4);

    BOOST_CHECK_EQUAL(b.data->pt_data->vehicle_journeys.size(), 4);

    vj = b.data->pt_data->vehicle_journeys[0];
    BOOST_CHECK_EQUAL(vj->uri, "vehicle_journey:vj1");

//    BOOST_CHECK_EQUAL(*vj->validity_pattern,  ValidityPattern(b.begin, "1111111"));
    bg::date testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130308T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130309T1739");
    BOOST_CHECK_EQUAL(vj->validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130308T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130309T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130310T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130311T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->validity_pattern->beginning_date).days()), false);

    BOOST_CHECK_EQUAL(vj->stop_time_list.size(), 2);
    for(unsigned int i=0; i < vj->stop_time_list.size(); ++i){
        BOOST_CHECK_EQUAL(vj->stop_time_list[i]->journey_pattern_point->order, i);
    }



    vj = b.data->pt_data->vehicle_journeys[1];
    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1:adapted:0");
    BOOST_CHECK_EQUAL(vj->is_adapted,  true);

    BOOST_CHECK(vj->validity_pattern->days.none());

    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130308T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130309T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130310T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130311T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    BOOST_CHECK_EQUAL(vj->stop_time_list.size(), 1);
    for(unsigned int i=0; i < vj->stop_time_list.size(); ++i){
        BOOST_CHECK_EQUAL(vj->stop_time_list[i]->journey_pattern_point->order, i);
    }

    vj = b.data->pt_data->vehicle_journeys[2];
    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1:adapted:1");
    BOOST_CHECK_EQUAL(vj->is_adapted,  true);

    BOOST_CHECK(vj->validity_pattern->days.none());

    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130308T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130309T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130310T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130311T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    BOOST_CHECK_EQUAL(vj->stop_time_list.size(), 1);
    for(unsigned int i=0; i < vj->stop_time_list.size(); ++i){
        BOOST_CHECK_EQUAL(vj->stop_time_list[i]->journey_pattern_point->order, i);
    }

    vj = b.data->pt_data->vehicle_journeys[3];
    BOOST_CHECK_EQUAL(vj->uri,  "vehicle_journey:vj1:adapted:2");
    BOOST_CHECK_EQUAL(vj->is_adapted,  true);

    BOOST_CHECK(vj->validity_pattern->days.none());

    testdate = bg::date_from_iso_string("20130301T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130302T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130303T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130304T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), true);

    testdate = bg::date_from_iso_string("20130305T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130306T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130307T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130308T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130309T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130310T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), false);

    testdate = bg::date_from_iso_string("20130311T1739");
    BOOST_CHECK_EQUAL(vj->adapted_validity_pattern->check((testdate - vj->adapted_validity_pattern->beginning_date).days()), true);

    BOOST_CHECK_EQUAL(vj->stop_time_list.size(), 0);
}
