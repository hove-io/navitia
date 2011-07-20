#pragma once
#include "baseworker.h"
//#include "configuration.h"
#include "context.h"
#include "navitia.h"
#include "pool.h"



class Dispatcher{
    public:
    void operator()(webservice::RequestData& request, webservice::ResponseData& response, Pool& pool, Context& context);

};

class Worker : public webservice::BaseWorker<Pool> {

    webservice::ResponseData handle(webservice::RequestData& request, Pool& pool);
    webservice::ResponseData register_navitia(webservice::RequestData& request, Pool& pool);
    webservice::ResponseData status(webservice::RequestData& request, Pool& pool);
    webservice::ResponseData load(webservice::RequestData& request, Pool& pool);
    
    
    Dispatcher dispatcher;

    public:    
    Worker(Pool &);
};



