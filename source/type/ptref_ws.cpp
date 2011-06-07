#include "baseworker.h"
#include "configuration.h"
#include <iostream>
#include "data.h"
#include "ptreferential.h"
#include <boost/algorithm/string/replace.hpp>
using namespace webservice;


class Worker : public BaseWorker<navitia::type::Data> {

    ResponseData query(RequestData request, Data & d) {
        ResponseData rd;        
        std::string nql_request = request.params["arg"];
        boost::algorithm::replace_all(nql_request, "%20", " ");
        boost::algorithm::replace_all(nql_request, "%3C", "<");
        rd.response << nql_request << "<br/>";
        pbnavitia::PTRefResponse result = ::query(nql_request, d);
        rd.response << pb2txt(result);
        rd.content_type = "text/html";
        rd.status_code = 200;
        return rd;
    }

    
    ResponseData load(RequestData, navitia::type::Data & d) {
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
    Worker(Data &){
        register_api("/query",boost::bind(&Worker::query, this, _1, _2), "Api ptref");
        register_api("/load",boost::bind(&Worker::load, this, _1, _2), "Api de chargement des données");
        add_default_api();
    }
};

MAKE_WEBSERVICE(navitia::type::Data, Worker)
