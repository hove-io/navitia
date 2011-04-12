#include "gateway.h"
#include "http.h"
#include "baseworker.h"
#include <boost/foreach.hpp>
#include "configuration.h"
#include <rapidxml.hpp>
#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>

using namespace webservice;
Navitia::Navitia(const std::string & server, const std::string & path) : 
				server(server), path(path), error_count(0), next_time_status_ok(bt::second_clock::local_time()),
					global_error_count(0),maxError_count(0), call_count(0),is_loading(false), is_navitia_ready(true), 
                    thread_date(bt::second_clock::local_time()){
}


Navitia::Navitia() : 
			        error_count(0), next_time_status_ok(bt::second_clock::local_time()),
					global_error_count(0),maxError_count(0), call_count(0),is_loading(false), is_navitia_ready(true), 
                    thread_date(bt::second_clock::local_time())
{
}

Navitia::Navitia(const Navitia & n) : 
				server(n.server), path(n.path), error_count(0), next_time_status_ok(bt::second_clock::local_time()),
					global_error_count(0),maxError_count(0), call_count(0),is_loading(false), is_navitia_ready(true), 
                    thread_date(bt::second_clock::local_time())
{
}

Navitia & Navitia::operator=(const Navitia & other){
    this->server = other.server;
    this->path = other.path;
    is_navitia_ready = other.is_navitia_ready;
    is_loading = other.is_loading;
    call_count = other.call_count;
    maxError_count =  other.maxError_count;
    global_error_count = other.global_error_count;
    error_count = other.error_count;
    return *this;
}

std::string Navitia::query(const std::string & request){
    std::string q = path + request;
	std::string resp;
	try
	{
		//il faut ajouter &safemode=0 si 
        resp = get_http(server, q);
	}
	catch (http_error e){
        resp = "<ServerError>\n";
		resp +="<http code=\"";
		resp += boost::lexical_cast<std::string>(e.code)+ "\">";
		resp += e.message;
		resp += "</http>";
        resp += "</ServerError>\n";
	}

	return resp;
}

void NavitiaPool::add(const std::string & server, const std::string & path){
    navitias.push_back(Navitia(server, path));
    next_navitia = navitias.begin();
}

/// Classe associ√©e √  chaque thread
struct Worker : public BaseWorker<NavitiaPool> {

    
    //API status
	//Reste ‡ faire : Version ,SafeMode, SQLConnection, StatBlackListedFile, StatFileWorking, StatFileSQLWriting,
	//NavitiaToLoad, NAViTiAEventSynchro 
	
	ResponseData status(RequestData, NavitiaPool & np) {
        ResponseData resp;
        resp.status_code = 200;
        resp.content_type = "text/xml";
        resp.response << "<GatewayStatus>\n"
                <<"<Version>0</Version>\n"
                <<"<CheckAccess>" << np.use_database_user << "</CheckAccess>\n"
                <<"<SafeMode>0</SafeMode>\n"
                <<"<SaveStat>" << np.use_database_stat << "</SaveStat>\n"
                <<"<SQLConnection>0</SQLConnection>\n"
                <<"<StatBlackListedFile>0</StatBlackListedFile>\n"
                //Nom du fichier de stat
                <<"<StatFileWorking StatLineWorking=\"0\"></StatFileWorking>\n"
                <<"<StatFileSQLWriting StatLineSQLWriting=\"0\"></StatFileSQLWriting>"
                <<"<LoadStatus NavitiaCount=\"" << np.navitias.size() <<  "\" NavitiaToLoad=\"0\"></LoadStatus>\n"
                <<"<GatewayThread GatewayThreadMax=\"" << np.nb_threads << "\">"<< np.nb_threads << "</GatewayThread>\n"
                << "<NavitiaList NAViTiAOnError=\"" << np.navitia_on_error_count() << "\"" <<" NAViTiAEventSynchro=\"0\" DeactivatedNAViTiA=\"" << np.deactivated_navitia_count() << "\">\n";

        BOOST_FOREACH(Navitia & n, np.navitias)
        {
            resp.response << "<WSN WSNId=\"" << np.web_service_id <<  "\" WSNURL=\"http://" << n.server << "/" << n.path+"\">\n"
                    << "<NavitiaStatus ErrorCount=\"" << n.error_count <<  "\">\n"
                    << n.get_status()
                    << "</NavitiaStatus>\n"
                    << "</WSN>\n";
        }

        resp.response<< "</NavitiaList>\n"
                << "</GatewayStatus>\n";

        log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
        LOG4CPLUS_DEBUG(logger, "Appel de status");

		return resp;
    }

