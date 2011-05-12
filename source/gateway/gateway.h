#pragma once

#include <string>
#include <vector>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/shared_mutex.hpp>
#include "baseworker.h"

#include "stat.h"

namespace bt = boost::posix_time;

/** Contient toutes les informations relatives aux instances NAViTiA
 * pilotÃ©es par la passerelle
 */
struct Navitia{
    /// Constructeur par copie
    Navitia(const Navitia & n);

    /// Constructeur par dÃ©faut d'un Navitia
    Navitia();


    /// opÃ©rateur d'affection
    Navitia & operator=(const Navitia & other);

    /// Mutex pour proteger l'itÃ©rateur indiquant le prochain NAViTiA Ã  utiliser
    boost::shared_mutex mutex;

    /// Serveur utilisÃ©
    std::string server;

    /// Chemin vers dll/fcgi que l'on dÃ©sire interroger
    std::string path;

    /// Nombre d'erreurs de type simple rencontrÃ©es
    int error_count;

    /// La date Ã  partir de laquelle ce NAViTiA sera actif
    bt::ptime next_time_status_ok;

    /// Nombre d'erreur de type grave rencontrÃ©es par NAViTiA
    int global_error_count;

    /// Nombre d'erreur de type grave incrÃ©mentÃ© Ã  chaque dÃ©sactivation NAViTiA
    int maxError_count;

    /// Nombre d'appel NAViTiA
    int call_count;

    /// Etat de chargement de NAViTiA
    bool is_loading;

    /// Etat de NAViTiA
    bool is_navitia_ready;

    /// La date de l'activation de la Thread NAViTiA
    bt::ptime thread_date;

    /// Constructeur : on passe le serveur, et le chemin vers dll/fcgi que l'on dÃ©sire utiliser
    Navitia(const std::string & server, const std::string & path);

    /// On interroge dll/fcgi avec la requÃªte passÃ©e en paramÃštre
    std::string query(const std::string & request);

    std::string get_status();
    std::string get_load();
    bool is_navitia_loaded(const std::string & response);
    bool is_server_error(const std::string & response);
    bool is_navitia_error(const std::string & response);
    bool exists_in_response(const std::string &response, const std::string &word);
    void activate();
    void desactivate(const int timeValue, const bool pb_global = false);
    void activate_thread();
    void add_error_count();
	bool is_navitia_on_load(const std::string & response);	
};

/** Contient un ensemble de NAViTiA qui sont interrogeables */
struct NavitiaPool {
    // clock pour la gestion des stats
    ClockThread clockStat;

	// gestion des utilisateurs
	Manage_user manageUser;

	// gestion du coût de l'API 
	Manage_cost manageCost;

    /// Iterateur vers le prochain NAViTiA Ã  interroger
    std::vector<Navitia>::iterator next_navitia;

    /// Structure contenant l'ensemble des navitias
    std::vector<Navitia> navitias;

    /// Mutex pour proteger l'itÃ©rateur indiquant le prochain NAViTiA Ã  utiliser
    boost::mutex iter_mutex;

    // L'index de navitia en chargement:
    int navitia_onload_index;
    
    /// Nombre de threads
    int nb_threads;

    ///ErrorLevel
    int error_level;

    /// ExecptionLimit pour chaque navitia
    int exception_limit;

    /// ReactivationDelay
    int reactivation_delay;

    /// GlobalReactivationDelay
    int global_reactivation_delay;
    int global_error_limit;
    int reinitialise_exception;

    /// UseDatabaseStat
    bool use_database_stat;

    ///UseDatabaseuser;
    bool use_database_user;

    ///MaxCallTry;
    int max_call_try;

    ///Timer
    int timer_value;

    ///Section [LOG] Les paramÃštre d'activation de la recherche itinÃ©raire.
    std::string log_fileName;
    bool plan_journey_enabled;
    bool response_plan_journey_enabled;
    bool detail_plan_journey_enabled;

    ///Section [SQLLOG] paramÃštres de la base de stat
    int web_service_id;
    std::string db_host_name;
    std::string db_name;
    std::string db_user_name;
    std::string db_password;
    std::string db_datetime_format;

    ///Section [LOAD]
    std::string load_user;
    std::string load_password;

    ///Section [STATUS]
    std::string status_user;
    std::string status_password;

    /// Constructeur par dÃ©faut
    NavitiaPool();
    /// Rajoute un nouveau navitia au Pool. Celui-ci sera copiÃ©
    void add(const std::string & server, const std::string & path);

    /// Choisit un NAViTiA et lui fait executer la requÃªte
    void query(const std::string & query, ResponseData& response);

    /// Choisi le prochain NAViTiA libre et l'envoyer
    Navitia & get_next_navitia();

    void desactivate_navitia_on_load(Navitia & nav);

    void activate_all_navitia();

    int deactivated_navitia_count();
    int navitia_on_error_count();
    //Vérifie s'il y a un seul navitia activé dans le NavitiaPool
    bool one_navitia_activated();

    int active_navitia_percent();

    void verify_and_desactivate_navitia(Navitia & nav);
    void desactivate_navitia(Navitia & nav);
    std::string get_query_response();
    void add_navitia_error_count(Navitia & nav);
};


