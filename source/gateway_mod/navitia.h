#pragma once
#include <boost/thread/shared_mutex.hpp>

class Navitia {
    public:
        std::string url;
        int unused_thread;
        int last_request_at;
        boost::shared_mutex mutex;
        int current_thread;
        int nb_errors;
        int last_errors_at;
        bool enable;
        int reactivate_at;
        int next_decrement;


        static const int desactivation_time = 5;


        Navitia(const std::string& url) : url(url), unused_thread(1), last_request_at(0), current_thread(0), nb_errors(0), last_errors_at(0), enable(true), reactivate_at(0), next_decrement(0){}
        Navitia(const std::string& url, int thread) : url(url), unused_thread(thread), last_request_at(0), current_thread(0), nb_errors(0), last_errors_at(0), enable(true), reactivate_at(0), next_decrement(0){}

        Navitia(const Navitia& nav) : url(nav.url), unused_thread(nav.unused_thread), last_request_at(0), current_thread(0), nb_errors(0), last_errors_at(0), enable(true), reactivate_at(0), next_decrement(0){}

        std::pair<int, std::string> query(const std::string& request);

        bool operator==(const Navitia& other){
            return this->url == other.url;
        }

        void load();
        
        void use();
        void release();
        void on_error();

        void reactivate();
        void decrement_error();
};
