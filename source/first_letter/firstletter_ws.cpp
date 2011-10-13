#include "baseworker.h"
#include "configuration.h"
#include <iostream>
#include "type/data.h"
#include "type.pb.h"
#include "type/pb_converter.h"

using namespace webservice;

namespace nt = navitia::type;

class Worker : public BaseWorker<navitia::type::Data> {

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

    ResponseData firstletter(RequestData& request, navitia::type::Data & d){
        ResponseData rd;
        std::string filter, name;
        filter = request.params["filter"];//TODO non géré
        name = request.params["name"];
        std::vector<nt::idx_t> result;
        try{
            pbnavitia::FirstLetter pb;
            result = d.stop_area_first_letter.find(name);
            create_pb(result, nt::eStopArea, d, pb);
            result = d.city_first_letter.find(name);
            create_pb(result, nt::eCity, d, pb);
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
        d.load_flz("data.nav.flz");
        d.loaded = true;
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

