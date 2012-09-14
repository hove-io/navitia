#include "config.h"
#include "baseworker.h"
#include "utils/configuration.h"
#include <iostream>
#include "type/data.h"
#include "type/type.pb.h"
#include "type/pb_converter.h"
#include "routing/routing.h"
#include "routing/raptor.h"
#include "routing/raptor_api.h"

#include "first_letter/firstletter_api.h"
#include "street_network/street_network_api.h"
#include "proximity_list/proximitylist_api.h"
#include "ptreferential/ptreferential.h"
#include <boost/tokenizer.hpp>
#include "type/locker.h"
#include "interface/renderer.h"

using namespace webservice;

namespace nt = navitia::type;

namespace pt = boost::posix_time;
namespace bg = boost::gregorian;


class Worker : public BaseWorker<navitia::type::Data> {

    std::unique_ptr<navitia::routing::raptor::RAPTOR> calculateur;

    log4cplus::Logger logger;

    pbnavitia::Response pb_response;
    /**
     * Vérifie la validité des paramétres, charge les données si elles ne le sont pas,
     * et gére le cas des données en cours de chargement
     *
     * Retourne le Locker associé à la requéte, si il est bien vérouillé, le traitement peux continuer
     */
    nt::Locker check_and_init(RequestData & request, navitia::type::Data & d,
            pbnavitia::API requested_api, ResponseData& rd) {

        pb_response.set_requested_api(requested_api);
        if(!request.params_are_valid){
            rd.status_code = 400;
            rd.content_type = "application/octet-stream";
            pb_response.set_info("invalid argument");
            return nt::Locker();
        }
        if(!d.loaded){
            try{
                this->load(d);
            }catch(...){
                rd.status_code = 500;
                rd.content_type = "application/octet-stream";
                pb_response.set_error("error while loading data");
                return nt::Locker();
            }
        }
        nt::Locker locker(d);
        if(!locker.locked){
            //on est en cours de chargement
            rd.status_code = 503;
            rd.content_type = "application/octet-stream";
            pb_response.set_error("loading");
            return nt::Locker();
        }
        return locker;
    }

    ///netoyage pour le traitement
    virtual void pre_compute(webservice::RequestData&, nt::Data&){
        pb_response.Clear();
    }

    ///gestion du format de sortie
    virtual void post_compute(webservice::RequestData& request, webservice::ResponseData& response){
        navitia::render(request, response, pb_response);
    }

