#pragma once
#include <boost/thread/shared_mutex.hpp>

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

        bool operator==(const Navitia& other){
            return this->url == other.url;
        }

        void load();
        
        void use();
        void release();

};
