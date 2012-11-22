#include "config.h"
#include "baseworker.h"
#include "utils/configuration.h"
#include "type/data.h"
#include "type/type.pb.h"
#include "type/pb_converter.h"
#include "interface/renderer.h"
#include "type/locker.h"

#include "routing/raptor_api.h"
#include "first_letter/firstletter_api.h"
#include "street_network/street_network_api.h"
#include "proximity_list/proximitylist_api.h"
#include "ptreferential/ptreferential.h"
#include "time_tables/next_departures.h"
#include "time_tables/departure_board.h"
#include "time_tables/line_schedule.h"

#include <boost/tokenizer.hpp>
#include <iostream>
//#include <gperftools/profiler.h>
using namespace webservice;

namespace nt = navitia::type;
namespace pt = boost::posix_time;
namespace bg = boost::gregorian;

class Worker : public BaseWorker<navitia::type::Data> {  

    std::unique_ptr<navitia::routing::raptor::RAPTOR> calculateur;
    std::unique_ptr<navitia::georef::StreetNetworkWorker> street_network_worker;

    log4cplus::Logger logger;

    pbnavitia::Response pb_response;
    boost::posix_time::ptime last_load_at;

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
    virtual void pre_compute(webservice::RequestData& request, nt::Data&){
        LOG4CPLUS_TRACE(logger, "Requête : "  + request.path + "?" + request.raw_params);
        pb_response.Clear();
    }

    ///gestion du format de sortie
    virtual void post_compute(webservice::RequestData& request, webservice::ResponseData& response){
        if(pb_response.IsInitialized()){
            navitia::render(request, response, pb_response);
        }
    }

    virtual void on_std_exception(const std::exception & e, RequestData &, ResponseData & rd, nt::Data &){
        LOG4CPLUS_FATAL(logger, boost::format("Erreur : %s") % e.what());
        rd.status_code = 500;
#ifdef DEBUG
        throw;
#endif
    }

    virtual void on_unknown_exception(RequestData &, ResponseData & rd, nt::Data &){
        LOG4CPLUS_FATAL(logger, "Erreur inconnue");
        rd.status_code = 500;
#ifdef DEBUG
        throw;
#endif
    }