	//API load
	ResponseData load(RequestData, NavitiaPool & np) 
	{
		/// Reste ‡ faire 
		// 1. Gestion de droit de load : utilisation de user et password dans l'url
		ResponseData resp;
        resp.response << "<Status>Loading</Status>";
		std::string response_load = "";
		
		//Chargement de tous les NAViTiA:
		bool navitia_load_error;
		np.navitia_onload_index = 0;
		BOOST_FOREACH(Navitia &nav, np.navitias)
		{
			//Changer la valeur IsLoading = true pour bloquer ce navitia aux autres:
			nav.is_loading = true;
			np.navitia_onload_index ++;
			navitia_load_error = true;

			//On appelle cette DLL (de l'index en cour) jusqu'au gi_MaxCallTry fois avec /load
			//si le chargement passe mal : si le GATEWAY ne r√©ussi pas charger la base, il d√©sactive la DLL

			for(int index=0; index < np.max_call_try; index++)
			{
				// Appeler le DLL avec /load
				response_load = nav.get_load();

				//Si serveur erreur alors d√©sactiver ce navitia
				if (nav.is_server_error(response_load)){
					np.verify_and_desactivate_navitia(nav);
					navitia_load_error = false;
					break;
				}
				
				//si pas d'erreur
				//if ((nav.is_navitia_loaded(response_load)) && (! nav.is_load_error(response_load)))
				if (nav.is_navitia_loaded(response_load))
				{
					navitia_load_error = false;
					break;
				}
			}

			//Si erreur de chargement apr√®s MaxCallTry iteration alors d√©sactiver:
			if (navitia_load_error)
			{
				//D√©sactiver ce navitia avec une v√©rification de /status
				np.desactivate_navitia_on_load(nav);
			}
			else
			{
				//Ecrire le log
			}
			nav.is_loading = false;
		}
        log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
        LOG4CPLUS_DEBUG(logger, "Appel de load");

		return resp;
	}	

    ResponseData relay(RequestData req, NavitiaPool & pool) {
        ResponseData resp;
        resp.status_code = 200;
        resp.content_type = "text/xml";
        pool.query(req.path + "?" + req.raw_params, resp);
        return resp;
    }
	//API Const
	ResponseData constant(RequestData req, NavitiaPool & pool) 
	{
		ResponseData response;
        response.status_code = 200;
		response.content_type = "text/xml"; 
        pool.query(req.path + "?" + req.raw_params, response );
		return response;
	}	


    Worker(NavitiaPool &) {
        register_api("/api", boost::bind(&Worker::relay, this, _1, _2), "Relaye la requ√™te vers un NAViTiA du pool");
        add_param("/api", "action", "Requ√™te √  demander √  NAViTiA", "String", true);
        register_api("/status", boost::bind(&Worker::status, this, _1, _2), "Donne des informations sur la passerelle");
        register_api("/load", boost::bind(&Worker::load, this, _1, _2), "Chargement de tous les NAViTiA");
		register_api("/const", boost::bind(&Worker::constant, this, _1, _2), "constante d'un des NAViTiA");
        add_default_api();
    }

};

