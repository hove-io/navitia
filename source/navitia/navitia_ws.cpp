#include "baseworker.h"
#include "configuration.h"
#include <iostream>
#include "type/data.h"
#include "type.pb.h"
#include "type/pb_converter.h"
#include <boost/tokenizer.hpp>

using namespace webservice;

namespace nt = navitia::type;

class Worker : public BaseWorker<navitia::type::Data> {

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

        if(!request.params_are_valid){
            rd.status_code = 500;
            rd.content_type = "text/plain";
            rd.response << "invalid argument";
            return rd;
        }

        if(!d.loaded){
            this->load(request, d);
        }
        Locker locker(d);
        if(!locker.locked){
            //on est en cours de chargement
            rd.status_code = 500;
            rd.content_type = "text/plain";
            rd.response << "loading";
            return rd;
        }

        std::string name;
        std::vector<nt::Type_e> filter = parse_param_filter(request.params["filter"]);
        name = boost::get<std::string>(request.parsed_params["name"].value);
        std::vector<nt::idx_t> result;
        try{
            pbnavitia::FirstLetter pb;
            BOOST_FOREACH(nt::Type_e type, filter){
                switch(type){
                case nt::eStopArea:
                    result = d.pt_data.stop_area_first_letter.find(name);break;
                case nt::eCity:
                    result = d.pt_data.city_first_letter.find(name);break;
                default: break;
                }
                create_pb(result, type, d, pb);
            }
            pb.SerializeToOstream(&rd.response);
            rd.content_type = "application/octet-stream";
            rd.status_code = 200;
        }catch(...){
            rd.status_code = 500;
        }
        return rd;
    }

    ResponseData streetnetwork(RequestData & request, navitia::type::Data & d){
        ResponseData rd;
        if(!request.params_are_valid){
            rd.status_code = 500;
            rd.content_type = "text/plain";
            rd.response << "invalid argument";
            return rd;
        }
        if(!d.loaded){
            this->load(request, d);
        }
        Locker locker(d);
        if(!locker.locked){
            //on est en cours de chargement
            rd.status_code = 500;
            rd.content_type = "text/plain";
            rd.response << "loading";
            return rd;
        }

        double startlon = boost::get<double>(request.parsed_params["startlon"].value);
        double startlat = boost::get<double>(request.parsed_params["startlat"].value);
        double destlon = boost::get<double>(request.parsed_params["destlon"].value);
        double destlat = boost::get<double>(request.parsed_params["destlat"].value);

        std::vector<navitia::streetnetwork::vertex_t> start = {d.street_network.pl.find_nearest(startlon, startlat)};
        std::vector<navitia::streetnetwork::vertex_t> dest = {d.street_network.pl.find_nearest(destlon, destlat)};
        navitia::streetnetwork::Path path = d.street_network.compute(start, dest);

        pbnavitia::StreetNetwork sn;
        sn.set_length(path.length);
        BOOST_FOREACH(auto item, path.path_items){
            if(item.way_idx < d.street_network.ways.size()){
                pbnavitia::PathItem * path_item = sn.add_path_item_list();
                path_item->set_name(d.street_network.ways[item.way_idx].name);
                path_item->set_length(item.length);
            }else{
                std::cout << "Way étrange : " << item.way_idx << std::endl;
            }

        }

        BOOST_FOREACH(auto coord, path.coordinates){
            pbnavitia::GeographicalCoord * pb_coord = sn.add_coordinate_list();
            pb_coord->set_x(coord.x);
            pb_coord->set_y(coord.y);
        }
        sn.SerializeToOstream(&rd.response);
        rd.content_type = "application/octet-stream";
        rd.status_code = 200;

        return rd;
    }

    ResponseData load(RequestData, navitia::type::Data & d){
        ResponseData rd;
        d.load_mutex.lock();
        d.loaded = true;
        d.load_flz("/home/tristram/data.nav");
        d.load_mutex.unlock();
        rd.response << "loaded!";
        rd.content_type = "text/html";
        rd.status_code = 200;
        return rd;
    }

    public:
    /** Constructeur par défaut
      *
      * On y enregistre toutes les api qu'on souhaite exposer
      */
    Worker(navitia::type::Data &){
        register_api("streetnetwork", boost::bind(&Worker::streetnetwork, this, _1, _2), "Retrouve les objets à proximité");
        add_param("streetnetwork", "startlon", "Longitude en degrés", ApiParameter::DOUBLE, true);
        add_param("streetnetwork", "startlat", "Latitude en degrés", ApiParameter::DOUBLE, true);
        add_param("streetnetwork", "destlon", "Longitude en degrés", ApiParameter::DOUBLE, true);
        add_param("streetnetwork", "destlat", "Latitude en degrés", ApiParameter::DOUBLE, true);

        register_api("firstletter", boost::bind(&Worker::firstletter, this, _1, _2), "Retrouve les objets dont le nom commence par certaines lettres");
        add_param("firstletter", "name", "Valeur recherchée", ApiParameter::STRING, true);
        add_param("firstletter", "filter", "Type à rechercher", ApiParameter::STRING, false, {"stop_areas", "cities"});

        register_api("load", boost::bind(&Worker::load, this, _1, _2), "Api de chargement des données");
        add_default_api();
    }
};

MAKE_WEBSERVICE(navitia::type::Data, Worker)

