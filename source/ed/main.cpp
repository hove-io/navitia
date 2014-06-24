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

#include "config.h"
#include <iostream>

#include "gtfs_parser.h"
#include "osm2nav.h"
#include "utils/timer.h"

#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "utils/exception.h"
#include "connectors/adjustit_connector.h"
#include "adapted.h"
#include "poi_parser.h"

namespace po = boost::program_options;
namespace pt = boost::posix_time;

int main(int argc, char * argv[])
{
    std::string input, output, date, topo_path, osm_filename, poi_path, alias_path;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Affiche l'aide")
        ("date,d", po::value<std::string>(&date), "Date de début")
        ("input,i", po::value<std::string>(&input), "Repertoire d'entrée")
        ("topo", po::value<std::string>(&topo_path), "Repertoire contenant la bd topo")
        ("osm", po::value<std::string>(&osm_filename), "Fichier OpenStreetMap au format pbf")
        ("output,o", po::value<std::string>(&output)->default_value("data.nav"), "Fichier de sortie")
        ("version,v", "Affiche la version")
        ("poi", po::value<std::string>(&poi_path), "Repertoire des fichiers POI et POIType au format txt")
        ("config-file", po::value<std::string>(), "chemin vers le fichier de configuration")
        ("at-connection-string", po::value<std::string>(), "parametres de connexion à la base de données : DRIVER=FreeTDS;SERVER=;UID=;PWD=;DATABASE=;TDS_Version=8.0;Port=1433;ClientCharset=UTF-8")
        ("alias",po::value<std::string>(&alias_path), "Repertoire des fichiers alias et synonymes au format txt pour autocompletion");


    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if(vm.count("version")){
        std::cout << argv[0] << " V" << NAVITIA_VERSION << " " << NAVITIA_BUILD_TYPE << std::endl;
        return 0;
    }


    if(vm.count("config-file")){
        std::ifstream stream;
        stream.open(vm["config-file"].as<std::string>());
        if(!stream.is_open()){
            throw navitia::exception("loading config file failed");
        }else{
            po::store(po::parse_config_file(stream, desc), vm);
        }
    }

    if(vm.count("help") || !vm.count("input")) {
        std::cout << desc <<  "\n";
        return 1;
    }

    po::notify(vm);

    if(!vm.count("topo") && !vm.count("osm")) {
        std::cout << "Pas de topologie chargee" << std::endl;
    }
    pt::ptime start, end;
    int read, complete, clean, sort, transform, save, autocomplete, sn, apply_adapted_duration;

    ed::Data data; // Structure temporaire
    navitia::type::Data nav_data; // Structure définitive


    nav_data.meta.publication_date = pt::microsec_clock::local_time();

    // Est-ce que l'on charge la carto ?
    start = pt::microsec_clock::local_time();
    if(vm.count("topo")){
        std::cerr << "Support BDTOPO désactivé" << std::endl;
    } else if(vm.count("osm")){
        navitia::georef::fill_from_osm(nav_data.geo_ref, osm_filename);
    }

    if (vm.count("poi")){
        navitia::georef::PoiParser poiparser(poi_path);
        poiparser.fill(nav_data.geo_ref);
        std::cout << "POI : " << nav_data.geo_ref.poi_map.size() << std::endl;
    }

    if (vm.count("alias")){
        navitia::georef::PoiParser aliasparser(alias_path);
        aliasparser.fill_alias_synonyme(nav_data.geo_ref);
        std::cout << "alias : " << nav_data.geo_ref.alias.size() << std::endl;
        std::cout << "synonymes : " << nav_data.geo_ref.synonymes.size() << std::endl;
    }

    sn = (pt::microsec_clock::local_time() - start).total_milliseconds();


    start = pt::microsec_clock::local_time();

    nav_data.meta.data_sources.push_back(boost::filesystem::absolute(input).native());


    ed::connectors::GtfsParser connector(input);
    connector.fill(data, date);
    nav_data.meta.production_date = connector.production_date;

    data.normalize_uri();
    read = (pt::microsec_clock::local_time() - start).total_milliseconds();


    std::cout << "line: " << data.lines.size() << std::endl;
    std::cout << "journey_pattern: " << data.journey_patterns.size() << std::endl;
    std::cout << "stoparea: " << data.stop_areas.size() << std::endl;
    std::cout << "stoppoint: " << data.stop_points.size() << std::endl;
    std::cout << "vehiclejourney: " << data.vehicle_journeys.size() << std::endl;
    std::cout << "stop: " << data.stops.size() << std::endl;
    std::cout << "connection: " << data.connections.size() << std::endl;
    std::cout << "journey_pattern points: " << data.journey_pattern_points.size() << std::endl;
    std::cout << "modes: " << data.physical_modes.size() << std::endl;
    std::cout << "validity pattern : " << data.validity_patterns.size() << std::endl;
    std::cout << "voies (rues) : " << nav_data.geo_ref.ways.size() << std::endl;


    start = pt::microsec_clock::local_time();
    if(vm.count("at-connection-string")){
        navitia::AtLoader::Config conf;
        conf.connect_string = vm["at-connection-string"].as<std::string>();
        navitia::AtLoader loader;
        std::map<std::string, std::vector<navitia::type::Message>> messages;
        messages = loader.load_disrupt(conf, pt::second_clock::local_time());
        std::cout << "nombre d'objet impactés: " << messages.size() << std::endl;
        ed::AtAdaptedLoader adapter;
        adapter.apply(messages, data);
    }
    apply_adapted_duration = (pt::microsec_clock::local_time() - start).total_milliseconds();

    start = pt::microsec_clock::local_time();
    data.complete();
    complete = (pt::microsec_clock::local_time() - start).total_milliseconds();

    start = pt::microsec_clock::local_time();
    data.clean();
    clean = (pt::microsec_clock::local_time() - start).total_milliseconds();


    start = pt::microsec_clock::local_time();
    data.sort();
    sort = (pt::microsec_clock::local_time() - start).total_milliseconds();

    start = pt::microsec_clock::local_time();
    data.transform(nav_data.pt_data);

    transform = (pt::microsec_clock::local_time() - start).total_milliseconds();

    // Début d'affectation des données administratives
    nav_data.set_admins();

    std::cout << "Construction des contours de la région" << std::endl;
    nav_data.meta.shape = data.compute_bounding_box(nav_data.pt_data);

    start = pt::microsec_clock::local_time();
    std::cout << "Construction de proximity list" << std::endl;
    nav_data.build_proximity_list();
    std::cout << "Construction de external code" << std::endl;
    //construction des map uri => idx
    nav_data.build_uri();

    std::cout << "Construction de first letter" << std::endl;
    nav_data.build_autocomplete();
    std::cout << "On va construire les correspondances" << std::endl;
    {Timer t("Construction des correspondances");  nav_data.pt_data.build_connections();}
    autocomplete = (pt::microsec_clock::local_time() - start).total_milliseconds();
    std::cout <<"Debut sauvegarde ..." << std::endl;

    start = pt::microsec_clock::local_time();


    nav_data.save(output);
    save = (pt::microsec_clock::local_time() - start).total_milliseconds();

    std::cout << "temps de traitement" << std::endl;
    std::cout << "\t lecture des fichiers " << read << "ms" << std::endl;
    std::cout << "\t application des données adaptées " << apply_adapted_duration << "ms" << std::endl;
    std::cout << "\t completion des données " << complete << "ms" << std::endl;
    std::cout << "\t netoyage des données " << clean << "ms" << std::endl;
    std::cout << "\t trie des données " << sort << "ms" << std::endl;
    std::cout << "\t transformation " << transform << "ms" << std::endl;
    std::cout << "\t street network " << sn << "ms" << std::endl;
    std::cout << "\t construction de autocomplete " << autocomplete << "ms" << std::endl;
    std::cout << "\t serialization " << save << "ms" << std::endl;

    return 0;
}
