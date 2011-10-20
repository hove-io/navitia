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

    struct Locker{
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
     * se charge de remplir l'objet protocolbuffer firstletter passé en paramétre
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
                    item->set_name(data.stop_areas[idx].name);
                    break;
                case nt::eCity:
                    child = item->mutable_city();
                    navitia::fill_pb_object<nt::eCity>(idx, data, child);
                    item->set_name(data.cities[idx].name);
                    break;
                default:
                    break;
            }
        }
    }
    
    /**
     * methode qui parse le parametre filter afin de retourner une liste de Type_e
     */
    std::vector<nt::Type_e> parse_param_filter(const std::string& filter){
        std::vector<nt::Type_e> result;
        if(filter.empty()){//on utilise la valeur par défaut si pas de paramétre
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
        if(!d.loaded){
            this->load(request, d);
        }
        Locker locker(d);
        if(!locker.locked){
            rd.status_code = 500;
            rd.content_type = "text/plain";
            rd.response << "loading";
            
        }

        std::string name;
        std::vector<nt::Type_e> filter = parse_param_filter(request.params["filter"]);
        name = request.params["name"];
        std::vector<nt::idx_t> result;
        try{
            pbnavitia::FirstLetter pb;
            BOOST_FOREACH(nt::Type_e type, filter){
                switch(type){
                case nt::eStopArea:
                    result = d.stop_area_first_letter.find(name);break;
                case nt::eCity:
                    result = d.city_first_letter.find(name);break;
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
        //attention c'est mal, pas de lock sur data
        ResponseData rd;
        d.load_mutex.lock();
        d.loaded = true;
        d.load_flz("data.nav.flz");
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
        register_api("/firstletter", boost::bind(&Worker::firstletter, this, _1, _2), "Api firstletter");
        register_api("/load", boost::bind(&Worker::load, this, _1, _2), "Api de chargement des données");
        add_default_api();
    }
};

MAKE_WEBSERVICE(navitia::type::Data, Worker)

