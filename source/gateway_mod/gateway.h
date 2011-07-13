#pragma once
#include "baseworker.h"
//#include "configuration.h"
#include "context.h"
#include <boost/thread/shared_mutex.hpp>
#include <set>


class Navitia {
    public:
        std::string url;
        int unused_thread;
        int last_request_at;
        boost::shared_mutex mutex;

        //tas trié par nombre de requéte en cours de traitement
        Navitia(const std::string& url) : url(url), unused_thread(1), last_request_at(0){}
        Navitia(const std::string& url, int thread) : url(url), unused_thread(thread), last_request_at(0){}

        std::pair<int, std::string> query(const std::string& request);
        
        void use();
        void release();

};


class Pool{
    protected:
        boost::shared_mutex mutex;

        struct Sorter{
            bool operator()(const Navitia* a, const Navitia* b){
                if(a->unused_thread == b->unused_thread){
                    return a->last_request_at > b->last_request_at;
                }else{
                    return a->unused_thread > b->unused_thread;
                }
            }
        };

    public:
        
        int nb_threads;
        std::deque<Navitia*> navitia_list;

        Pool() : nb_threads(8){
            navitia_list.push_back(new Navitia("http://localhost:81/1", 8));
            std::make_heap(navitia_list.begin(), navitia_list.end(), Sorter());
        }

        inline void release_navitia(Navitia* navitia){
            navitia->release();
            mutex.lock();
            std::sort_heap(navitia_list.begin(), navitia_list.end(), Sorter());
            mutex.unlock();
        }

        inline Navitia* next(){
            boost::lock_guard<boost::shared_mutex> lock(mutex);
            std::pop_heap(navitia_list.begin(), navitia_list.end(), Sorter());
            Navitia* nav = navitia_list.back();
            nav->use();
            std::sort_heap(navitia_list.begin(), navitia_list.end(), Sorter());

            return nav;
        }

        void add_navitia(Navitia* navitia);

};



class Dispatcher{
    public:
    void operator()(webservice::RequestData& request, webservice::ResponseData& response, Pool& pool, Context& context);

};

class Worker : public webservice::BaseWorker<Pool> {
    webservice::ResponseData handle(webservice::RequestData& request, Pool& pool);
    webservice::ResponseData register_navitia(webservice::RequestData& request, Pool& pool);
    webservice::ResponseData status(webservice::RequestData& request, Pool& pool);
    Dispatcher dispatcher;

    public:    
    Worker(Pool &);
};