    /**
     * méthode qui parse le paramètre filter afin de retourner une liste de Type_e
     */
    std::vector<nt::Type_e> parse_param_filter(const std::string& filter){
        std::vector<nt::Type_e> result;
        if(filter.empty()){//on utilise la valeur par défaut si pas de paramètre
            result.push_back(nt::Type_e::eStopArea);
            result.push_back(nt::Type_e::eCity);
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
#ifndef DEBUG
        try{
#endif
            nt::Locker locker(check_and_init(request, data, pbnavitia::FIRSTLETTER, rd));
            if(!locker.locked){
                return rd;
            }

            std::vector<nt::Type_e> filter = parse_param_filter(request.params["filter"]);
            std::string name = boost::get<std::string>(request.parsed_params["name"].value);

            pb_response = navitia::firstletter::firstletter(name, filter, data);
            rd.status_code = 200;

#ifndef DEBUG
        }catch(std::exception& e){
            LOG4CPLUS_FATAL(logger, boost::format("Erreur : %s") % e.what());
            rd.status_code = 500;
        }catch(...){
            LOG4CPLUS_FATAL(logger, "Erreur inconnue");
            rd.status_code = 500;
        }
#endif
        return rd;
    }

    ResponseData streetnetwork(RequestData & request, navitia::type::Data & data){
        ResponseData rd;
#ifndef DEBUG
        try{
#endif
            nt::Locker locker(check_and_init(request, data, pbnavitia::STREET_NETWORK, rd));
            if(!locker.locked){
                return rd;
            }

            nt::GeographicalCoord origin(boost::get<double>(request.parsed_params["startlon"].value), boost::get<double>(request.parsed_params["startlat"].value), true);
            nt::GeographicalCoord destination(boost::get<double>(request.parsed_params["destlon"].value), boost::get<double>(request.parsed_params["destlat"].value), true);

            pb_response = navitia::streetnetwork::street_network(origin, destination, data);
            rd.status_code = 200;

#ifndef DEBUG
        }catch(std::exception& e){
            LOG4CPLUS_FATAL(logger, boost::format("Erreur : %s") % e.what());
            rd.status_code = 500;
        }catch(...){
            LOG4CPLUS_FATAL(logger, "Erreur inconnue");
            rd.status_code = 500;
        }
#endif
        return rd;
    }

    void load(navitia::type::Data & data){
        nt::Locker lock(data, true);
        try{
            Configuration * conf = Configuration::get();
            std::string database = conf->get_as<std::string>("GENERAL", "database", "IdF.nav");
            LOG4CPLUS_INFO(logger, "Chargement des données à partir du fichier " + database);
            data.loaded = true;
            data.load_lz4(database);
            data.build_proximity_list();
            data.build_raptor();
            calculateur = std::unique_ptr<navitia::routing::raptor::RAPTOR>(new navitia::routing::raptor::RAPTOR(data));
            LOG4CPLUS_TRACE(logger, "Chargement des donnés fini");
        }catch(...){
            data.loaded = false;
            LOG4CPLUS_ERROR(logger, "erreur durant le chargement des données");
            throw;
        }

    }

    ResponseData load(RequestData, navitia::type::Data & d){
        pb_response.set_requested_api(pbnavitia::LOAD);
        ResponseData rd;
        try{
            load(d);
        }catch(...){
            pb_response.set_error("Erreur durant le chargement");
            rd.status_code = 500;
            return rd;
        }
        pb_response.set_info("loaded!");
        rd.status_code = 200;
        LOG4CPLUS_INFO(logger, "Chargement fini");
        return rd;
    }

    ResponseData proximitylist(RequestData & request, navitia::type::Data &data){
        ResponseData rd;
#ifndef DEBUG
        try{
#endif
            nt::Locker locker(check_and_init(request, data, pbnavitia::PROXIMITYLIST, rd));
            if(!locker.locked){
                return rd;
            }

            navitia::type::GeographicalCoord coord(boost::get<double>(request.parsed_params["lon"].value),
                    boost::get<double>(request.parsed_params["lat"].value));
            double distance = 500;
            if(request.parsed_params.find("dist") != request.parsed_params.end())
                distance = boost::get<double>(request.parsed_params["dist"].value);

            std::vector<nt::Type_e> filter = parse_param_filter(request.params["filter"]);

            pb_response = navitia::proximitylist::find(coord, distance, filter, data);
            rd.status_code = 200;

#ifndef DEBUG
        }catch(std::exception& e){
            LOG4CPLUS_FATAL(logger, boost::format("Erreur : %s") % e.what());
            rd.status_code = 500;
        }catch(...){
            LOG4CPLUS_FATAL(logger, "Erreur inconnue");
            rd.status_code = 500;
        }
#endif
        return rd;
    }



    ResponseData status(RequestData &, navitia::type::Data & data) {
        ResponseData rd;
#ifndef DEBUG
        try{
#endif
            nt::Locker lock(data);
            if(!lock.locked){
                return rd;
            }

            pbnavitia::Status* status = pb_response.mutable_status();
            status->set_publication_date(pt::to_iso_string(data.meta.publication_date));
            status->set_start_production_date(bg::to_iso_string(data.meta.production_date.begin()));
            status->set_end_production_date(bg::to_iso_string(data.meta.production_date.end()));

            for(auto data_sources: data.meta.data_sources){
                status->add_data_sources(data_sources);
            }
            status->set_data_version(data.version);
            status->set_navimake_version(data.meta.navimake_version);
            status->set_navitia_version(NAVITIA_VERSION);

            status->set_loaded(data.loaded);
            status->set_last_load_status(data.last_load);
            status->set_last_load_at(pt::to_iso_string(data.last_load_at));

            rd.status_code = 200;
#ifndef DEBUG
        }catch(std::exception& e){
            LOG4CPLUS_FATAL(logger, boost::format("Erreur : %s") % e.what());
            rd.status_code = 500;
        }catch(...){
            LOG4CPLUS_FATAL(logger, "Erreur inconnue");
            rd.status_code = 500;
        }
#endif
        return rd;
    }


    ResponseData planner(RequestData & request, navitia::type::Data & d) {
        ResponseData rd;
#ifndef DEBUG
        try{
#endif
            nt::Locker locker(check_and_init(request, d, pbnavitia::PLANNER, rd));
            if(!locker.locked){
                return rd;
            }

            std::vector<navitia::routing::Path> pathes;
            int time = boost::get<int>(request.parsed_params["time"].value);
            auto date = boost::get<boost::gregorian::date>(request.parsed_params["date"].value);

            try {
                navitia::routing::raptor::checkDateTime(time, date, d.meta.production_date);
            } catch(navitia::routing::raptor::badTime) {
                pb_response.set_error("Temps non valide");
                rd.status_code = 400;
                return rd;
            } catch(navitia::routing::raptor::badDate) {
                pb_response.set_error("Date non valide");
                rd.status_code = 400;
                return rd;
            }

            navitia::type::EntryPoint departure = navitia::type::EntryPoint(boost::get<std::string>(request.parsed_params["departure"].value));
            navitia::type::EntryPoint destination = navitia::type::EntryPoint(boost::get<std::string>(request.parsed_params["destination"].value));

            navitia::routing::senscompute sens = navitia::routing::inconnu;
            if(boost::get<std::string>(request.parsed_params["sens"].value) == "apres")
                sens = navitia::routing::partirapres;

            else if(boost::get<std::string>(request.parsed_params["sens"].value) == "avant")
                sens = navitia::routing::arriveravant;

            pathes = calculateur->compute_all(departure, destination, time, (date - d.meta.production_date.begin()).days(), sens);

            pb_response = navitia::routing::raptor::make_response(pathes, d);

            rd.status_code = 200;
#ifndef DEBUG
        }catch(std::exception& e){
            LOG4CPLUS_FATAL(logger, boost::format("Erreur : %s") % e.what());
        }catch(...){
            LOG4CPLUS_FATAL(logger, "Erreur inconnue");
        }
#endif
        return rd;
    }

    ResponseData ptref(RequestData & request, navitia::type::Data &data){
        ResponseData rd;

#ifndef DEBUG
        try{
#endif
            nt::Locker locker(check_and_init(request, data, pbnavitia::PTREFERENTIAL, rd));
            if(!locker.locked){
                return rd;
            }

            std::string q = boost::get<std::string>(request.parsed_params["q"].value);
            pb_response = navitia::ptref::query(q, data.pt_data);
            rd.status_code = 200;

#ifndef DEBUG
        }catch(std::exception& e){
            LOG4CPLUS_FATAL(logger, boost::format("Erreur : %s") % e.what());
            rd.status_code = 500;
        }catch(...){
            LOG4CPLUS_FATAL(logger, "Erreur inconnue");
            rd.status_code = 500;
        }
#endif
        return rd;
    }

    public:
    /** Constructeur par défaut
     *
     * On y enregistre toutes les api qu'on souhaite exposer
     */
    Worker(navitia::type::Data & ){
        logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));


