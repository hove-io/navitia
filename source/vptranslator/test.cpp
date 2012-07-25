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
    BOOST_CHECK_EQUAL(dayofweek, 7);

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
    BOOST_CHECK_EQUAL(dayofweek, -7);

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

    //test de la decoupe de la condition de service
    testtranslation.splitcs();
    BOOST_CHECK_EQUAL(testtranslation.week_map[0].week_bs.to_string(), "0001011");
    testdate = boost::gregorian::date(2012,7,19);
    BOOST_CHECK_EQUAL(testtranslation.week_map[0].startdate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.week_map[1].week_bs.to_string(), "1001111");
    testdate = boost::gregorian::date(2012,7,23);
    BOOST_CHECK_EQUAL(testtranslation.week_map[1].startdate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.week_map[2].week_bs.to_string(), "0011000");
    testdate = boost::gregorian::date(2012,7,30);
    BOOST_CHECK_EQUAL(testtranslation.week_map[2].startdate, testdate);

    testCS="111111111111111111111";
    testdate= boost::gregorian::date(2012,7,2);
    response = testtranslation.initcs(testdate, testCS);
    BOOST_CHECK_EQUAL(response, true);
    BOOST_CHECK_EQUAL(testtranslation.CS, "111111111111111111111");
    testdate = boost::gregorian::date(2012,7,2);
    BOOST_CHECK_EQUAL(testtranslation.startdate, testdate);

    testdate = boost::gregorian::date(2012,7,22);
    BOOST_CHECK_EQUAL(testtranslation.enddate, testdate);

    //test de la decoupe de la condition de service
    testtranslation.splitcs();
    BOOST_CHECK_EQUAL(testtranslation.week_map[0].week_bs.to_string(), "1111111");
    testdate = boost::gregorian::date(2012,7,2);
    BOOST_CHECK_EQUAL(testtranslation.week_map[0].startdate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.week_map[1].week_bs.to_string(), "1111111");
    testdate = boost::gregorian::date(2012,7,9);
    BOOST_CHECK_EQUAL(testtranslation.week_map[1].startdate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.week_map[2].week_bs.to_string(), "1111111");
    testdate = boost::gregorian::date(2012,7,16);
    BOOST_CHECK_EQUAL(testtranslation.week_map[2].startdate, testdate);

    testtranslation.translate();

    MakeTranslation::target targetresponse;
//    for(std::map<int, MakeTranslation::target>::iterator it = testtranslation.target_map.begin(); it!= testtranslation.target_map.end(); it++) {
//        targetresponse = it->second;
//        std::cout << targetresponse.week_bs.to_string()<<std::endl;
//        for(std::vector<boost::gregorian::date>::iterator it2 = targetresponse.periodlist.begin(); it2!= targetresponse.periodlist.end(); it2++) {
//            std::cout << *it2 <<std::endl;;
//        }
//    }
    testtranslation.bounddrawdown();

    testCS="011111100000001111111";
    testdate= boost::gregorian::date(2012,7,2);
    response = testtranslation.initcs(testdate, testCS);
    BOOST_CHECK_EQUAL(response, true);
    BOOST_CHECK_EQUAL(testtranslation.CS, "11111100000001111111");
    testdate = boost::gregorian::date(2012,7,3);
    BOOST_CHECK_EQUAL(testtranslation.startdate, testdate);

    testdate = boost::gregorian::date(2012,7,22);
    BOOST_CHECK_EQUAL(testtranslation.enddate, testdate);

    //test de la decoupe de la condition de service
    testtranslation.splitcs();
    BOOST_CHECK_EQUAL(testtranslation.week_map[0].week_bs.to_string(), "0111111");
    testdate = boost::gregorian::date(2012,7,3);
    BOOST_CHECK_EQUAL(testtranslation.week_map[0].startdate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.week_map[1].week_bs.to_string(), "0000000");
    testdate = boost::gregorian::date(2012,7,9);
    BOOST_CHECK_EQUAL(testtranslation.week_map[1].startdate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.week_map[2].week_bs.to_string(), "1111111");
    testdate = boost::gregorian::date(2012,7,16);
    BOOST_CHECK_EQUAL(testtranslation.week_map[2].startdate, testdate);

    testtranslation.translate();


//    for(std::map<int, MakeTranslation::target>::iterator it = testtranslation.target_map.begin(); it!= testtranslation.target_map.end(); it++) {
//        targetresponse = it->second;
//        std::cout<<std::endl<< targetresponse.week_bs.to_string()<<std::endl;
//        for(std::vector<boost::gregorian::date>::iterator it2 = targetresponse.periodlist.begin(); it2!= targetresponse.periodlist.end(); it2++) {
//            std::cout << *it2 <<std::endl;
//        }
//    }
    testtranslation.bounddrawdown();



    testCS="001101111111011111111";
    testdate= boost::gregorian::date(2012,7,2);
    response = testtranslation.initcs(testdate, testCS);
    BOOST_CHECK_EQUAL(response, true);
    BOOST_CHECK_EQUAL(testtranslation.CS, "1101111111011111111");
    testdate = boost::gregorian::date(2012,7,4);
    BOOST_CHECK_EQUAL(testtranslation.startdate, testdate);

    testdate = boost::gregorian::date(2012,7,22);
    BOOST_CHECK_EQUAL(testtranslation.enddate, testdate);

    //test de la decoupe de la condition de service
    testtranslation.splitcs();
    BOOST_CHECK_EQUAL(testtranslation.week_map[0].week_bs.to_string(), "0011011");
    testdate = boost::gregorian::date(2012,7,4);
    BOOST_CHECK_EQUAL(testtranslation.week_map[0].startdate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.week_map[1].week_bs.to_string(), "1111101");
    testdate = boost::gregorian::date(2012,7,9);
    BOOST_CHECK_EQUAL(testtranslation.week_map[1].startdate, testdate);

//    BOOST_CHECK_EQUAL(testtranslation.week_map[2].week_bs.to_string(), "1111111");
//    testdate = boost::gregorian::date(2012,7,16);
//    BOOST_CHECK_EQUAL(testtranslation.week_map[2].startdate, testdate);

    std::cout << "nouveau test : " <<std::endl;
    testtranslation.translate();
    testtranslation.bounddrawdown();

//et le 07/jul
//sauf  06/jul
    for(std::map<int, MakeTranslation::target>::iterator it = testtranslation.target_map.begin(); it!= testtranslation.target_map.end(); it++) {
        targetresponse = it->second;
        std::cout<<std::endl<< targetresponse.week_bs.to_string()<<std::endl;
        std::cout << "liste des periodes : " <<std::endl;
        for(std::vector<boost::gregorian::date>::iterator it2 = targetresponse.periodlist.begin(); it2!= targetresponse.periodlist.end(); it2++) {
            std::cout << *it2 <<std::endl;
        }
        std::cout << "liste des ET : " <<std::endl;
        for(std::vector<boost::gregorian::date>::iterator it3 = targetresponse.andlist.begin(); it3!= targetresponse.andlist.end(); it3++) {
            std::cout << *it3 <<std::endl;
        }
        std::cout << "liste des SAUF: " <<std::endl;
        for(std::vector<boost::gregorian::date>::iterator it4 = targetresponse.exceptlist.begin(); it4!= targetresponse.exceptlist.end(); it4++) {
            std::cout << *it4 <<std::endl;
        }
    }
}
