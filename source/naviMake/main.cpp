#include "config.h"
#include <iostream>
#include <boost/foreach.hpp>

#include "connectors.h"
#include "gtfs_parser.h"
#include "bdtopo_parser.h"

#include "utils/timer.h"

#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

namespace po = boost::program_options;
namespace pt = boost::posix_time;

int main(int argc, char * argv[])
{
    std::string type, input, output, date, topo_path;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Affiche l'aide")
        ("type,t", po::value<std::string>(&type), "Type du format d'entrée [fusio, gtfs]")
        ("date,d", po::value<std::string>(&date), "Date de début")
        ("input,i", po::value<std::string>(&input), "Repertoire d'entrée")
        ("topo", po::value<std::string>(&topo_path), "Repertoire contenant la bd topo")
        ("output,o", po::value<std::string>(&output)->default_value("data.nav"), "Fichier de sortie")
        ("version,v", "Affiche la version");


    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if(vm.count("version")){
        std::cout << argv[0] << " V" << NAVITIA_VERSION << " " << NAVITIA_BUILD_TYPE << std::endl;
        return 0;
    }

    if(vm.count("help") || !vm.count("input") || !vm.count("type")) {
        std::cout << desc <<  "\n";
        return 1;
    }
    if(!vm.count("topo")) {
        std::cout << "Pas de topologie chargee" << std::endl;
    }
    pt::ptime start, end;
    int read, clean, sort, transform, save, first_letter, sn;

    navimake::Data data; // Structure temporaire
    navitia::type::Data nav_data; // Structure définitive


    nav_data.meta.publication_date = pt::microsec_clock::local_time();
    
    // Est-ce que l'on charge la carto ?
    start = pt::microsec_clock::local_time();
    if(vm.count("topo")){
        nav_data.meta.data_sources.push_back(boost::filesystem::absolute(topo_path).native());

        navimake::connectors::BDTopoParser topo_parser(topo_path);
        //gtfs ne contient pas le référentiel des villes, on le charges depuis la BDTOPO
        topo_parser.load_city(data);        
        topo_parser.load_georef(nav_data.geo_ref);
        nav_data.set_cities(); // Assigne les villes aux voiries du filaire
    }
    sn = (pt::microsec_clock::local_time() - start).total_milliseconds();


    start = pt::microsec_clock::local_time();

    nav_data.meta.data_sources.push_back(boost::filesystem::absolute(input).native());

    if(type == "fusio") {
        navimake::connectors::CsvFusio connector(input);
        connector.fill(data);
    }
    else if(type == "gtfs") {
        navimake::connectors::GtfsParser connector(input);
        connector.fill(data);
        nav_data.meta.production_date = connector.production_date;
    }
    else {
        std::cout << desc << "\n";
        return 1;
    }
    read = (pt::microsec_clock::local_time() - start).total_milliseconds();


    std::cout << "line: " << data.lines.size() << std::endl;
    std::cout << "route: " << data.routes.size() << std::endl;
    std::cout << "stoparea: " << data.stop_areas.size() << std::endl;
    std::cout << "stoppoint: " << data.stop_points.size() << std::endl;
    std::cout << "vehiclejourney: " << data.vehicle_journeys.size() << std::endl;
    std::cout << "stop: " << data.stops.size() << std::endl;
    std::cout << "connection: " << data.connections.size() << std::endl;
    std::cout << "route points: " << data.route_points.size() << std::endl;
    std::cout << "city: " << data.cities.size() << std::endl;
    std::cout << "modes: " << data.modes.size() << std::endl;
    std::cout << "validity pattern : " << data.validity_patterns.size() << std::endl;

    start = pt::microsec_clock::local_time();
    data.clean();
    clean = (pt::microsec_clock::local_time() - start).total_milliseconds();

    start = pt::microsec_clock::local_time();
    data.sort();
    sort = (pt::microsec_clock::local_time() - start).total_milliseconds();

    start = pt::microsec_clock::local_time();
    data.transform(nav_data.pt_data);
    transform = (pt::microsec_clock::local_time() - start).total_milliseconds();


    start = pt::microsec_clock::local_time();
    nav_data.build_first_letter();
    nav_data.build_proximity_list();
    nav_data.build_external_code();
    std::cout << "On va construire les correspondances" << std::endl;
    {Timer t("Construction des correspondances");  nav_data.pt_data.build_connections();}
    first_letter = (pt::microsec_clock::local_time() - start).total_milliseconds();
    std::cout <<"Debut sauvegarde ..." << std::endl;
    start = pt::microsec_clock::local_time();

    nav_data.lz4(output);
    save = (pt::microsec_clock::local_time() - start).total_milliseconds();

    std::cout << "temps de traitement" << std::endl;
    std::cout << "\t lecture des fichiers " << read << "ms" << std::endl;
    std::cout << "\t netoyage des données " << clean << "ms" << std::endl;
    std::cout << "\t trie des données " << sort << "ms" << std::endl;
    std::cout << "\t transformation " << transform << "ms" << std::endl;
    std::cout << "\t street network " << sn << "ms" << std::endl;
    std::cout << "\t construction de firstletter " << first_letter << "ms" << std::endl;
    std::cout << "\t serialization " << save << "ms" << std::endl;
    
    return 0;
}