        register_api("streetnetwork", boost::bind(&Worker::streetnetwork, this, _1, _2), "Calcul d'itinéraire piéton");
        add_param("streetnetwork", "startlon", "Longitude en degrés", ApiParameter::DOUBLE, true);
        add_param("streetnetwork", "startlat", "Latitude en degrés", ApiParameter::DOUBLE, true);
        add_param("streetnetwork", "destlon", "Longitude en degrés", ApiParameter::DOUBLE, true);
        add_param("streetnetwork", "destlat", "Latitude en degrés", ApiParameter::DOUBLE, true);

        register_api("firstletter", boost::bind(&Worker::firstletter, this, _1, _2), "Retrouve les objets dont le nom commence par certaines lettres");
        add_param("firstletter", "name", "Valeur recherchée", ApiParameter::STRING, true);
        add_param("firstletter", "filter", "Type à rechercher", ApiParameter::STRING, false);

        register_api("proximitylist", boost::bind(&Worker::proximitylist, this, _1, _2), "Liste des objets à proxmité");
        add_param("proximitylist", "lon", "Longitude en degrés", ApiParameter::DOUBLE, true);
        add_param("proximitylist", "lat", "Latitude en degrés", ApiParameter::DOUBLE, true);
        add_param("proximitylist", "dist", "Distance maximale, 500m par défaut", ApiParameter::DOUBLE, false);
        std::vector<RequestParameter::Parameter_variant> default_params;
        default_params.push_back("stop_areas");
        default_params.push_back("stop_name");
        add_param("proximitylist", "filter", "Type à rechercher", ApiParameter::STRING, false, default_params);

        register_api("planner", boost::bind(&Worker::planner, this, _1, _2), "Calcul d'itinéraire en Transport en Commun");
        add_param("planner", "departure", "Point de départ", ApiParameter::STRING, true);
        add_param("planner", "destination", "Point d'arrivée", ApiParameter::STRING, true);
        add_param("planner", "time", "Heure de début de l'itinéraire", ApiParameter::TIME, true);
        add_param("planner", "date", "Date de début de l'itinéraire", ApiParameter::DATE, true);

        add_param("planner", "sens", "Sens du calcul", ApiParameter::STRING, true);


        register_api("load", boost::bind(&Worker::load, this, _1, _2), "Api de chargement des données");
        register_api("status", boost::bind(&Worker::status, this, _1, _2), "Api de monitoring");

        register_api("ptref", boost::bind(&Worker::ptref, this, _1, _2), "Exploration du référentiel de transports en commun");
        add_param("ptref", "q", "Requête", ApiParameter::STRING, true);

        add_default_api();
    }

};



MAKE_WEBSERVICE(navitia::type::Data, Worker)

