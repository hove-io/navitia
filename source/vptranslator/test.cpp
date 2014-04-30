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
#define BOOST_TEST_MODULE vptranslator_test

#include <boost/test/unit_test.hpp>
#include "vptranslator/vptranslator.h"

/*using namespace navitia::type;*/

BOOST_AUTO_TEST_CASE(decoupage_borne) {
    boost::gregorian::date testdate;
    std::string testCS;
    bool response;
    int dayofweek;
    MakeTranslation testtranslation;
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
}

BOOST_AUTO_TEST_CASE(default_test) {
    boost::gregorian::date testdate;
    std::string testCS;
    bool response;
    MakeTranslation testtranslation;

    testCS ="0000000000";
    testdate = boost::gregorian::date(2012,7,16);
    response = testtranslation.initcs(testdate, testCS);
    testtranslation.splitcs();
    testtranslation.translate();
    testtranslation.bounddrawdown();
    BOOST_CHECK_EQUAL(response, false);

    testCS="000000000010000";
    testdate= boost::gregorian::date(2012,7,2);
    response = testtranslation.initcs(testdate, testCS);
    //test de la decoupe de la condition de service
    testtranslation.splitcs();
    testtranslation.translate();
    testtranslation.bounddrawdown();

    BOOST_CHECK_EQUAL(response, true);
    BOOST_CHECK_EQUAL(testtranslation.CS, "1");
    testdate = boost::gregorian::date(2012,7,12);
    BOOST_CHECK_EQUAL(testtranslation.startdate, testdate);
    BOOST_CHECK_EQUAL(testtranslation.enddate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.week_vector[0].week_bs.to_string(), "0001000");
    testdate = boost::gregorian::date(2012,7,12);
    BOOST_CHECK_EQUAL(testtranslation.week_vector[0].startdate, testdate);

    //test au limite : lundi
    testCS="000000010000";
    testdate= boost::gregorian::date(2012,7,2);
    response = testtranslation.initcs(testdate, testCS);
    //test de la decoupe de la condition de service
    testtranslation.splitcs();
    testtranslation.translate();
    testtranslation.bounddrawdown();
    testtranslation.targetdrawdown();

    BOOST_CHECK_EQUAL(response, true);
    BOOST_CHECK_EQUAL(testtranslation.CS, "1");
    testdate = boost::gregorian::date(2012,7,9);
    BOOST_CHECK_EQUAL(testtranslation.startdate, testdate);
    BOOST_CHECK_EQUAL(testtranslation.enddate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.week_vector[0].week_bs.to_string(), "1000000");
    testdate = boost::gregorian::date(2012,7,9);
    BOOST_CHECK_EQUAL(testtranslation.week_vector[0].startdate, testdate);

    //test au limite : dimanche
    testCS="000000100000";
    testdate= boost::gregorian::date(2012,7,2);
    response = testtranslation.initcs(testdate, testCS);
    //test de la decoupe de la condition de service
    testtranslation.splitcs();
    testtranslation.translate();
    testtranslation.bounddrawdown();
    testtranslation.targetdrawdown();

    BOOST_CHECK_EQUAL(response, true);
    BOOST_CHECK_EQUAL(testtranslation.CS, "1");
    testdate = boost::gregorian::date(2012,7,8);
    BOOST_CHECK_EQUAL(testtranslation.startdate, testdate);
    BOOST_CHECK_EQUAL(testtranslation.enddate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.week_vector[0].week_bs.to_string(), "0000001");
    testdate = boost::gregorian::date(2012,7,8);
    BOOST_CHECK_EQUAL(testtranslation.week_vector[0].startdate, testdate);


    testCS="000000000011110";
    testdate= boost::gregorian::date(2012,7,2);
    response = testtranslation.initcs(testdate, testCS);
    //test de la decoupe de la condition de service
    testtranslation.splitcs();
    testtranslation.translate();
    testtranslation.bounddrawdown();
    testtranslation.targetdrawdown();

    BOOST_CHECK_EQUAL(response, true);
    BOOST_CHECK_EQUAL(testtranslation.CS, "1111");
    testdate = boost::gregorian::date(2012,7,12);
    BOOST_CHECK_EQUAL(testtranslation.startdate, testdate);
    testdate = boost::gregorian::date(2012,7,15);
    BOOST_CHECK_EQUAL(testtranslation.enddate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.week_vector[0].week_bs.to_string(), "0001111");
    testdate = boost::gregorian::date(2012,7,12);
    BOOST_CHECK_EQUAL(testtranslation.week_vector[0].startdate, testdate);

    testCS="00010111001111001100";
    testdate= boost::gregorian::date(2012,7,16);
    response = testtranslation.initcs(testdate, testCS);
    //test de la decoupe de la condition de service
    testtranslation.splitcs();
    testtranslation.translate();
    testtranslation.bounddrawdown();
    testtranslation.targetdrawdown();

    BOOST_CHECK_EQUAL(response, true);
    BOOST_CHECK_EQUAL(testtranslation.CS, "101110011110011");
    testdate = boost::gregorian::date(2012,7,19);
    BOOST_CHECK_EQUAL(testtranslation.startdate, testdate);

    testdate = boost::gregorian::date(2012,8,2);
    BOOST_CHECK_EQUAL(testtranslation.enddate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.week_vector[0].week_bs.to_string(), "0001011");
    testdate = boost::gregorian::date(2012,7,19);
    BOOST_CHECK_EQUAL(testtranslation.week_vector[0].startdate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.week_vector[1].week_bs.to_string(), "1001111");
    testdate = boost::gregorian::date(2012,7,23);
    BOOST_CHECK_EQUAL(testtranslation.week_vector[1].startdate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.week_vector[2].week_bs.to_string(), "0011000");
    testdate = boost::gregorian::date(2012,7,30);
    BOOST_CHECK_EQUAL(testtranslation.week_vector[2].startdate, testdate);

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
    testtranslation.translate();
    testtranslation.bounddrawdown();
    testtranslation.targetdrawdown();

    BOOST_CHECK_EQUAL(testtranslation.week_vector[0].week_bs.to_string(), "1111111");
    testdate = boost::gregorian::date(2012,7,2);
    BOOST_CHECK_EQUAL(testtranslation.week_vector[0].startdate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.week_vector[1].week_bs.to_string(), "1111111");
    testdate = boost::gregorian::date(2012,7,9);
    BOOST_CHECK_EQUAL(testtranslation.week_vector[1].startdate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.week_vector[2].week_bs.to_string(), "1111111");
    testdate = boost::gregorian::date(2012,7,16);
    BOOST_CHECK_EQUAL(testtranslation.week_vector[2].startdate, testdate);


    MakeTranslation::target targetresponse;
//    for(std::map<int, MakeTranslation::target>::iterator it = testtranslation.target_map.begin(); it!= testtranslation.target_map.end(); it++) {
//        targetresponse = it->second;
//        std::cout << targetresponse.week_bs.to_string()<<std::endl;
//        for(std::vector<boost::gregorian::date>::iterator it2 = targetresponse.periodlist.begin(); it2!= targetresponse.periodlist.end(); it2++) {
//            std::cout << *it2 <<std::endl;;
//        }
//    }

    testCS="011111100000001111111";
    testdate= boost::gregorian::date(2012,7,2);
    response = testtranslation.initcs(testdate, testCS);
    //test de la decoupe de la condition de service
    testtranslation.splitcs();
    testtranslation.translate();
    testtranslation.bounddrawdown();
    testtranslation.targetdrawdown();

    BOOST_CHECK_EQUAL(response, true);
    BOOST_CHECK_EQUAL(testtranslation.CS, "11111100000001111111");
    testdate = boost::gregorian::date(2012,7,3);
    BOOST_CHECK_EQUAL(testtranslation.startdate, testdate);
    testdate = boost::gregorian::date(2012,7,22);
    BOOST_CHECK_EQUAL(testtranslation.enddate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.week_vector[0].week_bs.to_string(), "0111111");
    testdate = boost::gregorian::date(2012,7,3);
    BOOST_CHECK_EQUAL(testtranslation.week_vector[0].startdate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.week_vector[1].week_bs.to_string(), "0000000");
    testdate = boost::gregorian::date(2012,7,9);
    BOOST_CHECK_EQUAL(testtranslation.week_vector[1].startdate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.week_vector[2].week_bs.to_string(), "1111111");
    testdate = boost::gregorian::date(2012,7,16);
    BOOST_CHECK_EQUAL(testtranslation.week_vector[2].startdate, testdate);


//    for(std::map<int, MakeTranslation::target>::iterator it = testtranslation.target_map.begin(); it!= testtranslation.target_map.end(); it++) {
//        targetresponse = it->second;
//        std::cout<<std::endl<< targetresponse.week_bs.to_string()<<std::endl;
//        for(std::vector<boost::gregorian::date>::iterator it2 = targetresponse.periodlist.begin(); it2!= targetresponse.periodlist.end(); it2++) {
//            std::cout << *it2 <<std::endl;
//        }
//    }
//    testtranslation.bounddrawdown();



    testCS="0011011101110111011000";
    testdate= boost::gregorian::date(2012,7,2);
    response = testtranslation.initcs(testdate, testCS);
    //test de la decoupe de la condition de service
    testtranslation.splitcs();//0011011  1011101  101100  0
    testtranslation.translate();
    testtranslation.bounddrawdown();
    testtranslation.targetdrawdown();

    BOOST_CHECK_EQUAL(response, true);
    BOOST_CHECK_EQUAL(testtranslation.CS, "11011101110111011");
    testdate = boost::gregorian::date(2012,7,4);
    BOOST_CHECK_EQUAL(testtranslation.startdate, testdate);

    testdate = boost::gregorian::date(2012,7,20);
    BOOST_CHECK_EQUAL(testtranslation.enddate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.week_vector[0].week_bs.to_string(), "0011011");
    testdate = boost::gregorian::date(2012,7,4);
    BOOST_CHECK_EQUAL(testtranslation.week_vector[0].startdate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.week_vector[1].week_bs.to_string(), "1011101");
    testdate = boost::gregorian::date(2012,7,9);
    BOOST_CHECK_EQUAL(testtranslation.week_vector[1].startdate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.week_vector[2].week_bs.to_string(), "1101100");
    testdate = boost::gregorian::date(2012,7,16);
    BOOST_CHECK_EQUAL(testtranslation.week_vector[2].startdate, testdate);

//    std::cout <<std::endl<< "debut test : ";

//et le 07/jul
//et le 17/jul
//sauf  06/jul
//sauf  18/jul
    BOOST_CHECK_EQUAL(testtranslation.target_map.size(), 1);
    std::bitset<7> test_bitset("1011101");
    std::map<int, MakeTranslation::target>::iterator it= testtranslation.target_map.find(test_bitset.to_ulong());
    BOOST_CHECK_MESSAGE(it != testtranslation.target_map.end(), "Erreur MAP 1011101 non trouve");
    //test periode
    BOOST_CHECK_EQUAL(it->second.periodlist.size(), 2);
    testdate = boost::gregorian::date(2012,7,4);
    std::vector<boost::gregorian::date>::iterator itdst=find(it->second.periodlist.begin(), it->second.periodlist.end(), testdate);
    BOOST_CHECK_MESSAGE(itdst != it->second.periodlist.end(), "periode ne debute pas le 04/07/2012");

    testdate = boost::gregorian::date(2012,7,20);
    itdst=find(it->second.periodlist.begin(), it->second.periodlist.end(), testdate);
    BOOST_CHECK_MESSAGE(itdst != it->second.periodlist.end(), "periode ne finit pas le 20/07/2012");

    //test exception ET
    BOOST_CHECK_EQUAL(it->second.andlist.size(), 2);
    testdate = boost::gregorian::date(2012,7,7);
    itdst=find(it->second.andlist.begin(), it->second.andlist.end(), testdate);
    BOOST_CHECK_MESSAGE(itdst != it->second.andlist.end(), "l'exception ET le 07/07/2012 non presente");

    testdate = boost::gregorian::date(2012,7,17);
    itdst=find(it->second.andlist.begin(), it->second.andlist.end(), testdate);
    BOOST_CHECK_MESSAGE(itdst != it->second.andlist.end(), "l'exception ET le 17/07/2012 non presente");

    //test exception SAUF
    BOOST_CHECK_EQUAL(it->second.exceptlist.size(), 2);
    testdate = boost::gregorian::date(2012,7,6);
    itdst=find(it->second.exceptlist.begin(), it->second.exceptlist.end(), testdate);
    BOOST_CHECK_MESSAGE(itdst != it->second.exceptlist.end(), "l'exception SAUF le 06/07/2012 non presente");

    testdate = boost::gregorian::date(2012,7,18);
    itdst=find(it->second.exceptlist.begin(), it->second.exceptlist.end(), testdate);
    BOOST_CHECK_MESSAGE(itdst != it->second.exceptlist.end(), "l'exception SAUF le 18/07/2012 non presente");

    //test console

//    for(std::map<int, MakeTranslation::target>::iterator it = testtranslation.target_map.begin(); it!= testtranslation.target_map.end(); it++) {
//        targetresponse = it->second;
//        std::cout<<std::endl<< targetresponse.week_bs.to_string()<<std::endl;
//        std::cout << "liste des periodes : " <<std::endl;
//        for(std::vector<boost::gregorian::date>::iterator it2 = targetresponse.periodlist.begin(); it2!= targetresponse.periodlist.end(); it2++) {
//            std::cout << *it2 <<std::endl;
//        }
//        std::cout << "liste des ET : " <<std::endl;
//        for(std::vector<boost::gregorian::date>::iterator it3 = targetresponse.andlist.begin(); it3!= targetresponse.andlist.end(); it3++) {
//            std::cout << *it3 <<std::endl;
//        }
//        std::cout << "liste des SAUF: " <<std::endl;
//        for(std::vector<boost::gregorian::date>::iterator it4 = targetresponse.exceptlist.begin(); it4!= targetresponse.exceptlist.end(); it4++) {
//            std::cout << *it4 <<std::endl;
//        }
//    }
//    std::cout << "fin test : " <<std::endl;

    //0001100
    //1101100   //lundi 02/07/2012
    //1100100
    //1101100
    //1111100
    //1100000

    //000110011011001100100110110011111001100000
    testCS="000110011011001100100110110011111001100000";
    testdate= boost::gregorian::date(2012,6,25);

    response = testtranslation.initcs(testdate, testCS);
    testtranslation.splitcs();
    testtranslation.translate();
    testtranslation.bounddrawdown();
    testtranslation.targetdrawdown();

    BOOST_CHECK_EQUAL(response, true);
    BOOST_CHECK_EQUAL(testtranslation.CS, "1100110110011001001101100111110011");
    testdate = boost::gregorian::date(2012,6,28);
    BOOST_CHECK_EQUAL(testtranslation.startdate, testdate);

    testdate = boost::gregorian::date(2012,7,31);
    BOOST_CHECK_EQUAL(testtranslation.enddate, testdate);

    BOOST_CHECK_EQUAL(testtranslation.target_map.size(), 1);
    std::bitset<7> test_bitset2("1101100");
    it= testtranslation.target_map.find(test_bitset2.to_ulong());
    BOOST_CHECK_MESSAGE(it != testtranslation.target_map.end(), "Erreur MAP 1101100 non trouve");
    //test periode
    BOOST_CHECK_EQUAL(it->second.periodlist.size(), 2);
    testdate = boost::gregorian::date(2012,6,28);
    itdst=find(it->second.periodlist.begin(), it->second.periodlist.end(), testdate);
    BOOST_CHECK_MESSAGE(itdst != it->second.periodlist.end(), "periode ne debute pas le 28/06/2012");

    testdate = boost::gregorian::date(2012,7,31);
    itdst=find(it->second.periodlist.begin(), it->second.periodlist.end(), testdate);
    BOOST_CHECK_MESSAGE(itdst != it->second.periodlist.end(), "periode ne finit pas le 31/07/2012");

    //test exception ET
    BOOST_CHECK_EQUAL(it->second.andlist.size(), 1);
    testdate = boost::gregorian::date(2012,7,25);
    itdst=find(it->second.andlist.begin(), it->second.andlist.end(), testdate);
    BOOST_CHECK_MESSAGE(itdst != it->second.andlist.end(), "l'exception ET le 25/07/2012 non presente");

    //test exception SAUF
    BOOST_CHECK_EQUAL(it->second.exceptlist.size(), 1);
    testdate = boost::gregorian::date(2012,7,12);
    itdst=find(it->second.exceptlist.begin(), it->second.exceptlist.end(), testdate);
    BOOST_CHECK_MESSAGE(itdst != it->second.exceptlist.end(), "l'exception SAUF le 12/07/2012 non presente");

    //test console
//    std::cout <<std::endl<< "debut test : ";
//    for(std::map<int, MakeTranslation::target>::iterator it = testtranslation.target_map.begin(); it!= testtranslation.target_map.end(); it++) {
//        targetresponse = it->second;
//        std::cout<<std::endl<< targetresponse.week_bs.to_string()<<std::endl;
//        std::cout << "liste des periodes : " <<std::endl;
//        for(std::vector<boost::gregorian::date>::iterator it2 = targetresponse.periodlist.begin(); it2!= targetresponse.periodlist.end(); it2++) {
//            std::cout << *it2 <<std::endl;
//        }
//        std::cout << "liste des ET : " <<std::endl;
//        for(std::vector<boost::gregorian::date>::iterator it3 = targetresponse.andlist.begin(); it3!= targetresponse.andlist.end(); it3++) {
//            std::cout << *it3 <<std::endl;
//        }
//        std::cout << "liste des SAUF: " <<std::endl;
//        for(std::vector<boost::gregorian::date>::iterator it4 = targetresponse.exceptlist.begin(); it4!= targetresponse.exceptlist.end(); it4++) {
//            std::cout << *it4 <<std::endl;
//        }
//    }
//    std::cout << "fin test : " <<std::endl;
}
