#include "baseworker.h"
#include "utils/configuration.h"
#include <iostream>
#include "type/data.h"
#include "type/type.pb.h"
#include "type/pb_converter.h"
#include "routing/routing.h"
#include "routing/raptor.h"
#include "first_letter/firstletter_api.h"
#include "street_network/street_network_api.h"
#include "proximity_list/proximitylist_api.h"
#include "ptreferential/ptreferential.h"
#include <boost/tokenizer.hpp>
#include "utils/locker.h"

using namespace webservice;

namespace nt = navitia::type;



class Worker : public BaseWorker<navitia::type::Data> {

    std::unique_ptr<navitia::routing::raptor::RAPTOR> calculateur;

    
    /**
     * Vérifie la validité des paramétres, charge les données si elles ne le sont pas, 
     * et gére le cas des données en cours de chargement
     *
     * Retourne le Locker associé à la requéte, si il est bien vérouillé, le traitement peux continuer
     */
    navitia::utils::Locker check_and_init(RequestData & request, navitia::type::Data & d,
            pbnavitia::API requested_api, pbnavitia::Response& pb_response, ResponseData& rd) {
        pb_response.set_requested_api(requested_api);
        if(!request.params_are_valid){
            rd.status_code = 400;
            rd.content_type = "application/octet-stream";
            pb_response.set_info("invalid argument");
            pb_response.SerializeToOstream(&rd.response);
            return navitia::utils::Locker();
        }
        if(!d.loaded){
            try{
                this->load(d);
            }catch(...){
                rd.status_code = 500;
                rd.content_type = "application/octet-stream";
                pb_response.set_error("error while loading data");
                pb_response.SerializeToOstream(&rd.response);
                return navitia::utils::Locker();
            }
        }
        navitia::utils::Locker locker(d);
        if(!locker.locked){
            //on est en cours de chargement
            rd.status_code = 503;
            rd.content_type = "application/octet-stream";
            pb_response.set_error("loading");
            pb_response.SerializeToOstream(&rd.response);
            return navitia::utils::Locker();
        }
        return locker;
    }



    /**
     * méthode qui parse le paramètre filter afin de retourner une liste de Type_e
     */
    std::vector<nt::Type_e> parse_param_filter(const std::string& filter){
        std::vector<nt::Type_e> result;
        if(filter.empty()){//on utilise la valeur par défaut si pas de paramètre
            result.push_back(nt::eStopArea);
            result.push_back(nt::eCity);
            return result;
        }

        typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
        boost::char_separator<char> sep(";");
        tokenizer tokens(filter, sep);
        nt::static_data* sdata = nt::static_data::get();
        for(tokenizer::iterator tok_iter = tokens.begin();tok_iter != tokens.end(); ++tok_iter){
            try{
                result.push_back(sdata->typeByCaption(*tok_iter));
            }catch(std::out_of_range e){}
        }
        return result;
    }

    ResponseData firstletter(RequestData& request, navitia::type::Data &data){
        ResponseData rd;
        pbnavitia::Response pb_response;
        navitia::utils::Locker locker(check_and_init(request, data, pbnavitia::FIRSTLETTER, pb_response, rd));
        if(!locker.locked){
            return rd;
        }

        std::vector<nt::Type_e> filter = parse_param_filter(request.params["filter"]);
        std::string name = boost::get<std::string>(request.parsed_params["name"].value);

        try{
            pb_response = navitia::firstletter::firstletter(name, filter, data);
            pb_response.SerializeToOstream(&rd.response);
            rd.content_type = "application/octet-stream";
            rd.status_code = 200;
        }catch(...){
            rd.status_code = 500;
        }
        return rd;
    }