NavitiaPool::NavitiaPool() : nb_threads(16){	
	//RÈcuperer le chemin de la dll et le dom de l'application:
    std::string sectionName;
	std::string serverName;
	std::string pathValue;
	
	Configuration * conf = Configuration::get();
    std::string initFileName = conf->get_string("path") + conf->get_string("application") + ".ini";
	//conf->set_string("log_file",conf->get_string("path") + conf->get_string("application") + ".log");

	//chargement de la configuration du logger
	log4cplus::PropertyConfigurator::doConfigure(LOG4CPLUS_TEXT(initFileName));
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    LOG4CPLUS_DEBUG(logger, "chargement de la configuration");
    
    conf->load_ini(initFileName);
    conf->set_int("wsn_id", 0);
	
	//Lecture des paramËtre dans la section "GENERAL"
	nb_threads = conf->get_as<int>("GENERAL","NbThread", 4);
	error_level = conf->get_as<int>("GENERAL","ErrorLevel", 0);
	exception_limit = conf->get_as<int>("GENERAL","ExceptionLimit", 10);
	reactivation_delay = conf->get_as<int>("GENERAL","ReactivationDelay", 60);
	global_reactivation_delay = conf->get_as<int>("GENERAL","GlobalReactivationDelay", 3600);
	global_error_limit = conf->get_as<int>("GENERAL","GlobalErrorLimit", 3);
	reinitialise_exception = conf->get_as<int>("GENERAL","ReinitialiseException", 5000);
	use_database_stat = conf->get_as<bool>("GENERAL","UseDataBaseStat", false);
	use_database_user = conf->get_as<bool>("GENERAL","UseDataBaseUser", false);
	max_call_try = conf->get_as<int>("GENERAL","MaxCallTry", 1);
    timer_value = conf->get_as<int>("GENERAL","TIMER", 60); // 2 minutes

	//Lecture des paramËtre dans la section "SQLLOG"
	plan_journey_enabled = conf->get_as<bool>("SQLLOG","PlanJourneyEnabled",false);
	response_plan_journey_enabled = conf->get_as<bool>("SQLLOG","ResponsePlanJourneyEnabled",false);
	detail_plan_journey_enabled = conf->get_as<bool>("SQLLOG","DetailPlanJourneyEnabled",false);
	web_service_id = conf->get_as<int>("SQLLOG","WebServiceID",0);
	db_host_name = conf->get_as<std::string>("SQLLOG","IP","");
	db_name = conf->get_as<std::string>("SQLLOG","DatabaseName","");
	db_user_name = conf->get_as<std::string>("SQLLOG","Login","");
	db_password = conf->get_as<std::string>("SQLLOG","Password","");

	//NAVITIA_
	for (int i = 0;i < 20; i++){
		sectionName = "NAVITIA_" + boost::lexical_cast<std::string>(i);
        if (conf->has_section(sectionName)){
            serverName = conf->get_as<std::string>(sectionName, "server", "");
            pathValue = conf->get_as<std::string>(sectionName, "path", "");
            if ((serverName != "") && (pathValue != "")){
                add(serverName,pathValue);
            }
        }
	}

	// chargement des utilisateurs pour l'authontification de l'utilisateur
	manageUser.fill_user_list(web_service_id);

	// Chargement des API et ces co˚ts.
	manageCost.fill_cost_list();

    // On lance le thread qui gËre les statistiques & base
    clockStat.start();
}

Navitia & NavitiaPool::get_next_navitia(){
    iter_mutex.lock();
	bool navitia_found = false;
	std::vector<Navitia>::iterator oldest_navitia_index = this->next_navitia;
	
	//Initialiser la date de navitia_last_used_date √  now + 10 seconds
	bt::ptime  navitia_last_used_date = bt::second_clock::local_time() + bt::seconds(10);
	
	for(unsigned int index=0;index < this->navitias.size() * this->max_call_try; index++){
		
		next_navitia++;
		if (next_navitia == navitias.end())
			next_navitia = navitias.begin();
		
		// Si ErrorCount est sup√©rieur √  exception_limit alors desactiver cette dll et aller au prochain navitia
		//Attention : La d√©sactivation se fait dans une criticalsection
		if (next_navitia->error_count > this->exception_limit){
			this->verify_and_desactivate_navitia(*next_navitia);
			continue;
		}

		//R√©cup√©rer la date de d√©rni√®re utilisation de navitia la plus ancienne.
        if (next_navitia->thread_date < navitia_last_used_date){
            navitia_last_used_date = next_navitia->thread_date;
			oldest_navitia_index = next_navitia;
		}
		
		//V√©rifier si le NAViTiA est libre pour utiliser:
		if ((!next_navitia->is_loading) && 
			(next_navitia->is_navitia_ready) && 
			(next_navitia->next_time_status_ok < bt::second_clock::local_time())){
			navitia_found = true;
			next_navitia->call_count++;

			//Si callcount est sup√©rieur √  une valeur max alors reinitialiser erreurCount √  0:
			if ((next_navitia->error_count > 0) && (next_navitia->call_count > this->reinitialise_exception)){
				next_navitia->error_count = 0;
				next_navitia->call_count = 0;
			}
		}
		
		//Si navitia trouv√© alors sortir de la boucle for
		if (navitia_found){
			break;
		}
	}

	//A la sortie de la boucle for si aucun navitia est utilisable alors 
	//envoyer le navitia le plus ancien et activer tous les navitia:
	if (!navitia_found){
		//Utiliser le navitia le plus ancien (le navitia le plus ancien n'est jamais d√©sactiv√©)
		next_navitia = oldest_navitia_index;
        log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
        LOG4CPLUS_DEBUG(logger, "Oldest navitia used : http://" + next_navitia->server + next_navitia->path);
		//R√©activer tous les navitia qui ont √©t√©s d√©sactiv√©s avec une valeur normale.
		this->activate_all_navitia();
	}

    Navitia & nav = *next_navitia;
    iter_mutex.unlock();

    nav.mutex.lock();
	nav.is_navitia_ready = false;
    nav.thread_date = bt::second_clock::local_time();
    nav.mutex.unlock();
    
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    LOG4CPLUS_DEBUG(logger, "navitia utilisÈ : http://" + nav.server + nav.path);
	
    return nav;
}

