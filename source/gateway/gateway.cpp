#include "gateway.h"
#include "http.h"
#include "boost/lexical_cast.hpp"
#include "baseworker.h"
#include <rapidxml.hpp> 
#include <boost/foreach.hpp>
#include "stat.h"

using namespace webservice;
Navitia::Navitia(const std::string & server, const std::string & path) : 
				server(server), path(path), error_count(0), next_time_status_ok(bt::second_clock::local_time()),
					global_error_count(0),maxError_count(0), call_count(0),is_loading(false), is_navitia_ready(true), 
					navitia_thread_date(bt::second_clock::local_time()) 
{
}


Navitia::Navitia() : 
			        error_count(0), next_time_status_ok(bt::second_clock::local_time()),
					global_error_count(0),maxError_count(0), call_count(0),is_loading(false), is_navitia_ready(true), 
					navitia_thread_date(bt::second_clock::local_time()) 
{
}

Navitia::Navitia(const Navitia & n) : 
				server(n.server), path(n.path), error_count(0), next_time_status_ok(bt::second_clock::local_time()),
					global_error_count(0),maxError_count(0), call_count(0),is_loading(false), is_navitia_ready(true), 
					navitia_thread_date(bt::second_clock::local_time()) 
{
}

void Navitia::operator=(const Navitia & other){
    this->server = other.server;
    this->path = other.path;
}
std::string Navitia::query(const std::string & request){

	std::string resp;
	try
	{
		resp = get_http(server, path + request);
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

/// Classe associée �  chaque thread
struct Worker : public BaseWorker<NavitiaPool> {
    
    //API status
	ResponseData status(RequestData, NavitiaPool & np) {
		ResponseData resp;
		std::string strResponse = ""; 
        resp.status_code = 200;
        resp.content_type = "text/xml";
        resp.response = "";
		BOOST_FOREACH(Navitia & n, np.navitias) 
		{
			strResponse += "<WSN WSNId=\""+ boost::lexical_cast<std::string>(np.web_service_id)+ "\" WSNURL=\"http://"+n.server+"/"+n.path+"\">\n";
			strResponse += "<NavitiaStatus ErrorCount=\"" + boost::lexical_cast<std::string>(n.error_count) + "\">\n";
			strResponse += n.get_status();
			strResponse += "</NavitiaStatus>\n";
			strResponse += "</WSN>\n"; 	
		}
		resp.response = "<GatewayStatus>\n";
		resp.response +="<Version>0</Version>\n";
		resp.response +="<CheckAccess>"+boost::lexical_cast<std::string>(np.use_database_user)+"</CheckAccess>\n";
		resp.response +="<SafeMode>0</SafeMode>\n";
		resp.response +="<SaveStat>"+boost::lexical_cast<std::string>(np.use_database_stat)+"</SaveStat>\n";
		resp.response +="<SQLConnection>0</SQLConnection>\n";
		resp.response +="<StatBlackListedFile>0</StatBlackListedFile>\n";
		//Nom du fichier de stat
		resp.response +="<StatFileWorking StatLineWorking=\"0\"></StatFileWorking>\n";
		resp.response +="<StatFileSQLWriting StatLineSQLWriting=\"0\"></StatFileSQLWriting>";
		resp.response +="<LoadStatus NavitiaCount=\"" + boost::lexical_cast<std::string>(np.navitias.size()) + "\" NavitiaToLoad=\"0\"></LoadStatus>\n";
		resp.response +="<GatewayThread GatewayThreadMax=\""+ boost::lexical_cast<std::string>(np.nb_threads) + "\">";
		resp.response += boost::lexical_cast<std::string>(np.nb_threads) +"</GatewayThread>\n";
		resp.response += "<NavitiaList NAViTiAOnError=\"" + boost::lexical_cast<std::string>(np.navitia_on_error_count())+ "\"";
		resp.response +=" NAViTiAEventSynchro=\"0\" DeactivatedNAViTiA=\""+boost::lexical_cast<std::string>(np.deactivated_navitia_count())+"\">\n"; 
		resp.response +=strResponse;
		resp.response += "</NavitiaList>\n";
		resp.response += "</GatewayStatus>\n";		

		return resp;
    }

	//API load
	ResponseData load(RequestData, NavitiaPool & np) 
	{
		ResponseData resp;
		resp.response = "<Status>Loading</Status>"; 		
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
			//si le chargement passe mal : si le GATEWAY ne réussi pas charger la base, il désactive la DLL

			for(int index=0; index < np.max_call_try; index++)
			{
				// Appeler le DLL avec /load
				response_load = nav.get_load();

				//Si serveur erreur alors désactiver ce navitia
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

			//Si erreur de chargement après MaxCallTry iteration alors désactiver:
			if (navitia_load_error)
			{
				//Désactiver ce navitia avec une vérification de /status
				np.desactivate_navitia_on_load(nav);
			}
			else
			{
				//Ecrire le log
			}
			nav.is_loading = false;
		}
		return resp;
	}	

    ResponseData relay(RequestData req, NavitiaPool & pool) {
        ResponseData resp;
        resp.status_code = 200;
        resp.content_type = "text/xml";
        //try{
                resp.response = pool.query(req.path + "?" + req.raw_params );
          /* }
            catch(http_error e){
                resp.response = "<error>"
                    "<http code=\"";
                resp.response += boost::lexical_cast<std::string>(e.code)+ "\">";
                    resp.response += e.message;
                    resp.response += "</http>"
                    "</error>";
            }
			*/
            return resp;
    }

    Worker(NavitiaPool &) {
        register_api("/api", boost::bind(&Worker::relay, this, _1, _2), "Relaye la requête vers un NAViTiA du pool");
        add_param("/api", "action", "Requête �  demander �  NAViTiA", "String", true);
        register_api("/status", boost::bind(&Worker::status, this, _1, _2), "Donne des informations sur la passerelle");
        register_api("/load", boost::bind(&Worker::load, this, _1, _2), "Chargement de tous les NAViTiA");
        add_default_api();
    }

};

NavitiaPool::NavitiaPool() : nb_threads(16){

	std::string Server = "";
	std::string Path = "";
	std::string sectionNameToFind = "";
	std::string sectionNameINI = "";
	size_t sectionFound;
	boost::property_tree::ptree pt;
	
	boost::property_tree::read_ini("L:\\NAViTiACpp\\trunk\\build\\gateway\\Debug\\qgateway.ini", pt);
	//boost::property_tree::read_ini("qgateway.ini", pt);
	boost::property_tree::ptree::const_iterator it_end = pt.end();
	
	/*
	for(boost::property_tree::ptree::iterator it = pt.begin(); it != pt.end(); ++it){

		//sectionNameINI = it.first;
		std::cout<<it->first<<std::endl;
		BOOST_FOREACH(auto a, it->first){
			
		}
	}*/

	BOOST_FOREACH(auto it, pt)
	{
		sectionNameINI = it.first;
		
		//Lire les paramètres de la section [GENERAL]
		sectionNameToFind="GENERAL";
		if (sectionNameINI == sectionNameToFind)
		{
			this->nb_threads = pt.get<int>(sectionNameToFind + ".NbThread");
			this->error_level = pt.get<int>(sectionNameToFind + ".ErrorLevel");
			this->exception_limit = pt.get<int>(sectionNameToFind + ".ExceptionLimit");
			this->reactivation_delay = pt.get<int>(sectionNameToFind + ".ReactivationDelay");
			this->global_reactivation_delay = pt.get<int>(sectionNameToFind + ".GlobalReactivationDelay");
			this->global_error_limit = pt.get<int>(sectionNameToFind + ".GlobalErrorLimit");
			this->reinitialise_exception = pt.get<int>(sectionNameToFind + ".ReinitialiseException");
			this->use_database_stat = pt.get<int>(sectionNameToFind + ".UseDataBaseStat");
			this->use_database_user = pt.get<int>(sectionNameToFind + ".UseDataBaseUser");
			this->max_call_try = pt.get<int>(sectionNameToFind + ".MaxCallTry");
			this->timer_value = pt.get<int>(sectionNameToFind + ".TIMER");
		}

		//Lire les paramères de la section [LOG]
		sectionNameToFind="LOG";
		if (sectionNameINI == sectionNameToFind)
		{
			this->log_fileName = pt.get<std::string>(sectionNameToFind + ".LogFile");
			this->plan_journey_enabled = pt.get<bool>(sectionNameToFind + ".PlanJourneyEnabled");
			this->response_plan_journey_enabled = pt.get<bool>(sectionNameToFind + ".ResponsePlanJourneyEnabled");
			this->detail_plan_journey_enabled = pt.get<bool>(sectionNameToFind + ".DetailPlanJourneyEnabled");
		}

		//Lire les paramères de la section [SQLLOG]
		sectionNameToFind="SQLLOG";
		if (sectionNameINI == sectionNameToFind)
		{
			this->web_service_id = pt.get<int>(sectionNameToFind + ".WebServiceID");
			this->db_host_name = pt.get<std::string>(sectionNameToFind + ".IP");
			this->db_name = pt.get<std::string>(sectionNameToFind + ".DatabaseName");
			this->db_user_name = pt.get<std::string>(sectionNameToFind + ".Login");
			this->db_password = pt.get<std::string>(sectionNameToFind + ".Password");
		}

		//Lire les paramètres de la section [LOAD] ???
		sectionNameToFind="LOAD";

		//Lire les paramètres des sections qui commencent par [NAVITIA_
		sectionFound = sectionNameINI.find("NAVITIA_");
		if (sectionFound!=std::string::npos)
		{
			Server= pt.get<std::string>(sectionNameINI + ".server");
			Path= pt.get<std::string>(sectionNameINI + ".path");
			add(Server,Path);
		}
	}	
}

Navitia & NavitiaPool::get_next_navitia(){
	iter_mutex.lock();
	bool navitia_found = false;
	std::vector<Navitia>::iterator oldest_navitia_index = this->next_navitia;
	
	//Initialiser la date de navitia_last_used_date �  now + 10 seconds
	bt::ptime  navitia_last_used_date = bt::second_clock::local_time() + bt::seconds(10);
	
	for(unsigned int index=0;index < this->navitias.size() * this->max_call_try; index++){
		next_navitia++;
		if (next_navitia == navitias.end())
			next_navitia = navitias.begin();
		
		// Si ErrorCount est supérieur �  exception_limit alors desactiver cette dll et aller au prochain navitia
		//Attention : La désactivation se fait dans une criticalsection
		if (next_navitia->error_count > this->exception_limit){
			this->verify_and_desactivate_navitia(*next_navitia);
			continue;
		}

		//Récupérer la date de dérnière utilisation de navitia la plus ancienne.
		if (next_navitia->navitia_thread_date < navitia_last_used_date){
			navitia_last_used_date = next_navitia->navitia_thread_date;
			oldest_navitia_index = next_navitia;
		}
		
		//Vérifier si le NAViTiA est libre pour utiliser:
		if ((!next_navitia->is_loading) && 
			(next_navitia->is_navitia_ready) && 
			(next_navitia->next_time_status_ok < bt::second_clock::local_time())){
			navitia_found = true;
			next_navitia->call_count++;

			//Si callcount est supérieur �  une valeur max alors reinitialiser erreurCount �  0:
			if ((next_navitia->error_count > 0) && (next_navitia->call_count > this->reinitialise_exception)){
				next_navitia->error_count = 0;
				next_navitia->call_count = 0;
			}
		}
		
		//Si navitia trouvé alors sortir de la boucle for
		if (navitia_found){
			break;
		}
	}

	//A la sortie de la boucle for si aucun navitia est utilisable alors 
	//envoyer le navitia le plus ancien et activer tous les navitia:
	if (!navitia_found){
		//Utiliser le navitia le plus ancien (le navitia le plus ancien n'est jamais désactivé)
		next_navitia = oldest_navitia_index;
		//Réactiver tous les navitia qui ont étés désactivés avec une valeur normale.
		this->activate_all_navitia();
	}

	next_navitia->is_navitia_ready = false;
	next_navitia->navitia_thread_date = bt::second_clock::local_time();
	iter_mutex.unlock();

	return *next_navitia;
}

std::string NavitiaPool::query(const std::string & query){
	std::string response;
	bool is_response_ok = false;
	/*
	iter_mutex.lock();
    next_navitia++;
    if(next_navitia == navitias.end())
        next_navitia = navitias.begin();
    if(next_navitia == navitias.end())
        return "Aucune instance de NAViTiA présente";
    iter_mutex.unlock();
	return next_navitia->query(query);
	*/
	
	//Récupérer le prochain navitia libre �  utiliser(gestion de loadbalancing):
	for(int call_index = 0; call_index <= this->max_call_try || !is_response_ok; call_index++){
		Navitia & na = this->get_next_navitia();
		response = na.query(query);
	
		//Liberer ce navitia pour pouvoir utiliser par les autres appels:
		na.activate_thread();
		
		//En cas d'erreur ServerError désactiver ce navitita et faire appel au navitia suivant:
		if (na.is_server_error(response)){
			this->desactivate_navitia(na);
		}
		//En cas d'erreur NavitiaError incrémenter le ErrorCount
		else if (na.is_navitia_error(response)){
			na.add_error_count();
		}
		else{
			is_response_ok = true;
		}
		//il faut re-récupérer le prochain navitia et passer la requête.
	}

	//S'il y a une bonne réponse alors traiter le HIT
	//1. il faut traiter les information dans le noeud <HIT>......</HIT> pour enregistrer les statistiques
	//2. il faut supprimer ce noeud dans la réponse et renvoyer le reste de la réponse.
	if (is_response_ok){

	}

	return response;
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
	this->navitia_mutex.lock();
	this->error_count++;
	this->navitia_mutex.unlock();
}

void NavitiaPool::desactivate_navitia_on_load(Navitia & nav)
{
	// Déclarer les variables
	std::string response;
	//Appeler ce navitia avec /status?&SafeMode=0
	response = nav.query("/status?&SafeMode=0");
	if (!nav.is_navitia_loaded(response)){
		this->verify_and_desactivate_navitia(nav);
	}
}
void NavitiaPool::verify_and_desactivate_navitia(Navitia & nav){
	//Désactiver ce navitia:
	//Si le nombre de NAViTiA activé = 1 alors on désactive jamais; 
	if (one_navitia_activated()){ 
		return;
	}
			
	//Si pourcentage de NAViTiA disponible < 50, alor on réactive tous les NAViTia
	//sauf celui avec GlobalReactivationDelay
	if (this->active_navitia_percent() < 50){
		this->activate_all_navitia();
	}
			
	//Si le global error count > global error limit alors désactiver ce navitia avec 
	//une valeur de global reactivation delay si non avec reactivation delay:
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
		if (n.next_time_status_ok > bt::second_clock::local_time())
			desactive_navitia_count++;
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
	bt::ptime next_time_ok = bt::second_clock::local_time() + bt::seconds(this->reactivation_delay);
	BOOST_FOREACH(Navitia & nav, this->navitias){
		if (nav.next_time_status_ok < next_time_ok){
			nav.activate();
		}
	}
		
}
void Navitia::desactivate(const int timeValue, const bool pb_global){
	this->next_time_status_ok = bt::second_clock::local_time() + bt::seconds(timeValue);
	this->error_count = 0;
	this->call_count = 0;
	this->is_navitia_ready = true;
	this->navitia_thread_date = bt::second_clock::local_time();
	this->maxError_count++;
	if (pb_global){
		this->global_error_count = 0;
	}
	else {
		this->global_error_count++;
	}
	this->maxError_count++;
}

void Navitia::activate(){
	this->next_time_status_ok = bt::second_clock::local_time();
	this->error_count = 0;
	this->call_count = 0;
}

bool Navitia::is_navitia_loaded(const std::string & response)
{
	std::string Complet = ">Complete<";
	std::string LoadError = "<LoadError>";
	std::string ServerError = "ServerError";	
	return ((!existe_in_response(response, ServerError)) && existe_in_response(response, Complet) && (!existe_in_response(response, LoadError)));
}

bool Navitia::existe_in_response(const std::string &response, const std::string &word){
	size_t word_found;
	word_found = response.find(word);
	return (word_found != std::string::npos); 
}

bool Navitia::is_server_error(const std::string & response){
	return existe_in_response(response, "ServerError");
}

bool Navitia::is_navitia_error(const std::string & response){
	std::string navitiaError = "<error>";
	std::string loadError = "<LoadError>";
	return (existe_in_response(response, navitiaError) || existe_in_response(response, loadError));
}

void Navitia::activate_thread(){
	this->navitia_mutex.lock();
	this->is_navitia_ready = true;
	this->navitia_thread_date = bt::second_clock::local_time();
	this->navitia_mutex.unlock();
}


/*void NavitiaPool::const_stats(std::string &data)
{
    rapidxml::xml_document<> doc;    // character type defaults to char
    char * data_ptr = doc.allocate_string(data.c_str());
    doc.parse<0>(data_ptr);    // 0 means default parse flags

    rapidxml::xml_node<> *node = doc.first_node("const");
    if(node)
    {
        node = node->first_node("Thread");
        if(node)
        {
            node = node->first_node("ActiveThread");
            if (node)
            {
                const_calls++;
                nb_threads_sum += atoi(node->value());
                if(const_calls % 100 == 0){
                    std::cout << "Nombre d'appels : " << const_calls << " moyenne de threads actifs : " << (nb_threads_sum/const_calls) << std::endl;
                }
            }
        }
    }
}*/


MAKE_WEBSERVICE(NavitiaPool, Worker)