    ResponseData streetnetwork(RequestData & request, navitia::type::Data & data){
        ResponseData rd;
        pbnavitia::Response pb_response;
        navitia::utils::Locker locker(check_and_init(request, data, pbnavitia::STREET_NETWORK, pb_response, rd));
        if(!locker.locked){
            return rd;
        }

        nt::GeographicalCoord origin(boost::get<double>(request.parsed_params["startlon"].value), boost::get<double>(request.parsed_params["startlat"].value), true);
        nt::GeographicalCoord destination(boost::get<double>(request.parsed_params["destlon"].value), boost::get<double>(request.parsed_params["destlat"].value), true);

        pb_response = navitia::streetnetwork::street_network(origin, destination, data);
        pb_response.SerializeToOstream(&rd.response);
        rd.content_type = "application/octet-stream";
        rd.status_code = 200;

        return rd;
    }

    void load(navitia::type::Data & data){
        navitia::utils::Locker lock(data, true);
        try{
            log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
            Configuration * conf = Configuration::get();
            std::string database = conf->get_as<std::string>("GENERAL", "database", "IdF.nav");
            LOG4CPLUS_INFO(logger, "Chargement des données à partir du fichier " + database);
            data.loaded = true;
            data.load_lz4(database);
            data.build_proximity_list();
            calculateur = std::unique_ptr<navitia::routing::raptor::RAPTOR>(new navitia::routing::raptor::RAPTOR(data));
        }catch(...){
            data.loaded = false;
            throw;
        }
        
    }

    ResponseData load(RequestData, navitia::type::Data & d){
        log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
        pbnavitia::Response pb_response;
        pb_response.set_requested_api(pbnavitia::LOAD);
        ResponseData rd;
        rd.content_type = "application/octet-stream";
        try{
            load(d);
        }catch(...){
            pb_response.set_error("Erreur durant le chargement");
            rd.status_code = 500;
            pb_response.SerializeToOstream(&rd.response);               
            return rd;
        }
        pb_response.set_info("loaded!");
        pb_response.SerializeToOstream(&rd.response);               
        rd.status_code = 200;
        LOG4CPLUS_INFO(logger, "Chargement fini");
        return rd;
    }

    ResponseData proximitylist(RequestData & request, navitia::type::Data &data){
        pbnavitia::Response pb_response;
        ResponseData rd;
        navitia::utils::Locker locker(check_and_init(request, data, pbnavitia::PROXIMITYLIST, pb_response, rd));
        if(!locker.locked){
            return rd;
        }

        navitia::type::GeographicalCoord coord(boost::get<double>(request.parsed_params["lon"].value),
                                               boost::get<double>(request.parsed_params["lat"].value));
        double distance = 500;
        if(request.parsed_params.find("dist") != request.parsed_params.end())
            distance = boost::get<double>(request.parsed_params["dist"].value);

        std::vector<nt::Type_e> filter = parse_param_filter(request.params["filter"]);

        try{
            pb_response = navitia::proximitylist::find(coord, distance, filter, data);
            pb_response.SerializeToOstream(&rd.response);
            rd.content_type = "application/octet-stream";
            rd.status_code = 200;
        }catch(...){
            rd.status_code = 500;
        }
        return rd;
    }

    /**
     *  se charge de remplir l'objet protocolbuffer solution passé en paramètre
    */
    void create_pb_froute(navitia::routing::Path & path, const nt::Data & data, pbnavitia::FeuilleRoute &solution) {
        solution.set_nbchanges(path.nb_changes);
        solution.set_duree(path.duration);
        int i = 0;
        pbnavitia::Etape * etape;
        BOOST_FOREACH(navitia::routing::PathItem item, path.items) {
            if(i%2 == 0) {
                etape = solution.add_etapes();
                etape->mutable_mode()->set_ligne(data.pt_data.lines.at(item.line_idx).name);
                etape->mutable_depart()->mutable_lieu()->mutable_geo()->set_x(data.pt_data.stop_areas.at(item.said).coord.x);
                etape->mutable_depart()->mutable_lieu()->mutable_geo()->set_y(data.pt_data.stop_areas.at(item.said).coord.y);
                etape->mutable_depart()->mutable_lieu()->set_nom(data.pt_data.stop_areas.at(item.said).name);
                etape->mutable_depart()->mutable_date()->set_date(item.day);
                etape->mutable_depart()->mutable_date()->set_heure(item.time);
            } else {
                etape->mutable_arrivee()->mutable_lieu()->mutable_geo()->set_x(data.pt_data.stop_areas.at(item.said).coord.x);
                etape->mutable_arrivee()->mutable_lieu()->mutable_geo()->set_y(data.pt_data.stop_areas.at(item.said).coord.y);
                etape->mutable_arrivee()->mutable_lieu()->set_nom(data.pt_data.stop_areas.at(item.said).name);
                etape->mutable_arrivee()->mutable_date()->set_date(item.day);
                etape->mutable_arrivee()->mutable_date()->set_heure(item.time);
            }

            ++i;
        }

    }

