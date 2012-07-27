#include "baseworker.h"
#include <iostream>
#include "type/data.h"
#include "type/type.pb.h"
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
    void create_pb(const std::vector<FirstLetter<nt::idx_t>::fl_quality>& result, const nt::Type_e type, const nt::Data& data, pbnavitia::FirstLetter& pb_fl){
        BOOST_FOREACH(auto result_item, result){
            pbnavitia::FirstLetterItem* item = pb_fl.add_items();
            google::protobuf::Message* child = NULL;
            switch(type){
                case nt::eStopArea:
                    child = item->mutable_stop_area();
                    navitia::fill_pb_object<nt::eStopArea>(result_item.idx, data, child, 2);
                    item->set_name(data.pt_data.stop_areas[result_item.idx].name);
                    item->set_uri(nt::EntryPoint::get_uri(data.pt_data.stop_areas[result_item.idx]));
                    item->set_quality(result_item.quality);
                    break;
                case nt::eCity:
                    child = item->mutable_city();
                    navitia::fill_pb_object<nt::eCity>(result_item.idx, data, child);
                    item->set_name(data.pt_data.cities[result_item.idx].name);
                    item->set_uri(nt::EntryPoint::get_uri(data.pt_data.cities[result_item.idx]));
                    item->set_quality(result_item.quality);
                    break;
                case nt::eStopPoint:
                    child = item->mutable_stop_point();
                    navitia::fill_pb_object<nt::eStopPoint>(result_item.idx, data, child, 2);
                    item->set_name(data.pt_data.stop_points[result_item.idx].name);
                    item->set_uri(nt::EntryPoint::get_uri(data.pt_data.stop_points[result_item.idx]));
                    item->set_quality(result_item.quality);
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
            result.push_back(nt::eStopPoint);
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
        std::vector<FirstLetter<nt::idx_t>::fl_quality> result;
        try{
            pbnavitia::FirstLetter pb;
            BOOST_FOREACH(nt::Type_e type, filter){
                switch(type){
                case nt::eStopArea:
                    result = d.pt_data.stop_area_first_letter.find_complete(name);break;
                case nt::eStopPoint:

                    result = d.pt_data.stop_point_first_letter.find_complete(name);
                    break;
                case nt::eCity:
                    result = d.pt_data.city_first_letter.find_complete(name);break;
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

    
    ResponseData load(RequestData, navitia::type::Data & d){
        log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
        Configuration * conf = Configuration::get();
        std::string database = conf->get_as<std::string>("GENERAL", "database", "data.flz");
        LOG4CPLUS_INFO(logger, "Chargement des données à partir du fichier " + database);
        ResponseData rd;
        d.load_mutex.lock();
        d.loaded = true;
        d.load_lz4(database);
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
        register_api("firstletter", boost::bind(&Worker::firstletter, this, _1, _2), "Api firstletter");
        add_param("firstletter", "name", "valeur recherché", ApiParameter::STRING, true);
        std::vector<RequestParameter::Parameter_variant> options;
        options.push_back("stop_area");
        options.push_back("city");
        options.push_back("stop_point");
        add_param("firstletter", "filter", "type à rechercher", ApiParameter::STRING, false, options);

        register_api("load", boost::bind(&Worker::load, this, _1, _2), "Api de chargement des données");
        add_default_api();
    }
};

MAKE_WEBSERVICE(navitia::type::Data, Worker)