void NavitiaPool::query(const std::string & q, ResponseData& response){
    std::string query = q;
    std::string response_navitia; 
    bool is_response_ok = false;
    bool nonStat = false;
    int user_id_local = 0;

    // test de l'utilisateur
    if (this->use_database_user){
        user_id_local = manageUser.grant_access(q);
    }
    if (user_id_local > -1 ) {

        //Il faut ajouter &safemode=0 si enregistrement du stat est activÈ
        if (this->use_database_stat){
            query+="&safemode=0";
        }
        log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
        LOG4CPLUS_DEBUG(logger, "RequÍte d'appel : " + query);

        //R√©cup√©rer le prochain navitia libre √  utiliser(gestion de loadbalancing):
        for(int call_index = 0; (call_index <= this->max_call_try) && (!(is_response_ok)); call_index++){
            Navitia & na = this->get_next_navitia();
            response_navitia = na.query(query);

            //Liberer ce navitia pour pouvoir utiliser par les autres appels:
            na.activate_thread();

            //En cas d'erreur ServerError d√©sactiver ce navitita et faire appel au navitia suivant:
            if (na.is_server_error(response_navitia)){
                this->desactivate_navitia(na);
            }
            //En cas d'erreur NavitiaError incr√©menter le ErrorCount
            else if (na.is_navitia_error(response_navitia)){
                na.add_error_count();
            }
            else{
                is_response_ok = true;
                break;
            }
            //il faut re-r√©cup√©rer le prochain navitia et passer la requ√™te.
        }
    }

    //S'il y a une bonne r√©ponse alors traiter le HIT
    //1. il faut traiter les information dans le noeud <HIT>......</HIT> pour enregistrer les statistiques
    //2. il faut supprimer ce noeud dans la r√©ponse et renvoyer le reste de la r√©ponse.
    if (is_response_ok){
        //Si l'enregistrement de stat est activÈ alors traiter le flux de rÈponse navitia
        if (this->use_database_stat==true){
            StatNavitia statnav;
            //gestion de nonstat : ‡ ne pas enregistrer pjo/rpjo/dpjo si l'url contient &nonstat=1 ou true
            statnav.nonStat = str_to_bool_def(getStringByRequest(q, "nonstat", "&"), false);

            // Affectation du code de l'utilisateur et wsnid
            statnav.user_id = user_id_local;
            statnav.wsn_id = web_service_id;

            // Lecture des informations sur hit/planjourney/responseplanjourney/detailplanjourney
            // Supprime le noeud HIT de la rÈponse NAViTiA
            statnav.readXML(response_navitia, response);

            // RÈcuperations du co˚t de l'API
            //statnav.hit.api_cost = manageCost.getCostByApi(getStringByRequest(q, "action", "&"));
            statnav.hit.api_cost = manageCost.getCostByApi(q);
            //CrÈation d'un nouveau fichier de stat :
            //Soit ‡ chaque cycle du clock / soit quand le nombre d'appel enregister dans un fichier dÈpasse une valeur configurÈ (1000 par dÈfaut)
            if (clockStat.hit_call_count > 1000){
                clockStat.createNewFileName();
            }
            else
                clockStat.hit_call_count++;

            // PrÈparation des fichiers de stat avec les informations sur hit/planjourney/responseplanjourney/detailplanjourney
            statnav.writeSql();
        }

    }else{
        response.response << "<ServerError>\n";
        response.response <<"<http>";
        response.response <<  "User non valide";
        response.response << "</http>";
        response.response << "</ServerError>\n";
    }
}

