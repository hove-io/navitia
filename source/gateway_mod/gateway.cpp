#include "gateway.h"
#include "type/type.pb.h"

#include <boost/thread/locks.hpp>
#include <ctime>
#include <iostream>
#include <google/protobuf/descriptor.h>
#include <curlpp/Exception.hpp>
#include <log4cplus/logger.h>
#include "type/pb_utils.h"


Worker::Worker(Pool &){
    register_api("/", boost::bind(&Worker::handle, this, _1, _2), "traite les requétes");
    register_api("firstletter", boost::bind(&Worker::handle, this, _1, _2), "traite les requètes");
    register_api("streetnetwork", boost::bind(&Worker::handle, this, _1, _2), "traite les requètes");
    register_api("planner", boost::bind(&Worker::handle, this, _1, _2), "planne les requêtes");
    register_api("load", boost::bind(&Worker::load, this, _1, _2), "traite les requétes");
    register_api("register", boost::bind(&Worker::register_navitia, this, _1, _2), "ajout d'un NAViTiA au pool");
    register_api("status", boost::bind(&Worker::status, this, _1, _2), "status");
    register_api("unregister", boost::bind(&Worker::unregister_navitia, this, _1, _2), "suppression d'un NAViTiA du pool");
}


webservice::ResponseData Worker::status(webservice::RequestData& request, Pool& pool){
    webservice::ResponseData rd;
    Context context;

    render_status(request, rd, context, pool);

    return rd;
}

webservice::ResponseData Worker::register_navitia(webservice::RequestData& request, Pool& pool){
    webservice::ResponseData rd;
    int thread = 8;

    if(request.params.find("url") == request.params.end()){
        rd.status_code = 500;
        return rd;
    }
    auto it = request.params.find("thread");
    if(it != request.params.end()){
        //@TODO: std::lexical_cast
        thread = atoi(it->second.c_str());
    }

    //TODO valider l'url
    pool.add_navitia(std::make_shared<Navitia>(request.params["url"], thread));

    return status(request, pool);
}

webservice::ResponseData Worker::unregister_navitia(webservice::RequestData& request, Pool& pool){
    webservice::ResponseData rd;

    if(request.params.find("url") == request.params.end()){
        rd.status_code = 500;
        return rd;
    }

    //TODO valider l'url
    pool.remove_navitia(Navitia(request.params["url"], 8));

    return status(request, pool);
}

webservice::ResponseData Worker::handle(webservice::RequestData& request, Pool& pool){
    pool.check_desactivated_navitia();
    webservice::ResponseData rd;
    Context context;
    dispatcher(request, rd, pool, context);
    render(request, rd, context);
    return rd;
}

webservice::ResponseData Worker::load(webservice::RequestData& request, Pool& pool){
    //TODO gestion de la desactivation
    BOOST_FOREACH(auto nav, pool.navitia_list){
        try{
            nav->load();
        }catch(RequestException& ex){
            if(ex.timeout){
                //le rechargement est trop long, rien de dramatique, on enchaine
                // ==> euh si, ca serait balot d'avoir tous les moteurs qui load en meme temps
                continue;
            }
            log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
            LOG4CPLUS_WARN(logger, "le rechargement de " + nav->url + " à echouer");
        }
    }
    return this->status(request, pool);
}

void Dispatcher::operator()(webservice::RequestData& request, webservice::ResponseData& response, Pool& pool, Context& context){
    std::pair<int, std::string> res;
    int nb_try = 0;
    bool ok = true;
    do{
        ok = true;
        nb_try++;
        auto nav = pool.next();
        try{
            res = nav->query(request.path.substr(request.path.find_last_of('/')) + "?" + request.raw_params);
            pool.release_navitia(nav);
        }catch(RequestException& ex){
            ok = false;
            nav->on_error();
            log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
            LOG4CPLUS_WARN(logger, "la requétes a échouer");
            pool.release_navitia(nav);
            response.status_code = ex.code;
            continue;
        }
        std::unique_ptr<google::protobuf::Message> resp = create_pb();
        if(resp->ParseFromString(res.second)){
            /*if(resp->has_error()){
                ok = false;
                nav->on_error();
                context.str = resp->error();
                context.service = Context::BAD_RESPONSE;
                continue;
            }*/
            context.pb = std::move(resp);
            context.service = Context::PTREF;
        }else{
            ok = false;
            nav->on_error();
            log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
            LOG4CPLUS_WARN(logger, "erreur de chargement du protobuf");
            context.str = res.second;
            context.service = Context::BAD_RESPONSE;
            response.status_code = res.first;
        }
    }while(!ok && nb_try < 4);
}

MAKE_WEBSERVICE(Pool, Worker)
