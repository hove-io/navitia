#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE vptranslator_test

#include <boost/test/unit_test.hpp>
#include "vptranslator/vptranslator.h"

/*using namespace navitia::type;*/


BOOST_AUTO_TEST_CASE(default_test) {
    boost::gregorian::date testdate;
    std::string testCS;
    bool response;
    int dayofweek;
    MakeTranslation testtranslation;


    testCS ="0000000000";
    testdate = boost::gregorian::date(2012,7,16);
    response = testtranslation.initcs(testdate, testCS);
    BOOST_CHECK_EQUAL(response, false);

    testCS="0001011100";
    testdate= boost::gregorian::date(2012,7,16);
    response = testtranslation.initcs(testdate, testCS);
    BOOST_CHECK_EQUAL(response, true);
    BOOST_CHECK_EQUAL(testtranslation.CS, "10111");

    testdate = boost::gregorian::date(2012,7,19);
    BOOST_CHECK_EQUAL(testtranslation.startdate, testdate);

    testdate = boost::gregorian::date(2012,7,23);
    BOOST_CHECK_EQUAL(testtranslation.enddate, testdate);

    testCS="000001000";
    testdate= boost::gregorian::date(2012,7,16);
    response = testtranslation.initcs(testdate, testCS);
    BOOST_CHECK_EQUAL(response, true);
    BOOST_CHECK_EQUAL(testtranslation.CS, "1");

    testdate = boost::gregorian::date(2012,7,21);
    BOOST_CHECK_EQUAL(testtranslation.startdate, testdate);

    testdate = boost::gregorian::date(2012,7,21);
    BOOST_CHECK_EQUAL(testtranslation.enddate, testdate);

    testdate = boost::gregorian::date(2012,7,23); //lundi
    dayofweek = testtranslation.getnextmonday(testdate, 1);
    BOOST_CHECK_EQUAL(dayofweek, 0);

    testdate = boost::gregorian::date(2012,7,24); //mardi
    dayofweek = testtranslation.getnextmonday(testdate, 1);
    BOOST_CHECK_EQUAL(dayofweek, 6);

    testdate = boost::gregorian::date(2012,7,25); //mercredi
    dayofweek = testtranslation.getnextmonday(testdate, 1);
    BOOST_CHECK_EQUAL(dayofweek, 5);

    testdate = boost::gregorian::date(2012,7,26); //jeudi
    dayofweek = testtranslation.getnextmonday(testdate, 1);
    BOOST_CHECK_EQUAL(dayofweek, 4);

    testdate = boost::gregorian::date(2012,7,27); //vendredi
    dayofweek = testtranslation.getnextmonday(testdate, 1);
    BOOST_CHECK_EQUAL(dayofweek, 3);

    testdate = boost::gregorian::date(2012,7,28); //samedi
    dayofweek = testtranslation.getnextmonday(testdate, 1);
    BOOST_CHECK_EQUAL(dayofweek, 2);

    testdate = boost::gregorian::date(2012,7,29); //dimanche
    dayofweek = testtranslation.getnextmonday(testdate, 1);
    BOOST_CHECK_EQUAL(dayofweek, 1);

    testdate = boost::gregorian::date(2012,7,23); //lundi
    dayofweek = testtranslation.getnextmonday(testdate, -1);
    BOOST_CHECK_EQUAL(dayofweek, 0);

    testdate = boost::gregorian::date(2012,7,24); //mardi
    dayofweek = testtranslation.getnextmonday(testdate, -1);
    BOOST_CHECK_EQUAL(dayofweek, -1);

    testdate = boost::gregorian::date(2012,7,25); //mercredi
    dayofweek = testtranslation.getnextmonday(testdate, -1);
    BOOST_CHECK_EQUAL(dayofweek, -2);

    testdate = boost::gregorian::date(2012,7,26); //jeudi
    dayofweek = testtranslation.getnextmonday(testdate, -1);
    BOOST_CHECK_EQUAL(dayofweek, -3);

    testdate = boost::gregorian::date(2012,7,27); //vendredi
    dayofweek = testtranslation.getnextmonday(testdate, -1);
    BOOST_CHECK_EQUAL(dayofweek, -4);

    testdate = boost::gregorian::date(2012,7,28); //samedi
    dayofweek = testtranslation.getnextmonday(testdate, -1);
    BOOST_CHECK_EQUAL(dayofweek, -5);

    testdate = boost::gregorian::date(2012,7,29); //dimanche
    dayofweek = testtranslation.getnextmonday(testdate, -1);
    BOOST_CHECK_EQUAL(dayofweek, -6);

    testCS="00010111001111001100";
    testdate= boost::gregorian::date(2012,7,16);
    response = testtranslation.initcs(testdate, testCS);
    BOOST_CHECK_EQUAL(response, true);
    BOOST_CHECK_EQUAL(testtranslation.CS, "101110011110011");
    testdate = boost::gregorian::date(2012,7,19);
    BOOST_CHECK_EQUAL(testtranslation.startdate, testdate);

    testdate = boost::gregorian::date(2012,8,2);
    BOOST_CHECK_EQUAL(testtranslation.enddate, testdate);

    testtranslation.splitcs();
//    0001011
//    1001111
//    001100

    BOOST_CHECK_EQUAL(testtranslation.week_map[0].week_bs.to_string(), "0001011");
    BOOST_CHECK_EQUAL(testtranslation.week_map[1].week_bs.to_string(), "1001111");
    BOOST_CHECK_EQUAL(testtranslation.week_map[2].week_bs.to_string(), "0011000");


}