std::string Navitia::get_status()
{
	std::string resp;
	std::string respStatus = "";
	std::string temp ="";
	std::string strAttrName, strAttrValue ="";
	std::string strNodeName, strNodeValue ="";
	rapidxml::xml_document<> doc; 
	rapidxml::xml_node<> *FirstNode = NULL;

	resp = this->query("/status" );
	// test s ' il n y a pas d'erreur
	if (this->is_server_error(resp)){
		respStatus = resp;
	}
	else{
		char * data_ptr = doc.allocate_string(resp.c_str());
		
		doc.parse<0>(data_ptr);
		FirstNode = doc.first_node("status");
		if(FirstNode)
		{
			for (rapidxml::xml_node<> *Node = FirstNode->first_node(); Node; Node = Node->next_sibling())
			{
				strNodeName = Node->name();
				if (strcmp(strNodeName.c_str(), "Params") != 0)
				{
					strNodeValue = Node->value();
					respStatus += "<"+strNodeName;
					for (rapidxml::xml_attribute<> *attr = Node->first_attribute(); attr; attr = attr->next_attribute())
					{
						strAttrName = attr->name();
						strAttrValue = attr->value();
						respStatus += " "+strAttrName+"=\""+strAttrValue+"\"";
					}			
                    respStatus += ">" + strNodeValue + "</"+strNodeName+">\n";
				}
			}
		}
	}
	return respStatus;
}

std::string Navitia::get_load()
{
	return this->query("/load?&SafeMode=0");
}

void Navitia::add_error_count(){
    this->error_count++;
}

void NavitiaPool::desactivate_navitia_on_load(Navitia & nav)
{
	// D√©clarer les variables
	std::string response;
	//Appeler ce navitia avec /status?&SafeMode=0 et vÈrifier si ce navitia est chargÈ ou pas.
	//Si le navitia n'est pas chargÈ alors dÈsactiver le avec des vÈrification.
	response = nav.query("/status?&SafeMode=0");
	if (!nav.is_navitia_loaded(response)){
		this->verify_and_desactivate_navitia(nav);
	}
}
void NavitiaPool::verify_and_desactivate_navitia(Navitia & nav){
	//D√©sactiver ce navitia:
	//Si le nombre de NAViTiA activ√© = 1 alors on d√©sactive jamais; 
	if (one_navitia_activated()){
        log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
        LOG4CPLUS_WARN(logger, "Navitia disponible = 1");
		return;
	}
			
	//Si pourcentage de NAViTiA disponible < 50, alor on r√©active tous les NAViTia
	//sauf celui avec GlobalReactivationDelay
	if (this->active_navitia_percent() < 50){
        log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
        LOG4CPLUS_WARN(logger, "Navitia disponible < 50 %");
		this->activate_all_navitia();
	}
			
	//Finalement dÈsactiver ce navitia : si le global error count > global error limit alors 
	//dÈsactiver ce navitia avec une valeur de global reactivation delay si non avec reactivation delay
	this->desactivate_navitia(nav);
}
void NavitiaPool::desactivate_navitia(Navitia & nav){
	
	bool is_global = nav.global_error_count> this->global_error_limit;
	int reactivation_value = nav.global_error_count> this->global_error_limit ? this->global_reactivation_delay : this->reactivation_delay;
	nav.desactivate(reactivation_value, is_global);
}

int NavitiaPool::deactivated_navitia_count(){
	int desactive_navitia_count = 0;
	BOOST_FOREACH(Navitia & n, this->navitias) 
	{
        n.mutex.lock_shared();
		if (n.next_time_status_ok > bt::second_clock::local_time())
			desactive_navitia_count++;
        n.mutex.unlock_shared();
	}
	return desactive_navitia_count;	
}