    void create_pb_itineraire(navitia::routing::Path &path, const nt::Data & data, pbnavitia::ItineraireDetail &itineraire) {
        if(path.items.size() > 0) {
            pbnavitia::Trajet * trajet = itineraire.add_trajets();
            trajet->set_mode(0);
            trajet->set_ligne("");

            unsigned int precsaid = std::numeric_limits<unsigned int>::max();
            BOOST_FOREACH(navitia::routing::PathItem item, path.items) {
                if(item.said == precsaid) {
                    trajet = itineraire.add_trajets();
                    trajet->set_mode(0);
                    trajet->set_ligne("");
                }
                pbnavitia::Geocode * geo = trajet->add_pas();
                geo->set_x(data.pt_data.stop_areas.at(item.said).coord.x);
                geo->set_y(data.pt_data.stop_areas.at(item.said).coord.y);
                precsaid = item.said;

            }
        }
    }


    ResponseData planner(RequestData & request, navitia::type::Data & d) {
        ResponseData rd;
        pbnavitia::Response pb_response;
        navitia::utils::Locker locker(check_and_init(request, d, pbnavitia::PLANNER, pb_response, rd));
        if(!locker.locked){
            return rd;
        }

//        navitia::routing::Path path;
        int time = boost::get<int>(request.parsed_params["time"].value);
        int date = d.pt_data.validity_patterns.front().slide(boost::get<boost::gregorian::date>(request.parsed_params["date"].value));
//        if(request.parsed_params.count("departure") ==1) {
//            int departure_idx = boost::get<int>(request.parsed_params["departure"].value);
//            int arrival_idx = boost::get<int>(request.parsed_params["destination"].value);
//            path = calculateur->compute(departure_idx, arrival_idx, time, date);
//        } else {
            double departure_lat = boost::get<double>(request.parsed_params["departure_lat"].value);
            double departure_lon = boost::get<double>(request.parsed_params["departure_lon"].value);
            double arrival_lat = boost::get<double>(request.parsed_params["destination_lat"].value);
            double arrival_lon = boost::get<double>(request.parsed_params["destination_lon"].value);

            std::vector<navitia::routing::Path> pathes = calculateur->compute_all(navitia::type::GeographicalCoord(departure_lon, departure_lat), 300, navitia::type::GeographicalCoord(arrival_lon, arrival_lat), 300, time, 7);
//        }

            std::cout << "Nb itineraires : " << pathes.size() << std::endl;

        BOOST_FOREACH(auto path, pathes) {
            navitia::routing::Path itineraire = calculateur->makeItineraire(path);
            pbnavitia::Planning * planning = pb_response.mutable_planner()->add_planning();
            create_pb_froute(itineraire, d, *planning->mutable_feuilleroute());
            create_pb_itineraire(path, d, *planning->mutable_itineraire());
        }

        pb_response.SerializeToOstream(&rd.response);
        rd.content_type = "application/octet-stream";
        rd.status_code = 200;
        return rd;
    }

