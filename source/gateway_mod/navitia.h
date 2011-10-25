#pragma once
#include <boost/thread/shared_mutex.hpp>

/**
 * représente une instance NAViTiA
 */

struct RequestException : public std::exception{
    std::string body;
    int code;

    bool timeout;

    RequestException(): code(0), timeout(false){};
    RequestException(const std::string& body, int code): body(body),  code(code), timeout(false){};
    RequestException(bool timeout): code(0), timeout(timeout){};
    
    //destructeur pour faire plaisir a gcc
    ~RequestException() throw(){}
};


class Navitia {
    public:

        /**
         * url compléte vers navitia
         */
        std::string url;
        ///nombre de thread non utilisé, utilisé pour pour la répartition, cette valeur peut etre négative
        int unused_thread;
        ///timestamp de la derniére requétes
        int last_request_at;

        boost::shared_mutex mutex;
        ///nombre de requétes en cours sur ce navitia
        int current_thread;
        ///nombre d'erreurs
        int nb_errors;
        //timestamps de la derniére erreurs
        int last_errors_at;
        ///état
        bool enable;
        ///timestamp a partir duquel ce navitia pourras etre réactivé
        int reactivate_at;
        ///ttimestamp ou le nombre d'erreurs pourras etre décrémenté de 1
        int next_decrement;

        ///temps de désactivation par erreurs
        static const int desactivation_time = 5;


        Navitia(const std::string& url) : url(url), unused_thread(1), last_request_at(0), current_thread(0), nb_errors(0), last_errors_at(0), enable(true), reactivate_at(0), next_decrement(0){}
        Navitia(const std::string& url, int thread) : url(url), unused_thread(thread), last_request_at(0), current_thread(0), nb_errors(0), last_errors_at(0), enable(true), reactivate_at(0), next_decrement(0){}

        Navitia(const Navitia& nav) : url(nav.url), unused_thread(nav.unused_thread), last_request_at(0), current_thread(0), nb_errors(0), last_errors_at(0), enable(true), reactivate_at(0), next_decrement(0){}

        std::pair<int, std::string> query(const std::string& request);
        
        /// la comparaison s'effectue uniquement sur l'url
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