int NavitiaPool::navitia_on_error_count(){
	int navitia_on_error_count = 0;
	BOOST_FOREACH(Navitia & nav, this->navitias) 
	{
		if (nav.maxError_count > 0)
			navitia_on_error_count++;
	}
	return navitia_on_error_count;
}

bool NavitiaPool::one_navitia_activated(){
	return ((this->navitias.size() == 1) ||  
			((this->navitias.size() - deactivated_navitia_count()) ==1));
}

int NavitiaPool::active_navitia_percent(){
	int percent = 0 ;
	int desactivated_navitia_count = this->deactivated_navitia_count();
	int navitia_count = this->navitias.size();
	if (navitia_count > 0){
		percent = static_cast<int>(((navitia_count - desactivated_navitia_count)*100)/navitia_count);
	}

	return percent;
}

void NavitiaPool::activate_all_navitia(){
	//Activation de tous les navitias sauf celui qui a ÈtÈ dÈsactivÈ avec une valeur globale.
	bt::ptime next_time_ok = bt::second_clock::local_time() + bt::seconds(this->reactivation_delay);
	BOOST_FOREACH(Navitia & nav, this->navitias){
        nav.mutex.lock_shared();
		if (nav.next_time_status_ok < next_time_ok){
            nav.mutex.unlock_shared();
			nav.activate();
		}
        else {
            nav.mutex.unlock_shared();
        }
	}
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    LOG4CPLUS_DEBUG(logger, "activation de tous les navitia");
}
void Navitia::desactivate(const int timeValue, const bool pb_global){
    mutex.lock();
	this->next_time_status_ok = bt::second_clock::local_time() + bt::seconds(timeValue);
    this->thread_date = bt::second_clock::local_time();
    mutex.unlock();
	this->error_count = 0;
	this->call_count = 0;
	this->is_navitia_ready = true;
	this->maxError_count++;
	if (pb_global){
		this->global_error_count = 0;
        log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
        LOG4CPLUS_DEBUG(logger, "navitia deactivÈ avec une valeur global : http://" + this->server + this->path);
	}
	else {
		this->global_error_count++;
        log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
        LOG4CPLUS_DEBUG(logger, "navitia deactivÈ avec une valeur locale : http://" + this->server + this->path);
	}
	this->maxError_count++;
}

void Navitia::activate(){
    mutex.lock();
	this->next_time_status_ok = bt::second_clock::local_time();
	this->error_count = 0;
	this->call_count = 0;
    mutex.unlock();
}

bool Navitia::is_navitia_loaded(const std::string & response)
{
	std::string Complet = ">Complete<";
	std::string LoadError = "<LoadError>";
	std::string ServerError = "ServerError";	
    return ((!exists_in_response(response, ServerError)) && exists_in_response(response, Complet) && (!exists_in_response(response, LoadError)));
}

bool Navitia::exists_in_response(const std::string &response, const std::string &word){
	size_t word_found;
	word_found = response.find(word);
	return (word_found != std::string::npos); 
}

bool Navitia::is_server_error(const std::string & response){
    return exists_in_response(response, "ServerError");
}

bool Navitia::is_navitia_error(const std::string & response){
	std::string navitiaError = "<error>";
	std::string loadError = "<LoadError>";
    return (exists_in_response(response, navitiaError) || exists_in_response(response, loadError) || this->is_navitia_on_load(response));
}

bool Navitia::is_navitia_on_load(const std::string & response){
	// Quand on antÈrroge un Navitia qui est en erreur de chargement avec un paramËte &safemode=0 
	// il envoie un flux de rÈponse avec le neoud <FluxLoad> 
	// qui contient une ligne <Load DLLState="Restoring" DataLoaded="No" BackUpActivated="No">Loading</Load>
	std::string fluxLoad = "<FluxLoad>";
	std::string loadError = ">Complete<";	
	return (exists_in_response(response, fluxLoad) & !exists_in_response(response, loadError));
}

void Navitia::activate_thread(){
    this->mutex.lock();
	this->is_navitia_ready = true;
    this->thread_date = bt::second_clock::local_time();
    this->mutex.unlock();
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
    LOG4CPLUS_DEBUG(logger, "navitia libÈrÈ : http://" + this->server + this->path);
}


MAKE_WEBSERVICE(NavitiaPool, Worker)
