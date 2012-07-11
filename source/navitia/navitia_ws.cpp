#include "baseworker.h"
#include "utils/configuration.h"
#include <iostream>
#include "type/data.h"
#include "type/type.pb.h"
#include "type/pb_converter.h"
#include "routing/routing.h"
#include "routing/time_dependent.h"
#include <boost/tokenizer.hpp>

using namespace webservice;

namespace nt = navitia::type;



class Worker : public BaseWorker<navitia::type::Data> {

    navitia::routing::timedependent::TimeDependent *td;
    /**
     * structure permettant de simuler un finaly afin
     * de délocké le mutex en sortant de la fonction appelante
     */
    struct Locker{
        /// indique si l'acquisition du verrou a réussi
        bool locked;
        nt::Data& data;
        Locker(navitia::type::Data& data) : data(data) { locked = data.load_mutex.try_lock_shared();}

        ~Locker() {
            if(locked){
                data.load_mutex.unlock_shared();
            }
        }
        private:
            Locker(const Locker&);
            Locker& operator=(const Locker&);

    };

    /**
     * se charge de remplir l'objet protocolbuffer firstletter passé en paramètre
     *
     */
    void create_pb(const std::vector<nt::idx_t>& result, const nt::Type_e type, const nt::Data& data, pbnavitia::FirstLetter& pb_fl){
        BOOST_FOREACH(nt::idx_t idx, result){
            pbnavitia::FirstLetterItem* item = pb_fl.add_items();
            google::protobuf::Message* child = NULL;
            switch(type){
                case nt::eStopArea:
                    child = item->mutable_stop_area();
                    navitia::fill_pb_object<nt::eStopArea>(idx, data, child, 2);
                    item->set_name(data.pt_data.stop_areas[idx].name);
                    item->set_uri(nt::EntryPoint::get_uri(data.pt_data.stop_areas[idx]));
                    break;
                case nt::eCity:
                    child = item->mutable_city();
                    navitia::fill_pb_object<nt::eCity>(idx, data, child);
                    item->set_name(data.pt_data.cities[idx].name);
                    item->set_uri(nt::EntryPoint::get_uri(data.pt_data.cities[idx]));
                    break;
                default:
                    break;
            }
        }
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

    ResponseData firstletter(RequestData& request, navitia::type::Data & d){
        ResponseData rd;
        pbnavitia::Response pb_response;
        pb_response.set_requested_api(pbnavitia::FIRSTLETTER);

        if(!request.params_are_valid){
            rd.status_code = 500;
            rd.content_type = "application/octet-stream";
            pb_response.set_error("invalid argument");
            pb_response.SerializeToOstream(&rd.response);
            return rd;
        }

        if(!d.loaded){
            this->load(request, d);
        }
        Locker locker(d);
        if(!locker.locked){
            //on est en cours de chargement
            rd.status_code = 500;
            rd.content_type = "application/octet-stream";
            pb_response.set_error("loading");
            pb_response.SerializeToOstream(&rd.response);
            return rd;
        }

        std::string name;
        std::vector<nt::Type_e> filter = parse_param_filter(request.params["filter"]);
        name = boost::get<std::string>(request.parsed_params["name"].value);
        std::vector<nt::idx_t> result;
        try{
            pbnavitia::FirstLetter* pb = pb_response.mutable_firstletter();
            BOOST_FOREACH(nt::Type_e type, filter){
                switch(type){
                case nt::eStopArea:
                    result = d.pt_data.stop_area_first_letter.find(name); break;
                case nt::eCity:
                    result = d.pt_data.city_first_letter.find(name); break;
                default: break;
                }
                create_pb(result, type, d, *pb);
            }
            //pb_response.set_firstletter(pb);
            pb_response.SerializeToOstream(&rd.response);
            rd.content_type = "application/octet-stream";
            rd.status_code = 200;
        }catch(...){
            rd.status_code = 500;
        }
        return rd;
    }

    ResponseData streetnetwork(RequestData & request, navitia::type::Data & d){
        ResponseData rd;
        pbnavitia::Response pb_response;
        pb_response.set_requested_api(pbnavitia::STREET_NETWORK);
        if(!request.params_are_valid){
            rd.status_code = 500;
            rd.content_type = "application/octet-stream";
            pb_response.set_error("invalid argument");
            pb_response.SerializeToOstream(&rd.response);
            return rd;
        }
        if(!d.loaded){
            this->load(request, d);
        }
        Locker locker(d);
        if(!locker.locked){
            //on est en cours de chargement
            rd.status_code = 500;
            rd.content_type = "application/octet-stream";
            pb_response.set_error("loading");
            pb_response.SerializeToOstream(&rd.response);
            return rd;
        }

        double startlon = boost::get<double>(request.parsed_params["startlon"].value);
        double startlat = boost::get<double>(request.parsed_params["startlat"].value);
        double destlon = boost::get<double>(request.parsed_params["destlon"].value);
        double destlat = boost::get<double>(request.parsed_params["destlat"].value);

        std::vector<navitia::streetnetwork::vertex_t> start = {d.street_network.pl.find_nearest(startlon, startlat)};
        std::vector<navitia::streetnetwork::vertex_t> dest = {d.street_network.pl.find_nearest(destlon, destlat)};
        navitia::streetnetwork::Path path = d.street_network.compute(start, dest);

        pbnavitia::StreetNetwork* sn = pb_response.mutable_street_network();
        sn->set_length(path.length);
        BOOST_FOREACH(auto item, path.path_items){
            if(item.way_idx < d.street_network.ways.size()){
                pbnavitia::PathItem * path_item = sn->add_path_item_list();
                path_item->set_name(d.street_network.ways[item.way_idx].name);
                path_item->set_length(item.length);
            }else{
                std::cout << "Way étrange : " << item.way_idx << std::endl;
            }

        }

        BOOST_FOREACH(auto coord, path.coordinates){
            pbnavitia::GeographicalCoord * pb_coord = sn->add_coordinate_list();
            pb_coord->set_x(coord.x);
            pb_coord->set_y(coord.y);
        }
        pb_response.SerializeToOstream(&rd.response);
        rd.content_type = "application/octet-stream";
        rd.status_code = 200;

        return rd;
    }

    ResponseData load(RequestData, navitia::type::Data & d){
        log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
        Configuration * conf = Configuration::get();
        std::string database = conf->get_as<std::string>("GENERAL", "database", "/home/vlara/navitia/jeu/poitiers/poitiers.nav");
        LOG4CPLUS_INFO(logger, "Chargement des données à partir du fichier " + database);
        ResponseData rd;
        d.load_mutex.lock();
        d.loaded = true;
        d.load_lz4(database);
        d.load_mutex.unlock();
        td = new navitia::routing::timedependent::TimeDependent(d.pt_data);
        td->build_graph();
        rd.response << "loaded!";
        rd.content_type = "text/html";
        rd.status_code = 200;
        LOG4CPLUS_INFO(logger, "Chargement fini");
        return rd;
    }

    ResponseData proximitylist(RequestData & , navitia::type::Data & ){
        ResponseData rd;
        return rd;
    }

    /**
     *  se charge de remplir l'objet protocolbuffer solution passé en paramètre
    */
    void create_pb_solution(navitia::routing::Path & path, const nt::Data & data, pbnavitia::Planner &solution) {
        solution.set_nbchanges(path.nb_changes);
        solution.set_duree(path.duration);
        int i =0;
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


    ResponseData planner(RequestData & request, navitia::type::Data & d){
        ResponseData rd;
        pbnavitia::Response pb_response;
        pb_response.set_requested_api(pbnavitia::PLANNER);
        if(!request.params_are_valid){
            rd.status_code = 500;
            rd.content_type = "application/octet-stream";
            pb_response.set_error("invalid argument");
            pb_response.SerializeToOstream(&rd.response);
            return rd;
        }
        if(!d.loaded){
            this->load(request, d);
        }
        Locker locker(d);
        if(!locker.locked){
            //on est en cours de chargement
            rd.status_code = 500;
            rd.content_type = "application/octet-stream";
            pb_response.set_error("loading");
            pb_response.SerializeToOstream(&rd.response);
            return rd;
        }

        int departure_idx = boost::get<int>(request.parsed_params["departure"].value);
        int arrival_idx = boost::get<int>(request.parsed_params["destination"].value);
        int time = boost::get<int>(request.parsed_params["time"].value);
        int date = d.pt_data.validity_patterns.front().slide(boost::get<boost::gregorian::date>(request.parsed_params["date"].value));

        navitia::routing::Path path = td->compute(departure_idx, arrival_idx, time, date);
        navitia::routing::Path itineraire = td->makeItineraire(path);

        create_pb_solution(itineraire, d, *pb_response.mutable_planner());

        pb_response.SerializeToOstream(&rd.response);
        rd.content_type = "application/octet-stream";
        rd.status_code = 200;
        return rd;
    }

    public:
    /** Constructeur par défaut
      *
      * On y enregistre toutes les api qu'on souhaite exposer
      */
    Worker(navitia::type::Data & ) {
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
        add_param("proximitylist", "dist", "Distance maximale, 100m par défaut", ApiParameter::DOUBLE, false);
        add_param("proximitylist", "filter", "Type à rechercher", ApiParameter::STRING, false, {"stop_area", "stop_name"});

        register_api("planner", boost::bind(&Worker::planner, this, _1, _2), "Calcul d'itinéraire en Transport en Commun");
        add_param("planner", "departure", "Point de départ", ApiParameter::INT, true);
        add_param("planner", "destination", "Point d'arrivée", ApiParameter::INT, true);
        add_param("planner", "time", "Heure de début de l'itinéraire", ApiParameter::TIME, true);
        add_param("planner", "date", "Date de début de l'itinéraire", ApiParameter::DATE, true);

        register_api("load", boost::bind(&Worker::load, this, _1, _2), "Api de chargement des données");


        add_default_api();
    }
~Worker() {
    delete td;
}
};

MAKE_WEBSERVICE(navitia::type::Data, Worker)

