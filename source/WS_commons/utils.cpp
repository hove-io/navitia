#include "utils.h"
#include <boost/tokenizer.hpp>


namespace webservice{
    map parse_params(const std::string & query_string) {
        boost::char_separator<char> sep("&;");
        boost::tokenizer<boost::char_separator<char> > tokens(query_string, sep);
        for(auto it = tokens.begin(); it != tokens.end(); ++it) {

        }
        return m;
    }
}

/*
SettingsMap parse_ini_file(std::string filename){
	
	std::string section = "";
	std::string param = "";
	std::string value = "";
	std::string sectionNameINI = "";	
	SIZE_T sectionFound;
	boost::property_tree::ptree pt;
	
	boost::property_tree::read_ini(filename, pt);
	boost::property_tree::ptree::const_iterator it_end = pt.end();

	BOOST_FOREACH(auto it, pt){
		sectionNameINI = it.first;
		it.
		
	}

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
			add(Navitia(Server,Path));
		}
	}
}
*/
int main(int, char**){
    webservice::parse_params("hi");
}

