#include "type/type.h"
#include "type/data.h"
#include "boost/random.hpp"
#include <iostream>
#include <fstream>
#include <string>

#include "boost/random/uniform_int.hpp"
#include <boost/program_options.hpp>


namespace po = boost::program_options;


int main(int argc, char * argv[]) {
    navitia::type::Data data;

    std::string date1, date2, lat, lon, dist;

    po::options_description desc("Allowed options");
    desc.add_options()
            ("date1,d1", po::value<std::string>(&date1), "Première date")
            ("date2,d2", po::value<std::string>(&date2), "Deuxième date")
            ("latitude, lat", po::value<std::string>(&lat), "Latitude")
            ("longitude, lon",  po::value<std::string>(&lon), "Longitude")
            ("distance, dist", po::value<std::string>(&dist), "Distance");


    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    std::cout << "Chargemement des données ... " << std::flush;

    data.load_lz4("/home/vlara/navitia/jeu/IdF/IdF.nav");

    std::cout << "Fin chargement" << std::endl;

    std::fstream file( "inputcsv", std::ios::out );
    std::string fday, sday;
    if(!vm.count("date1") || !vm.count("date2")) {
        fday = boost::gregorian::to_iso_string(data.pt_data.validity_patterns.at(0).beginning_date +boost::gregorian::date_duration(30));

        boost::gregorian::date_duration to_add((data.pt_data.validity_patterns.at(0).beginning_date.day_of_week().as_number() == 6) ? 41 : (47 -data.pt_data.validity_patterns.at(0).beginning_date.day_of_week().as_number()));

        sday =  boost::gregorian::to_iso_string(data.pt_data.validity_patterns.at(0).beginning_date + to_add);
    } else {
        fday = date1;
        sday = date2;
    }


    if(!vm.count("latitude")) {

        boost::mt19937 rng;
        boost::uniform_int<> sa(0,data.pt_data.stop_areas.size());
        boost::variate_generator<boost::mt19937&, boost::uniform_int<> > gen(rng, sa);

        std::cout << "premier jour " << fday << " deuxieme jour :  " << sday << std::endl;



        for(int i = 0; i < 100 ; ++i) {
            int a = gen();
            int b = gen();
            while(b == a ) {
                b = gen();
            }

            file << a << "," << b << "," << 0 << "," << fday << "\n";
            file << a << "," << b << "," << 28800 << "," << fday << "\n";
            file << a << "," << b << "," << 72000 << "," << fday << "\n";
            file << a << "," << b << "," << 86000 << "," << fday << "\n";

            file << a << "," << b << "," << 0 << "," <<  sday << "\n";
            file << a << "," << b << "," << 28800 << "," << sday << "\n";
            file << a << "," << b << "," << 72000 << "," << sday << "\n";
            file << a << "," << b << "," << 86000 << "," << sday << "\n";
        }
    } else {
        std::cout << "avec proximity list" << std::endl;
        navitia::type::GeographicalCoord geo(atof(lon.c_str()), atof(lat.c_str()));
        data.build_proximity_list();

        std::cout << "fin build " << std::endl;
        typedef std::pair<unsigned int, navitia::type::GeographicalCoord> retour_t;
        std::vector<retour_t> stop_areas = data.pt_data.stop_area_proximity_list.find_within(geo, atof(dist.c_str()));
        std::cout << "fin winthin " << stop_areas.size() << std::endl;

        boost::mt19937 rng;
        boost::uniform_int<> sa(0, stop_areas.size());
        boost::variate_generator<boost::mt19937&, boost::uniform_int<> > gen(rng, sa);
        for(int i = 0; i < 20 ; ++i) {
            int a = gen();
            int b = gen();
            while(b == a ) {
                b = gen();
            }

            file << stop_areas[a].first << "," << stop_areas[b].first << "," << 0 << "," << fday << "\n";
            file << stop_areas[a].first << "," << stop_areas[b].first << "," << 28800 << "," << fday << "\n";
            file << stop_areas[a].first << "," << stop_areas[b].first << "," << 72000 << "," << fday << "\n";
            file << stop_areas[a].first << "," << stop_areas[b].first << "," << 86000 << "," << fday << "\n";

            file << stop_areas[a].first << "," << stop_areas[b].first << "," << 0 << "," <<  sday << "\n";
            file << stop_areas[a].first << "," << stop_areas[b].first << "," << 28800 << "," << sday << "\n";
            file << stop_areas[a].first << "," << stop_areas[b].first << "," << 72000 << "," << sday << "\n";
            file << stop_areas[a].first << "," << stop_areas[b].first << "," << 86000 << "," << sday << "\n";
        }

        std::cout << "fin for" << std::endl;

    }

    file.close();
    return 0;

}