    /**
     * méthode qui parse le paramètre filter afin de retourner une liste de Type_e
     */
    std::vector<nt::Type_e> parse_param_filter(const std::string& filter){
        std::vector<nt::Type_e> result;
        if(filter.empty()){//on utilise la valeur par défaut si pas de paramètre
            result.push_back(nt::Type_e::eStopArea);
            result.push_back(nt::Type_e::eCity);
            result.push_back(nt::Type_e::eAddress);
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

        nt::Locker locker(check_and_init(request, data, pbnavitia::FIRSTLETTER, rd));
        if(!locker.locked){
            return rd;
        }

        std::vector<nt::Type_e> filter = parse_param_filter(boost::get<std::string>(request.parsed_params["filter"].value));
        std::string name = boost::get<std::string>(request.parsed_params["name"].value);

        pb_response = navitia::firstletter::firstletter(name, filter, data);
        rd.status_code = 200;

        return rd;
    }

    ResponseData streetnetwork(RequestData & request, navitia::type::Data & data){
        ResponseData rd;

        nt::Locker locker(check_and_init(request, data, pbnavitia::STREET_NETWORK, rd));
        if(!locker.locked){
            return rd;
        }

        nt::GeographicalCoord origin(boost::get<double>(request.parsed_params["startlon"].value), boost::get<double>(request.parsed_params["startlat"].value), true);
        nt::GeographicalCoord destination(boost::get<double>(request.parsed_params["destlon"].value), boost::get<double>(request.parsed_params["destlat"].value), true);

        pb_response = navitia::streetnetwork::street_network(origin, destination, data);
        rd.status_code = 200;

        return rd;
    }

    void load(navitia::type::Data & d){
        //ProfilerStart("navitia.prof");
        try{
            navitia::type::Data data;
            Configuration * conf = Configuration::get();
            std::string database = conf->get_as<std::string>("GENERAL", "database", "IdF.nav");
            LOG4CPLUS_INFO(logger, "Chargement des données à partir du fichier " + database);
            data.loaded = true;
            data.load_lz4(database);
            data.build_raptor();
            LOG4CPLUS_TRACE(logger, "acquisition du lock");
            nt::Locker lock(d, true);
            LOG4CPLUS_TRACE(logger, "déplacement de data");
            d = std::move(data);
            LOG4CPLUS_TRACE(logger, "Chargement des donnés fini");

        }catch(...){
            d.loaded = false;
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

        nt::Locker locker(check_and_init(request, data, pbnavitia::PROXIMITYLIST, rd));
        if(!locker.locked){
            return rd;
        }

        navitia::type::GeographicalCoord coord(boost::get<double>(request.parsed_params["lon"].value),
                                               boost::get<double>(request.parsed_params["lat"].value));
        double distance = 500;
        if(request.parsed_params.find("dist") != request.parsed_params.end())
            distance = boost::get<double>(request.parsed_params["dist"].value);

        std::vector<nt::Type_e> filter = parse_param_filter(boost::get<std::string>(request.parsed_params["filter"].value));

        pb_response = navitia::proximitylist::find(coord, distance, filter, data);
        rd.status_code = 200;

        return rd;
    }



    ResponseData status(RequestData &, navitia::type::Data & data) {
        ResponseData rd;

        pb_response.set_requested_api(pbnavitia::STATUS);
        nt::Locker lock(data);
        if(!lock.locked){
            rd.status_code = 503;
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
        status->set_nb_threads(data.nb_threads);

        rd.status_code = 200;

        return rd;
    }


    ResponseData journeys(RequestData & request, navitia::type::Data & d, bool multiple_datetime) {
        ResponseData rd;

        nt::Locker locker(check_and_init(request, d, pbnavitia::PLANNER, rd));
        if(!locker.locked){
            return rd;
        }
        if(d.last_load_at != this->last_load_at || !calculateur){
            calculateur = std::unique_ptr<navitia::routing::raptor::RAPTOR>(new navitia::routing::raptor::RAPTOR(d));
            street_network_worker = std::unique_ptr<navitia::georef::StreetNetworkWorker>(new navitia::georef::StreetNetworkWorker(d.geo_ref));
            this->last_load_at = d.last_load_at;

            LOG4CPLUS_INFO(logger, "instanciation du calculateur");
        }

        navitia::type::EntryPoint departure = navitia::type::EntryPoint(boost::get<std::string>(request.parsed_params["origin"].value));
        navitia::type::EntryPoint destination = navitia::type::EntryPoint(boost::get<std::string>(request.parsed_params["destination"].value));

        bool clockwise = true;
        if(request.parsed_params.find("clockwise") != request.parsed_params.end())
            clockwise = boost::get<bool>(request.parsed_params["clockwise"].value);

        std::multimap<std::string, std::string> forbidden;
        for(auto key : {"line", "mode", "route"}){
            std::string req_str = std::string("forbidden") + key + "[]";
            if(request.parsed_params.find(req_str) != request.parsed_params.end()){
                for(auto external_code : boost::get<std::vector<std::string>>(request.parsed_params[req_str].value))
                    forbidden.insert(std::make_pair(key, external_code));
            }
        }

        std::vector<std::string> datetimes;
        if(multiple_datetime){
            datetimes = boost::get<std::vector<std::string>>(request.parsed_params["datetime[]"].value);
        } else {
            datetimes.push_back(boost::get<std::string>(request.parsed_params["datetime"].value));
        }
        pb_response = navitia::routing::raptor::make_response(*calculateur, departure, destination, datetimes, clockwise, forbidden, *street_network_worker);

        rd.status_code = 200;

        return rd;
    }

    ResponseData ptref(nt::Type_e type, RequestData & request, navitia::type::Data &data){
        ResponseData rd;

        nt::Locker locker(check_and_init(request, data, pbnavitia::PTREFERENTIAL, rd));
        if(!locker.locked){
            return rd;
        }

        std::string filters = boost::get<std::string>(request.parsed_params["filter"].value);
        int depth;
        if(request.parsed_params.find("depth") != request.parsed_params.end())
            depth= boost::get<int>(request.parsed_params["depth"].value);
        else
            depth = 1;
        pb_response = navitia::ptref::query_pb(type, filters, depth, data);
        rd.status_code = 200;

        return rd;
    }

    ResponseData next_departures(RequestData & request, navitia::type::Data &data){
        ResponseData rd;

        nt::Locker locker(check_and_init(request, data, pbnavitia::NEXT_DEPARTURES, rd));
        if(!locker.locked){
            return rd;
        }

        std::string filters = boost::get<std::string>(request.parsed_params["filter"].value);
        std::string datetime = boost::get<std::string>(request.parsed_params["datetime"].value);
        std::string max_date_time = boost::get<std::string>(request.parsed_params["max_datetime"].value);

        int nb_departures = std::numeric_limits<int>::max();
        if(request.parsed_params.find("nb_departures") != request.parsed_params.end())
            nb_departures= boost::get<int>(request.parsed_params["nb_departures"].value);
        else if(max_date_time == "")
            nb_departures = 10;

        int depth;
        if(request.parsed_params.find("depth") != request.parsed_params.end())
            depth= boost::get<int>(request.parsed_params["depth"].value);
        else
            depth = 1;

        pb_response = navitia::timetables::next_departures(filters, datetime, max_date_time, nb_departures, depth, data);
        rd.status_code = 200;

        return rd;
    }

    ResponseData departure_board(RequestData & request, navitia::type::Data &data){
        ResponseData rd;

        nt::Locker locker(check_and_init(request, data, pbnavitia::NEXT_DEPARTURES, rd));
        if(!locker.locked){
            return rd;
        }

        std::string departure_filter = boost::get<std::string>(request.parsed_params["departure_filter"].value);
        std::string arrival_filter = boost::get<std::string>(request.parsed_params["arrival_filter"].value);
        std::string datetime = boost::get<std::string>(request.parsed_params["datetime"].value);
        std::string max_date_time = boost::get<std::string>(request.parsed_params["max_datetime"].value);

        int nb_departures;
        if(request.parsed_params.find("nb_departures") != request.parsed_params.end())
            nb_departures= boost::get<int>(request.parsed_params["nb_departures"].value);
        else
            nb_departures = 10;
        int depth;
        if(request.parsed_params.find("depth") != request.parsed_params.end())
            depth= boost::get<int>(request.parsed_params["depth"].value);
        else
            depth = 1;

        pb_response = navitia::timetables::departure_board(departure_filter, arrival_filter, datetime, max_date_time, nb_departures, depth, data);
        rd.status_code = 200;

        return rd;
    }


    ResponseData line_schedule(RequestData & request, navitia::type::Data &data){
        ResponseData rd;

        nt::Locker locker(check_and_init(request, data, pbnavitia::LINE_SCHEDULE, rd));
        if(!locker.locked){
            return rd;
        }

        std::string line_externalcode = boost::get<std::string>(request.parsed_params["line_external_code"].value);
        std::string datetime = boost::get<std::string>(request.parsed_params["datetime"].value);
        std::string max_date_time = boost::get<std::string>(request.parsed_params["max_datetime"].value);

        int nb_departures = std::numeric_limits<int>::max();
        if(request.parsed_params.find("nb_departures") != request.parsed_params.end())
            nb_departures= boost::get<int>(request.parsed_params["nb_departures"].value);
        else if(max_date_time == "")
            nb_departures = 10;

        int depth;
        if(request.parsed_params.find("depth") != request.parsed_params.end())
            depth= boost::get<int>(request.parsed_params["depth"].value);
        else
            depth = 1;

        pb_response = navitia::timetables::line_schedule(line_externalcode, datetime, max_date_time, nb_departures, depth, data);
        rd.status_code = 200;

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

        register_api("journeys", boost::bind(&Worker::journeys, this, _1, _2, false), "Calcul d'itinéraire multimodal");
        add_param("journeys", "datetime", "Date et heure de début de l'itinéraire, au format ISO", ApiParameter::STRING, true);
        register_api("journeysarray", boost::bind(&Worker::journeys, this, _1, _2, true), "Calcul d'itinéraire multimodal avec plusieurs heures de départ");
        add_param("journeysarray", "datetime[]", "Tableau de dates-heure de début de l'itinéraire, au format ISO", ApiParameter::STRINGLIST, true);

        for(auto api : {"journeys", "journeysarray"}){
            add_param(api, "origin", "Point de départ", ApiParameter::STRING, true);
            add_param(api, "destination", "Point d'arrivée", ApiParameter::STRING, true);
            add_param(api, "clockwise", "Sens du calcul ; si 1 on veut partir après l'heure indiquée, si 0, on veut arriver avant [par défaut 1]", ApiParameter::BOOLEAN, false);
            add_param(api, "forbiddenline[]", "Lignes interdites identifiées par leur external code", ApiParameter::STRINGLIST, false);
            add_param(api, "forbiddenmode[]", "Modes interdites identifiées par leur external code", ApiParameter::STRINGLIST, false);
            add_param(api, "forbiddenroute[]", "Routes interdites identifiées par leur external code", ApiParameter::STRINGLIST, false);
        }

        register_api("load", boost::bind(&Worker::load, this, _1, _2), "Api de chargement des données");
        register_api("status", boost::bind(&Worker::status, this, _1, _2), "Api de monitoring");

        nt::static_data * static_data = nt::static_data::get();
        for(navitia::type::Type_e type : {nt::Type_e::eStopArea, nt::Type_e::eStopPoint, nt::Type_e::eLine, nt::Type_e::eRoute, nt::Type_e::eNetwork,
            nt::Type_e::eModeType, nt::Type_e::eMode, nt::Type_e::eConnection, nt::Type_e::eRoutePoint, nt::Type_e::eCompany}){
            std::string str = static_data->captionByType(type) + "s";
            register_api(str /*+ "s"*/, boost::bind(&Worker::ptref, this, type, _1, _2), "Liste de " + str);
            add_param(str, "filter", "Conditions pour restreindre les objets retournés", ApiParameter::STRING, false);
            add_param(str, "depth", "Profondeur maximale pour les objets", ApiParameter::INT, false);
        }

        register_api("next_departures", boost::bind(&Worker::next_departures, this, _1, _2), "Renvoie les prochains départs");
        add_param("next_departures", "filter", "Conditions pour restreindre les départs retournés", ApiParameter::STRING, false);
        add_param("next_departures", "datetime", "Date à partir de laquelle on veut les prochains départs (au format iso)", ApiParameter::STRING, true);
        add_param("next_departures", "max_datetime", "Date à partir de laquelle on veut les prochains départs (au format iso)", ApiParameter::STRING, false);
        add_param("next_departures", "nb_departures", "Nombre maximum de départ souhaités", ApiParameter::INT, false);
        add_param("next_departures", "depth", "Profondeur maximale pour les objets", ApiParameter::INT, false);


        register_api("departure_board", boost::bind(&Worker::departure_board, this, _1, _2), "Renvoie le tableau depart/arrivee entre deux filtres");
        add_param("departure_board", "departure_filter", "Conditions pour restreindre les départs retournés", ApiParameter::STRING, false);
        add_param("departure_board", "arrival_filter", "Conditions pour restreindre les départs retournés", ApiParameter::STRING, false);
        add_param("departure_board", "datetime", "Date à partir de laquelle on veut les prochains départs (au format iso)", ApiParameter::STRING, true);
        add_param("departure_board", "max_datetime", "Date à partir de laquelle on veut les prochains départs (au format iso)", ApiParameter::STRING, false);
        add_param("departure_board", "nb_departures", "Nombre maximum de départ souhaités", ApiParameter::INT, false);
        add_param("departure_board", "depth", "Profondeur maximale pour les objets", ApiParameter::INT, false);

        register_api("line_schedule", boost::bind(&Worker::line_schedule, this, _1, _2), "Renvoie la fiche horaire de la ligne demandée");
        add_param("line_schedule", "line_external_code", "La ligne dont on veut les horaires", ApiParameter::STRING, false);
        add_param("line_schedule", "datetime", "Date à partir de laquelle on veut les prochains départs (au format iso)", ApiParameter::STRING, true);
        add_param("line_schedule", "max_datetime", "Date à partir de laquelle on veut les prochains départs (au format iso)", ApiParameter::STRING, false);
        add_param("line_schedule", "nb_departures", "Nombre maximum de départ souhaités", ApiParameter::INT, false);
        add_param("line_schedule", "depth", "Profondeur maximale pour les objets", ApiParameter::INT, false);



        add_default_api();
    }

};



MAKE_WEBSERVICE(navitia::type::Data, Worker)

