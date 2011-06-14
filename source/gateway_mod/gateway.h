#pragma once
#include "baseworker.h"
//#include "configuration.h"
#include "context.h"


class Navitia {
    public:
        std::string url;

        Navitia(const std::string& url) : url(url){}
        std::pair<int, std::string> query(const std::string& request);

};


class Pool{
    public:
        int nb_threads;
        std::vector<Navitia> navitia_list;
        std::vector<Navitia>::iterator it;

        Pool() : nb_threads(8){
            navitia_list.push_back(Navitia("http://localhost:81"));

            it = navitia_list.begin();
        }

        Navitia& next();

};



class Dispatcher{
    public:
    void operator()(webservice::RequestData& request, webservice::ResponseData& response, Pool& pool, Context& context);

};

class Worker : public webservice::BaseWorker<Pool> {
    webservice::ResponseData handle(webservice::RequestData& request, Pool& pool);
    Dispatcher dispatcher;
    public:    
    Worker(Pool &);
};