    ResponseData ptref(RequestData & request, navitia::type::Data &data){
        ResponseData rd;

        pbnavitia::Response pb_response;
        navitia::utils::Locker locker(check_and_init(request, data, pbnavitia::PTREFERENTIAL, pb_response, rd));
        if(!locker.locked){
            return rd;
        }
        try {
            std::string q = boost::get<std::string>(request.parsed_params["q"].value);
            pb_response = navitia::ptref::query(q, data.pt_data);
            pb_response.SerializeToOstream(&rd.response);
            rd.content_type = "application/octet-stream";
            rd.status_code = 200;
        }catch(...){
            rd.status_code = 500;
        }

        return rd;
    }

public:
    /** Constructeur par défaut
      *
      * On y enregistre toutes les api qu'on souhaite exposer
      */
    Worker(navitia::type::Data & ){
        register_api("streetnetwork", boost::bind(&Worker::streetnetwork, this, _1, _2), "Calcul d'itinéraire piéton");
        add_param("streetnetwork", "startlon", "Longitude en degrés", ApiParameter::DOUBLE, true);
        add_param("streetnetwork", "startlat", "Latitude en degrés", ApiParameter::DOUBLE, true);
        add_param("streetnetwork", "destlon", "Longitude en degrés", ApiParameter::DOUBLE, true);
        add_param("streetnetwork", "destlat", "Latitude en degrés", ApiParameter::DOUBLE, true);

        register_api("firstletter", boost::bind(&Worker::firstletter, this, _1, _2), "Retrouve les objets dont le nom commence par certaines lettres");
        add_param("firstletter", "name", "Valeur recherchée", ApiParameter::STRING, true);
        std::vector<RequestParameter::Parameter_variant> params;
        params.push_back("stop_areas");
        params.push_back("cities");
        add_param("firstletter", "filter", "Type à rechercher", ApiParameter::STRING, false, params);

        register_api("proximitylist", boost::bind(&Worker::proximitylist, this, _1, _2), "Liste des objets à proxmité");
        add_param("proximitylist", "lon", "Longitude en degrés", ApiParameter::DOUBLE, true);
        add_param("proximitylist", "lat", "Latitude en degrés", ApiParameter::DOUBLE, true);
        add_param("proximitylist", "dist", "Distance maximale, 500m par défaut", ApiParameter::DOUBLE, false);
        std::vector<RequestParameter::Parameter_variant> default_params;
        default_params.push_back("stop_areas");
        default_params.push_back("stop_name");
        add_param("proximitylist", "filter", "Type à rechercher", ApiParameter::STRING, false, default_params);

        register_api("planner", boost::bind(&Worker::planner, this, _1, _2), "Calcul d'itinéraire en Transport en Commun");
        add_param("planner", "departure", "Point de départ", ApiParameter::INT, false);
        add_param("planner", "destination", "Point d'arrivée", ApiParameter::INT, false);
        add_param("planner", "departure_lat", "Latitude de départ", ApiParameter::DOUBLE, false);
        add_param("planner", "departure_lon", "Longitude de départ", ApiParameter::DOUBLE, false);
        add_param("planner", "destination_lat", "Latitude d'arrivée", ApiParameter::DOUBLE, false);
        add_param("planner", "destination_lon", "Longitude d'arrivée", ApiParameter::DOUBLE, false);

        add_param("planner", "time", "Heure de début de l'itinéraire", ApiParameter::TIME, true);
        add_param("planner", "date", "Date de début de l'itinéraire", ApiParameter::DATE, true);

        register_api("load", boost::bind(&Worker::load, this, _1, _2), "Api de chargement des données");

        register_api("ptref", boost::bind(&Worker::ptref, this, _1, _2), "Exploration du référentiel de transports en commun");
        add_param("ptref", "q", "Requête", ApiParameter::STRING, true);

        add_default_api();
    }

};



MAKE_WEBSERVICE(navitia::type::Data, Worker)

