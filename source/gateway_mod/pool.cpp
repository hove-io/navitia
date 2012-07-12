#include "pool.h"
#include <iostream>
#include "utils/configuration.h"
#include <log4cplus/logger.h>
#include <curl/curl.h>
#include <boost/foreach.hpp>
#include <utils/logger.h>

Pool::Pool(){
    init_logger();
	Configuration * conf = Configuration::get();
    std::string initFileName = conf->get_string("path") + conf->get_string("application") + ".ini";
    init_logger(initFileName);
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    LOG4CPLUS_DEBUG(logger, "chargement de la configuration");

    conf->load_ini(initFileName);

	this->nb_threads = conf->get_as<int>("GENERAL","NbThread", 4);
    //@TODO géré l'enregistrement des NAVitIA depuis le fichiers de conf => définir un format potable

    navitia_list.push_back(new Navitia("http://localhost/n1/", 8));
    std::make_heap(navitia_list.begin(), navitia_list.end(), Sorter());


    //Initialisation de curl, cette methode n'est pas THREADSAFE
    if(curl_global_init(CURL_GLOBAL_NOTHING) != 0){
        LOG4CPLUS_ERROR(logger, "erreurs lors de l'initialisation de CURL");
    }
}


void Pool::add_navitia(Navitia* navitia){
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    LOG4CPLUS_DEBUG(logger, "ajout du navitia " + navitia->url);
    mutex.lock();
    navitia_list.push_back(navitia);
    std::push_heap(navitia_list.begin(), navitia_list.end(), Sorter());
    mutex.unlock();
}

void Pool::remove_navitia(const Navitia& navitia){
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    LOG4CPLUS_DEBUG(logger, "suppression du navitia" + navitia.url);
    mutex.lock();
    std::deque<Navitia*>::iterator it = std::find_if(navitia_list.begin(), navitia_list.end(), Comparer(navitia));
    if(it == navitia_list.end()){
        mutex.unlock();
        LOG4CPLUS_DEBUG(logger, "navitia : " + navitia.url + " introuvable");
        return;
    }
    const Navitia* nav = *it;
    navitia_list.erase(it);
    std::make_heap(navitia_list.begin(), navitia_list.end(), Sorter());
    mutex.unlock();
    while(nav->current_thread > 0){
        LOG4CPLUS_DEBUG(logger, "attente avant suppression de: " + navitia.url);
        usleep(2);
    }
    delete nav;
    //utilisé un thread pour détruire le navitia quand celui ci ne serat plus utilisé?
}

void Pool::check_desactivated_navitia(){
    //@TODO manque des mutex sur le nav
    int now = time(NULL);
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));

    BOOST_FOREACH(Navitia* nav, navitia_list){
        if(!nav->enable && nav->reactivate_at < now){
            nav->reactivate();
            LOG4CPLUS_DEBUG(logger, "réactivation de " + nav->url);
        }

        if(nav->nb_errors > 0 && nav->enable && nav->next_decrement < now){
            nav->decrement_error();
            LOG4CPLUS_DEBUG(logger, "reset des erreurs de " + nav->url);
        }
    }
}
