#include "baseworker.h"
#include "configuration.h"
#include <iostream>
#include "ptreferential.h"

using namespace webservice;

class Worker : public BaseWorker<navitia::type::Data> {

    ResponseData query(RequestData request, navitia::type::Data & d) {
        ResponseData rd;        
        std::string nql_request = request.params["arg"];
        //decode(nql_request);
        pbnavitia::PTReferential result;
        try{
            result = navitia::ptref::query(nql_request, d);
            result.SerializeToOstream(&(rd.response));
            rd.content_type = "application/octet-stream";
            rd.status_code = 200;
        }catch(...){
            rd.status_code = 500;
        }
        return rd;
    }

    
    ResponseData load(RequestData, navitia::type::Data & d) {
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
        register_api("query",boost::bind(&Worker::query, this, _1, _2), "Api ptref");
        add_param("query", "arg", "Requête sur PTReferential à exécuter", ApiParameter::STRING, true);
        register_api("load",boost::bind(&Worker::load, this, _1, _2), "Api de chargement des données");
        add_default_api();
    }
};

MAKE_WEBSERVICE(navitia::type::Data, Worker)
