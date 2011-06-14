#include "baseworker.h"
#include "configuration.h"
#include <iostream>
#include "data.h"
#include "ptreferential.h"
#include <boost/algorithm/string/replace.hpp>
using namespace webservice;


class Worker : public BaseWorker<navitia::type::Data> {

    void decode(std::string& str){
        size_t pos = -1;
        int code = 0;
        std::stringstream ss;
        std::string number;
        while((pos = str.find("%", pos+1)) != std::string::npos){
            number = str.substr(pos+1, 2);
            if(number.size() < 2){
                continue;
            }
            ss << std::hex << number;
            ss >> code;
            ss.clear();
            if(code < 32){
                continue;
            }
            str.replace(pos, 3, 1, (char)code);
        }

    }

    ResponseData query(RequestData request, Data & d) {
        ResponseData rd;        
        std::string nql_request = request.params["arg"];
        decode(nql_request);
        pbnavitia::PTRefResponse result;
        try{
            result = navitia::ptref::query(nql_request, d);
            result.SerializeToOstream(&(rd.response));
            rd.content_type = "text/protobuf";
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
    Worker(Data &){
        register_api("/query",boost::bind(&Worker::query, this, _1, _2), "Api ptref");
        register_api("/load",boost::bind(&Worker::load, this, _1, _2), "Api de chargement des données");
        add_default_api();
    }
};

MAKE_WEBSERVICE(navitia::type::Data, Worker)
