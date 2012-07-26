#include "pool.h"
#include <iostream>
#include "utils/configuration.h"
#include <log4cplus/logger.h>
#include <curl/curl.h>
#include <boost/foreach.hpp>
#include <utils/logger.h>
#include <assert.h>

namespace navitia{ namespace gateway{

Pool::Pool(){
    init_logger();
	Configuration * conf = Configuration::get();
    std::string initFileName = conf->get_string("path") + conf->get_string("application") + ".ini";
    init_logger(initFileName);
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    LOG4CPLUS_DEBUG(logger, "chargement de la configuration");

    conf->load_ini(initFileName);

	this->nb_threads = conf->get_as<int>("GENERAL","NbThread", 4);

    int i = 0;
    std::string section_name = std::string("NAVITIA_") + boost::lexical_cast<std::string>(i); 

    while(conf->has_section(section_name)){
        std::string url = conf->get_as<std::string>(section_name, "url", "");
        int nb_thread = conf->get_as<int>(section_name, "thread", 8);
        add_navitia( std::make_shared<Navitia>(url, nb_thread));       
        i++;
        section_name = std::string("NAVITIA_") + boost::lexical_cast<std::string>(i); 
    }

    //Initialisation de curl, cette methode n'est pas THREADSAFE
    if(curl_global_init(CURL_GLOBAL_NOTHING) != 0){
        LOG4CPLUS_ERROR(logger, "erreurs lors de l'initialisation de CURL");
    }
}

Pool::Pool(Pool& other) : nb_threads(other.nb_threads){
    boost::lock_guard<boost::shared_mutex> lock(other.mutex);
    navitia_list = other.navitia_list;
}

void Pool::add_navitia(std::shared_ptr<Navitia> navitia){
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    LOG4CPLUS_DEBUG(logger, "ajout du navitia " + navitia->url);
    boost::lock_guard<boost::shared_mutex> lock(mutex);
    navitia_list.push_back(navitia);
    std::push_heap(navitia_list.begin(), navitia_list.end(), Sorter());
    LOG4CPLUS_DEBUG(logger, navitia);
}

void Pool::remove_navitia(const Navitia& navitia){
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    LOG4CPLUS_DEBUG(logger, "suppression du navitia" + navitia.url);
    boost::lock_guard<boost::shared_mutex> lock(mutex);
    auto it = std::find_if(navitia_list.begin(), navitia_list.end(), Comparer(navitia));
    if(it == navitia_list.end()){
        LOG4CPLUS_DEBUG(logger, "navitia : " + navitia.url + " introuvable");
        return;
    }
    navitia_list.erase(it);
    std::make_heap(navitia_list.begin(), navitia_list.end(), Sorter());
}

void Pool::check_desactivated_navitia(){
    int now = time(NULL);
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));

    //@TODO gros lock pas beau
    boost::lock_guard<boost::shared_mutex> lock(mutex);
    BOOST_FOREACH(auto nav, navitia_list){
        if(!nav->enable && nav->reactivate_at < now){
            nav->reactivate();
            LOG4CPLUS_DEBUG(logger, "réactivation de " + nav->url);
        }

        if(nav->nb_errors > 0 && nav->enable && nav->next_decrement < now){
            nav->decrement_error();
            LOG4CPLUS_INFO(logger, "reset des erreurs de " + nav->url);
        }
    }
    std::make_heap(navitia_list.begin(), navitia_list.end(), Sorter());
}

}}
