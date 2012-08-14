#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <fstream>
#include <iostream>

#include "type/type.h"
#include "type/data.h"
#include "naviMake/gtfs_parser.h"
#include "raptor.h"

typedef boost::tokenizer<boost::char_separator<char> > my_tok;


unsigned int getSaIdxFromName(std::string name, navitia::type::PT_Data &pt_data) {
    BOOST_FOREACH(navitia::type::StopArea sa, pt_data.stop_areas) {
        if(sa.name == name)
            return sa.idx;
    }
    return pt_data.stop_areas.size();
}

int main(int argc, char * argv[]) {
    if(argc !=3) {
        std::cout << "Mauvais nombre d'arguments " << std::endl;
        exit(1);
    }



    navimake::Data data; // Structure temporaire
    navitia::type::Data nav_data; // Structure définitive

    std::string input = std::string(argv[1]);
    std::string date = std::string(argv[2]);
    navimake::connectors::GtfsParser connector(input, date);
    connector.fill(data);
    data.clean();
    data.sort();
    data.transform(nav_data.pt_data);
    nav_data.build_first_letter();
    nav_data.build_proximity_list();
    nav_data.build_external_code();


    navitia::routing::raptor::RAPTOR raptor(nav_data);
    std::fstream ifile((input + "/_request.csv").c_str());
    std::string line;

    if(!getline(ifile, line)) {
        std::cerr << "Impossible d'ouvrir _request.csv" << std::endl;
        return 1;
    }
    while(getline(ifile, line)) {
        boost::char_separator<char> sepsemicol(";");
        boost::char_separator<char> sepsharp("#");
        boost::char_separator<char> seppipe("|");
        boost::char_separator<char> sepand("&");
        boost::char_separator<char> sepequal("=");
        my_tok fields1(line, sepsemicol);
        auto fields1iter = fields1.begin();
        ++ fields1iter;
        my_tok fields2(*fields1iter, sepsharp);

        auto fields2iter = fields2.begin();
        my_tok fields3(*fields2iter, seppipe);

        std::string nameSA1;
        int i = 0;
        for(auto fields3iter = fields3.begin();fields3iter != fields3.end() && i < 3; ++ fields3iter) {
            nameSA1 = *fields3iter;
            ++i;
        }

        ++fields2iter;
        my_tok fields31(*fields2iter, seppipe);
        std::string nameSA2;
        i=0;
        for(auto fields3iter = fields31.begin();fields3iter != fields3.end() && i <3; ++ fields3iter) {
            nameSA2 = *fields3iter;
            ++i;
        }

        ++fields2iter;
        my_tok fields4(*fields2iter, sepand);
        auto fields4iter = fields4.begin();
        my_tok field51(*fields4iter, sepequal);
        auto field5iter = field51.begin();
        ++field5iter;
        my_tok fields32(*field5iter, seppipe);
        auto fields3iter = fields32.begin();

        std::string ddate = *fields3iter;
        ++fields3iter;
        ddate += *fields3iter;
        ++fields3iter;
        ddate += *fields3iter;


        ++fields4iter;
        my_tok field52(*fields4iter, sepequal);
        field5iter = field52.begin();
        ++ field5iter;
        my_tok fields33(*field5iter, seppipe);
        fields3iter = fields33.begin();
        int dtime = boost::lexical_cast<int>(*fields3iter) * 3600;
        ++fields3iter;
        dtime += boost::lexical_cast<int>(*fields3iter) * 60;

        std::cout << "Request " << nameSA1 << " -> " << nameSA2 << " le " << ddate << " a " << dtime << std::endl;

        auto result = raptor.compute(getSaIdxFromName(nameSA1, nav_data.pt_data), getSaIdxFromName(nameSA2, nav_data.pt_data), dtime, nav_data.pt_data.validity_patterns.front().slide(boost::gregorian::from_undelimited_string(ddate)));
        std::cout << result << std::endl;
    }


}
