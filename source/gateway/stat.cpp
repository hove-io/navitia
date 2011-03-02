#include "stat.h"
#include <rapidxml.hpp>
#include <rapidxml_print.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string> 
#include <boost/foreach.hpp>
#include "../SqlServer/mssql.h"
#include "file_utilities.h"

boost::posix_time::ptime seconds_from_epoch(const std::string& s) {
	boost::posix_time::ptime pt;
	for(size_t i=0; i<formats_n; ++i){
		std::istringstream is(s);
		is.imbue(formats[i]);
		is >> pt;
		if(pt != boost::posix_time::ptime()) break;
	}
	return pt;
}

std::string formatDateTime(boost::posix_time::ptime pt) {
	std::stringstream ss;
	ss.str("");
	ss<<pt.date().day();
	ss<<"-";
	ss<<pt.date().month().as_number();
	ss<<"-";
	ss<<pt.date().year();
	ss<<" ";
	ss<<pt.time_of_day().hours();
	ss<<":";
	ss<<pt.time_of_day().minutes();
	ss<<":";
	ss<<pt.time_of_day().seconds();
	return ss.str();
}

std::string format_double(double value, int lenth, int precesion, double default_value){
	std::string result;
	std::stringstream oss;
	oss<< "%";
	oss<< lenth;
	oss<< ".";
	oss<< precesion;
	oss<< "f";

	try{
		result = (boost::format(oss.str()) %value).str();
	}
	catch(int e){
		result = boost::lexical_cast<std::string>(default_value);
	}
return result;
}

int str_to_int_def(std::string value,int default_value){
	int result;
	try{
		result = boost::lexical_cast<int>(value);
	}
	catch(boost::bad_lexical_cast &){
		result = default_value;
	}
return result;
}

double str_to_float_def(std::string value,double default_value){
	double result;
	replace(value.begin(), value.end(), ',', '.');
	try{
		result = boost::lexical_cast<double>(value);
	}
	catch(boost::bad_lexical_cast &){
		result = default_value;
	}
return result;

}

PointType getpointTypeByCaption(const std::string strPointType){
	PointType pt=ptUndefined;

	for (int i = 0; i< (sizeof(PointTypeCaption)/sizeof(PointTypeCaption[0])); i++){
		if (strcmp(PointTypeCaption[i].c_str(), strPointType.c_str()) == 0){
			pt=(PointType) i;
			break;
		}
	}
	return pt;
}

Criteria getCriteriaByCaption(const std::string strCriteria){
	Criteria ct = cInitialization;
	for (int i = 0; i< (sizeof(CriteriaCaption)/sizeof(CriteriaCaption[0])); i++){
		if (strcmp(CriteriaCaption[i].c_str(), strCriteria.c_str()) == 0){
			ct=(Criteria) i;
			break;
		}
	}
	return ct;
}

bool strToBool(const std::string &strValue, bool defaultValue){
	bool result = defaultValue;
	for (int i = 0; i< (sizeof(TrueValue)/sizeof(TrueValue[0])); i++){
		if (strcmp(boost::to_upper_copy(TrueValue[i]).c_str(), boost::to_upper_copy(strValue).c_str()) == 0){
			result = true;
			break;
		}
	}
	return result;
}

void writeLineInFile(std::string & strline){
	std::ofstream logfile(gs_logFileName, std::ios::app);
	logfile<< strline;
}

std::string getApplicationPath(){
	char buf[2048];
	DWORD filePath = GetModuleFileName(NULL, buf, 2048);
	std::string::size_type posSlash = std::string(buf).find_last_of( "\\/" );
	std::string::size_type posDot = std::string(buf).find_last_of( "." );
	gs_applicationName = std::string(buf).substr(posSlash + 1, posDot - (posSlash - 1));
	return std::string(buf).substr( 0, posSlash);
}



DetailPlanJourney::DetailPlanJourney():user_id(0), wsn_id(0), response_ide(-1), dep_external_code(""),
    line_external_code(""), mode_external_code(""), company_external_code(""),
    network_external_code(""), route_external_code(""), arr_external_code(""),
    dep_dateTime(boost::posix_time::second_clock::local_time()),
    arr_dateTime(boost::posix_time::second_clock::local_time()), section_type(0){
};

void DetailPlanJourney::readXML(rapidxml::xml_node<> *Node){
    std::string attrName = "";
    std::string strDepDateTime = "";
    std::string strArrDateTime = "";

    //Lire les atributs et récupérer les information de DetailPlanJourney
    for(rapidxml::xml_attribute<> * attr = Node->first_attribute(); attr; attr = attr->next_attribute()){

        attrName = attr->name();
        if (strcmp(attrName.c_str(), "DepExternalCode") == 0){
            this->dep_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "DepCoordX") == 0){
			this->dep_coord.X = str_to_float_def(attr->value(), 0.00);
        }
        else if (strcmp(attrName.c_str(), "DepCoordY") == 0){
            this->dep_coord.Y = str_to_float_def(attr->value(), 0.00);
        }
        else if (strcmp(attrName.c_str(), "LineExternalCode") == 0){
            this->line_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "ModeExternalCode") == 0){
            this->mode_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "CompanyExternalCode") == 0){
            this->company_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "NetworkExternalCode") == 0){
            this->network_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "RouteExternalCode") == 0){
            this->route_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "ArrExternalCode") == 0){
            this->arr_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "ArrCoordX") == 0){
            this->arr_coord.X = str_to_float_def(attr->value(),0.00);
        }
        else if (strcmp(attrName.c_str(), "ArrCoordY") == 0){
            this->arr_coord.Y = str_to_float_def(attr->value(),0.00);
        }
        else if (strcmp(attrName.c_str(), "DepDateTime") == 0){
            strDepDateTime = attr->value();
        }
        else if (strcmp(attrName.c_str(), "ArrDateTime") == 0){
            strArrDateTime = attr->value();
        }
        else if (strcmp(attrName.c_str(), "SectionType") == 0){
            this->section_type = str_to_int_def(attr->value(), -1);
        }
    }

};

std::string DetailPlanJourney::writeXML(){
    std::string result = "toto";
    return result;
};

std::string DetailPlanJourney::getSql(){
    std::string requete_detailPlanJourney = "";
	requete_detailPlanJourney += ks_table_detailPlanJourney_insert+ (boost::format(ks_table_detailPlanJourney_values)

		% "@PJO_IDE" //1
		% "@RPJO_IDE"
		% "@PJO_MONTH"
		
		% this->dep_external_code //4
		% format_double(this->dep_coord.X, 10, 2, 0.0)
		% format_double(this->dep_coord.Y, 10, 2, 0.0)
		% this->line_external_code
		% this->mode_external_code
		% this->company_external_code
		% this->network_external_code
		% this->route_external_code
		
		% this->arr_external_code //12
		% format_double(this->arr_coord.X, 10, 2, 0.0)
		% format_double(this->arr_coord.Y, 10, 2, 0.0)

		//% this->dep_dateTime //15
		//% this->arr_dateTime
		% formatDateTime(this->dep_dateTime)
		% formatDateTime(this->arr_dateTime)

		% this->dep_dateTime.date().year()//17
		% this->dep_dateTime.date().month().as_number()
		% this->dep_dateTime.date().day()
		% this->dep_dateTime.time_of_day().hours()
		% this->dep_dateTime.time_of_day().minutes()

		% this->arr_dateTime.date().year()//22
		% this->arr_dateTime.date().month().as_number()
		% this->arr_dateTime.date().day()
		% this->arr_dateTime.time_of_day().hours()
		% this->arr_dateTime.time_of_day().minutes()

		% this->section_type //27
		% this->user_id
		% this->wsn_id//29
		).str() + ");"+ks_lineBreak;

	requete_detailPlanJourney += ks_errorExit;
	return requete_detailPlanJourney;
};


ResponsePlanJourney::ResponsePlanJourney():user_id(0), wsn_id(0), response_ide(-1), interchange(0),
    total_link_time(0.0),totalLink_hour(0),totalLink_minute(0),journey_duration(0.0), journeyDuration_hour(0),journeyDuration_minute(0),
    journey_dateTime(boost::posix_time::second_clock::local_time()),
    isBest(false), isFirst(false), isLast(false),
    detail_index(-1), detail_count(0), comment_type(0){
};

void ResponsePlanJourney::writeTotalLinkTime(){
	this->totalLink_hour = this->total_link_time * ki_hoursPerDay;
	this->totalLink_minute = (this->total_link_time * ki_minsPerDay) - (this->totalLink_hour * ki_minsPerHour);
	
}
void ResponsePlanJourney::writeJourneyDuration(){
	this->journeyDuration_hour = this->journey_duration * ki_hoursPerDay;
	this->journeyDuration_minute = (this->journey_duration * ki_minsPerDay) - (this->journeyDuration_hour * ki_minsPerHour);
	
}


void ResponsePlanJourney::add(DetailPlanJourney & detail){
    this->details.push_back(detail);
};

void ResponsePlanJourney::readXML(rapidxml::xml_node<> *Node){

	std::string strNodeName = "";
	std::string attrName = "";

    //Lire les atributs et récupérer les information de ResponsePlanJourney
    for(rapidxml::xml_attribute<> * attr = Node->first_attribute(); attr; attr = attr->next_attribute()){

        attrName = attr->name();
        if (strcmp(attrName.c_str(), "Interchange") == 0){
			this->interchange = str_to_int_def(attr->value(), -1);
        }
        else if (strcmp(attrName.c_str(), "TotalLinkTime") == 0){
			this->total_link_time = str_to_float_def(attr->value(), 0.0);
			this->writeTotalLinkTime();
        }
        else if (strcmp(attrName.c_str(), "JourneyDuration") == 0){
			this->journey_duration = str_to_float_def(attr->value(), 0.0);
			this->writeJourneyDuration();
        }
        else if (strcmp(attrName.c_str(), "IsFirst") == 0){
			this->isFirst = strToBool(attr->value(), false);
        }
        else if (strcmp(attrName.c_str(), "IsBest") == 0){
			this->isBest = strToBool(attr->value(), false);
        }
        else if (strcmp(attrName.c_str(), "IsLast") == 0){
			this->isLast = strToBool(attr->value(), false);
        }
        else if (strcmp(attrName.c_str(), "JourneyDateTime") == 0){
			this->journey_dateTime = seconds_from_epoch(attr->value());
        }
        else if (strcmp(attrName.c_str(), "CommentType") == 0){
            this->comment_type = str_to_int_def(attr->value(), -1);
        }
    }

    //Lire le noeud DetailPlanJourney
    for (rapidxml::xml_node<> *detailNode = Node->first_node(); detailNode; detailNode = detailNode->next_sibling()){
        strNodeName = detailNode->name();

        if (strcmp(strNodeName.c_str(), "DetailPlanJourney") == 0){
            DetailPlanJourney detail;
            detail.readXML(detailNode);
			this->add(detail);
            
        }
    }

};

std::string ResponsePlanJourney::writeXML(){
    std::string result = "toto";
    return result;
};

std::string ResponsePlanJourney::getSql(){

	std::string requete_ResponsePlanJourney = "";
	requete_ResponsePlanJourney += ks_table_ResponsePlanJourney_insert + (boost::format(ks_table_ResponsePlanJourney_values)

		% "@PJO_IDE" //1
		% this->journey_dateTime.date().month().as_number()
		% this->interchange
		% this->total_link_time
		% this->totalLink_hour
		% this->totalLink_minute
		
		// reste à faire
		% this->journey_duration//7
		% this->journeyDuration_hour
		% this->journeyDuration_minute

		% this->isFirst//10
		% this->isBest
		% this->isLast

		//% this->journey_dateTime//13
		% formatDateTime(this->journey_dateTime)
		% this->journey_dateTime.date().year()
		% this->journey_dateTime.date().month().as_number()
		% this->journey_dateTime.date().day()
		% this->journey_dateTime.time_of_day().hours()
		% this->journey_dateTime.time_of_day().minutes()

		//Méthode à faire GetTrancheShift(journey_dateTime)
		% 0//19
		% 0 // OK

		% this->comment_type//21
		% this->user_id
		% this->wsn_id//23
		).str() + ");"+ks_lineBreak;
	requete_ResponsePlanJourney += (boost::format(ks_scopeIdentity) % "RPJO_IDE").str() + ks_lineBreak;

	requete_ResponsePlanJourney += ks_errorExit;

	BOOST_FOREACH(DetailPlanJourney & dpj, this->details) {
		requete_ResponsePlanJourney += dpj.getSql();
	}
	return requete_ResponsePlanJourney;
}

PlanJourney::PlanJourney(): user_id(0), wsn_id(0), server_info(""), script_info(""),
                            plan_dateTime(boost::posix_time::second_clock::local_time()),
                            call_dateTime(boost::posix_time::second_clock::local_time()),
                            dep_external_code(""), dep_city_External_code(""),
                            dest_external_code(""), dest_city_External_code(""), sens(0),
                            mode(0), mode_string(""), walk_speed(0), equipement(0), equipement_string(""),
                            vehicle(0), vehicle_string(""),error(0),
                            hang_distance(0), dep_hang_distance(0), dest_hang_distance(0),
                            via_external_code(""), manage_disrupt(false),
                            forbidden_SA_external_code(""), forbidden_line_external_code(""){
    //Initialiser les propriétés

};

void PlanJourney::readXML(rapidxml::xml_node<> *Node){
    std::string strNodeName = "";
    std::string attrName = "";
    //std::string strRequestDate = "";
    //std::string strPlanDateTime = "";
    std::string strCumulCalcDuration = "";
    std::string strViaConnectionDuration = "";
    //int intCriteria;

    //Lire les atributs et récupérer les information de PlanJourney
    for(rapidxml::xml_attribute<> * attr = Node->first_attribute(); attr; attr = attr->next_attribute()){
        attrName = attr->name();
        if (strcmp(attrName.c_str(), "RequestDate") == 0){
			this->call_dateTime = seconds_from_epoch (attr->value());
		}
        else if (strcmp(attrName.c_str(), "Server") == 0){
            this->server_info = attr->value();
        }
        else if (strcmp(attrName.c_str(), "PlanDateTime") == 0){
            this->plan_dateTime = seconds_from_epoch (attr->value());
        }
        else if (strcmp(attrName.c_str(), "DepPointType") == 0){
			this->depType = getpointTypeByCaption(attr->value());
        }
        else if (strcmp(attrName.c_str(), "DepPointExternalCode") == 0){
            this->dep_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "DepCityExternalCode") == 0){
            this->dep_city_External_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "DepCoordX") == 0){
            this->dep_coord.X  = str_to_float_def(attr->value(), 0.00);
        }
        else if (strcmp(attrName.c_str(), "DepCoordY") == 0){
            this->dep_coord.Y = str_to_float_def(attr->value(), 0.00);
        }
        else if (strcmp(attrName.c_str(), "DestPointType") == 0){
			this->destType = getpointTypeByCaption(attr->value());
        }
        else if (strcmp(attrName.c_str(), "DestPointExternalCode") == 0){
            this->dest_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "DestCityExternalCode") == 0){
            this->dest_city_External_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "DestCoordX") == 0){
            this->dest_coord.X = str_to_float_def(attr->value(), 0.00);
        }
        else if (strcmp(attrName.c_str(), "DestCoordY") == 0){
            this->dest_coord.Y = str_to_float_def(attr->value(), 0.00);
        }
        else if (strcmp(attrName.c_str(), "Sens") == 0){
            this->sens = str_to_int_def(attr->value(), -1);
        }
        else if (strcmp(attrName.c_str(), "Criteria") == 0){
            this->criteria = getCriteriaByCaption(attr->value());
        }
        else if (strcmp(attrName.c_str(), "Mode") == 0){
            this->mode_string = attr->value();
        }
        else if (strcmp(attrName.c_str(), "WalkSpeed") == 0){
            this->walk_speed = str_to_int_def(attr->value(), -1);
        }
        else if (strcmp(attrName.c_str(), "Equipment") == 0){
            this->equipement_string = attr->value();
        }
        else if (strcmp(attrName.c_str(), "Vehicle") == 0){
            this->vehicle_string = attr->value();
        }
        else if (strcmp(attrName.c_str(), "CumulCalcDuration") == 0){
            strCumulCalcDuration = attr->value();
        }
        else if (strcmp(attrName.c_str(), "HangDistance") == 0){
            this->hang_distance = str_to_int_def(attr->value(), -1);
        }
        else if (strcmp(attrName.c_str(), "HangDistanceDep") == 0){
            this->dep_hang_distance= str_to_int_def(attr->value(), -1);
        }
        else if (strcmp(attrName.c_str(), "HangDistanceArr") == 0){
            this->dest_hang_distance = str_to_int_def(attr->value(), -1);
        }
        else if (strcmp(attrName.c_str(), "ViaExternalCode") == 0){
            this->via_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "ViaConnectionDuration") == 0){
            strViaConnectionDuration = attr->value();
        }
        else if (strcmp(attrName.c_str(), "ManageDisrupt") == 0){
            this->manage_disrupt = str_to_int_def(attr->value(), -1);
        }
        else if (strcmp(attrName.c_str(), "ForbiddenStopAreaExtCode") == 0){
            this->forbidden_SA_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "ForbiddenLineExtCode") == 0){
            this->forbidden_line_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "error") == 0){
            this->error = str_to_int_def(attr->value(),-1);
        }
    }

    //Lire le noeud ResponsePlanJourney
    for (rapidxml::xml_node<> *responseNode = Node->first_node(); responseNode; responseNode = responseNode->next_sibling()){
        strNodeName = responseNode->name();

        if (strcmp(strNodeName.c_str(), "ResponsePlanJourney") == 0){
            ResponsePlanJourney response;
            response.readXML(responseNode);
			this->add(response);
            
        }
    }
};



void PlanJourney::add(ResponsePlanJourney & response){
     this->responses.push_back(response);
};

std::string PlanJourney::writeXML(){
    std::string result = "toto";
    return result;
};

std::string PlanJourney::getSql(){
	
	std::string requete_PlanJourney = "";
	//SET @PJO_MONTH=2;
	requete_PlanJourney += (boost::format(ks_pjo_month) % this->call_dateTime.date().month().as_number()).str();
	//Insert into pjo_temp
	requete_PlanJourney += ks_table_planjourney_insert + (boost::format(ks_table_planjourney_values)

		//% this->call_dateTime 
		% formatDateTime(this->call_dateTime)
		% this->call_dateTime.date().year()
		% this->call_dateTime.date().month().as_number()
		% this->call_dateTime.date().day()
		% this->call_dateTime.time_of_day().hours()//5
		% this->call_dateTime.time_of_day().minutes()

		% this->server_info

		//% this->plan_dateTime
		% formatDateTime(this->plan_dateTime)
		% this->plan_dateTime.date().year()
		% this->plan_dateTime.date().month().as_number()//10
		% this->plan_dateTime.date().day()
		% this->plan_dateTime.time_of_day().hours()
		% this->plan_dateTime.time_of_day().minutes()

		% this->depType
		//% this->depType_value
		% this->dep_external_code//15
		% this->dep_city_External_code
		% format_double(this->dep_coord.X, 10, 2, 0.0) //17
		% format_double(this->dep_coord.Y, 10, 2, 0.0)//18

		% this->destType
		% this->dest_external_code//20
		% this->dest_city_External_code
		% format_double(this->dest_coord.X, 10, 2, 0.0)//22
		% format_double(this->dest_coord.Y, 10, 2, 0.0)//23

		% this->sens
		//% CriteriaCaption[boost::lexical_cast<int> (this->criteria)]//25
		% this->criteria
		% this->mode
		% this->walk_speed
		% this->equipement
		% this->vehicle
		% this->total_plan_duration//30
		
		% this->hang_distance
		% this->dep_hang_distance
		% this->dest_hang_distance
		% this->via_external_code
		% this->via_connection_duration//35
		% this->manage_disrupt
		% this->forbidden_SA_external_code
		% this->forbidden_line_external_code
		
		% "@HIT_IDE"
		% this->error//40
		% this->user_id
		% this->wsn_id
		).str() + ");"+ks_lineBreak;

	requete_PlanJourney += (boost::format(ks_scopeIdentity) % "PJO_IDE").str() +ks_lineBreak;
	requete_PlanJourney += ks_errorExit;
	
	// liste des réponses
	BOOST_FOREACH(ResponsePlanJourney & rpj, this->responses) {
		requete_PlanJourney += rpj.getSql();
	}
	return requete_PlanJourney;
}

Hit::Hit(): user_id(0), wsn_id(0), dateTime(boost::posix_time::second_clock::local_time()),
            website(""), server_ip(""), client_ip(""),
            client_login(""), script_info(""), action(""), response_size(0), api_cost(0){

};

void Hit::readXML(rapidxml::xml_node<> *Node){
    std::string attrName = "";
    std::string strDay = "";
    std::string strMonth = "";
    std::string strTime = "";

    for(rapidxml::xml_attribute<> * attr = Node->first_attribute(); attr; attr = attr->next_attribute()){

        attrName = attr->name();
        if (strcmp(attrName.c_str(), ks_param_read_date.c_str()) == 0){
        strDay = attr->value();
        }
        else if (strcmp(attrName.c_str(), ks_param_read_month.c_str()) == 0){
            strMonth = attr->value();
        }
        else if (strcmp(attrName.c_str(), ks_param_read_time.c_str()) == 0){
            strTime = attr->value();
        }
        else if (strcmp(attrName.c_str(), ks_param_read_action.c_str()) == 0){
            this->action = attr->value();
        }
        else if (strcmp(attrName.c_str(), ks_param_read_response_size.c_str()) == 0){
            this->response_size = str_to_int_def(attr->value(), -1);
        }
        else if (strcmp(attrName.c_str(), ks_param_read_ide.c_str()) == 0){
            this->user_id = str_to_int_def(attr->value(), 0);
        }
        else if (strcmp(attrName.c_str(), ks_param_read_wsn_id.c_str()) == 0){
            this->wsn_id = str_to_int_def(attr->value(), 0);
        }
	}
	this->client_ip = "localhost";

};

std::string Hit::writeXML(){
    std::string result = "toto";
    return result;
};

std::string Hit::getSql(){
	std::string requete_hit = "";
	requete_hit += ks_table_hit_insert + (boost::format(ks_table_hit_values) 
		//% this->dateTime 
		% formatDateTime(this->dateTime)
		//boost::posix_time::ptime
		% this->dateTime.date().year()
		% this->dateTime.date().month().as_number()
		% this->dateTime.date().day()
		% this->dateTime.time_of_day().hours()
		% this->dateTime.time_of_day().minutes()
		//% this->dateTime.time_of_day()
		// à faire
		% 0
		% this->server_ip
		% this->user_id
		% this->action
		//Reste à convertir
		% this->response_duration
		% this->response_size
		% this->script_info
		% this->wsn_id
		% this->api_cost
		% this->client_ip
		).str() + ");"+ks_lineBreak;

	requete_hit += (boost::format(ks_scopeIdentity) % "HIT_IDE").str() + ks_lineBreak;
	requete_hit += ks_errorExit;

	return requete_hit;
	
}

StatNavitia::StatNavitia(){
};

void StatNavitia::readXML(const std::string reponse_navitia){

    //Utilisation de RapidXML pour parser le flux XML de HIT
    rapidxml::xml_document<> xmlDoc;
    rapidxml::xml_node<> * HitNode = NULL;
	rapidxml::xml_node<> * Node = NULL;
    std::string nodeName = "";
    char * data_ptr = xmlDoc.allocate_string(reponse_navitia.c_str());

    xmlDoc.parse<0>(data_ptr);

    //HitNode = xmlDoc.first_node("Hit");
	Node = xmlDoc.first_node();
	for (rapidxml::xml_node<> * HitNode = Node->first_node(); HitNode; HitNode = HitNode->next_sibling()){
		nodeName = HitNode->name();
		if (nodeName=="Hit"){
		
			//Appeler la méthode readXML de hit en passant le neoud HitNode;
			this->hit.readXML(HitNode);

			for (rapidxml::xml_node<> * PlanNode = HitNode->first_node(); PlanNode; PlanNode = PlanNode->next_sibling()){
				nodeName = PlanNode->name();

				//si c'est le node "PlanJourney" alors appeler la méthode readXML de Planjourney en passant
				//le neoud PlanNode;
				if (nodeName=="PlanJourney"){
					this->planJourney.readXML(PlanNode);
				}
			}
		}
	}
};

std::string StatNavitia::writeXML(){
    std::string result = "toto";
    return result;
};

void StatNavitia::writeSQLInFile(){
	std::ofstream statfile(gs_statFileName, std::ios::app);
	statfile<< this->sql_requete;
}

void StatNavitia::writeSql(){
	this->sql_requete = "";
	this->sql_requete += this->hit.getSql();
	this->sql_requete += this->planJourney.getSql();
	this->writeSQLInFile();
	
}

std::string StatNavitia::delete_node_hit(std::string & response_navitia){
	
	//Utilisation de RapidXML pour parser le flux XML de HIT
	std::stringstream ss;
	rapidxml::xml_document<> xmlDoc;
    rapidxml::xml_node<> * HitNode = NULL;
	rapidxml::xml_node<> * Node = NULL;
    std::string nodeName = "";
    char * data_ptr = xmlDoc.allocate_string(response_navitia.c_str());

    xmlDoc.parse<0>(data_ptr);

    Node = xmlDoc.first_node();
	for (rapidxml::xml_node<> * HitNode = Node->first_node(); HitNode; HitNode = HitNode->next_sibling()){
		nodeName = HitNode->name();
		if (nodeName=="Hit"){
			Node->remove_node(HitNode);
			break;
		}
	}
	ss << *xmlDoc.first_node();
	response_navitia = ks_header_xml + ss.str();	
	return response_navitia;
}

// constructeur par défaul
ClockThread::ClockThread(): th_stoped(false) {
	this->start();
	std::pair<std::string, std::string> application_params = initFileParams();
	gs_applicationName = application_params.first;
	gs_filePathName = application_params.second;

}
// démarrage du thread
void ClockThread::start(){
	assert(!m_thread);
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&ClockThread::work, this)));
}
// arrêt du thread
void ClockThread::stop(){
	assert(m_thread);
	th_stoped = true;
	m_thread->join();
}
// mettre à disposition des autres threads un nouveau fichier
void ClockThread::createNewFileName(){
	// Stat_MMDD-HHNNSS-MS_SERVER_WsnId
	std::stringstream ss;
	ss.clear();
	boost::posix_time::ptime locale_dateTime = boost::posix_time::second_clock::local_time();

	ss<< "STAT_";
	ss<< locale_dateTime.date().month().as_number();
	ss<< locale_dateTime.date().day();
	ss<< "-";
	ss<< locale_dateTime.time_of_day().hours();
	ss<< locale_dateTime.time_of_day().minutes();
	ss<< locale_dateTime.time_of_day().seconds();
	ss<< "-";
	// MS
	ss<< locale_dateTime.time_of_day().total_milliseconds();
	ss<< "_";
	ss<< gs_serverName;
	ss<< "_";
	ss<< gi_wsn_id;
	ss <<".txt";
	this->m_mutex.lock();
	gs_statFileName = gs_filePathName + ss.str(); 
	this->m_mutex.unlock();
}
void ClockThread::work(){

	std::stringstream ss;
	boost::xtime xt; 
	while (!th_stoped){
		// le premier traitement : Attendre jusqu'à la fin du cycle
		//Début du cycle
		boost::xtime_get(&xt, boost::TIME_UTC); 
		xt.sec += 1 * gi_clockTimer; 
		// Créer un nouveau fichier
		this->createNewFileName();

		boost::thread::sleep(xt);
		//Traiter les fichiers de stat sauf le dérnier 
		this->getFileList();
		//Lancer les requêts sql / gerer les erreurs / supprimer le fichier traité avec succés 
		this->saveStatFromFileList();

		boost::posix_time::ptime locale_dateTime = boost::posix_time::second_clock::local_time();
		ss.str("");
		ss<< locale_dateTime<<std::endl;
		//writeLineInFile(ss.str());
	} // Fin du clock
}
void ClockThread::getFileList(){
	boost::filesystem::directory_iterator end_itr; 
	// Default ctor yields past-the-end 
	fileList.clear();
	for( boost::filesystem::directory_iterator i( gs_filePathName ); i != end_itr; i++ ) {
		// Skip if not a file
		if( !boost::filesystem::is_regular_file( i->status() ) ) 
			continue;
		boost::smatch what;
		// Skip if no match     
		if( !boost::regex_match( i->filename(), what, gs_statFileFilter ) ) 
			continue;      
		// File matches, store it
		if (i->leaf() != gs_statFileName){
			fileList.push_back( i->filename() ); 
		}
	} 
}
void ClockThread::saveStatFromFileList(){

	BOOST_FOREACH(std::string & fileName, this->fileList) {
		this->saveStatFromFile(fileName);
	}
}
void ClockThread::deleteStatFile(std::string & fileName){
	if(boost::filesystem::exists(gs_filePathName+fileName)){
		boost::filesystem::remove(gs_filePathName+fileName);
		std::stringstream ss;
		ss<<fileName;
		ss<<std::endl;
		writeLineInFile(ss.str());
	}
}
void ClockThread::renameStatFile(std::string & fileName){
	if(boost::filesystem::exists(gs_filePathName+fileName)){
		boost::filesystem::rename(gs_filePathName+fileName, gs_filePathName + (boost::format(gs_statErrorFileName) % fileName).str());
	}
}
void ClockThread::saveStatFromFile(std::string & fileName){
	std::string lineSql; 
	std::stringstream ss;
	boost::iostreams::stream<boost::iostreams::file_source> file(gs_filePathName+fileName.c_str());
	//std::ifstream
	while (std::getline(file, lineSql)) {
		ss<< lineSql;
	}
	file.close();
	lineSql = ss.str();
	lineSql = ks_Begin + lineSql + (boost::format(ks_End) % fileName).str();
	//Sql::MSSql conn(gs_serverDb, gs_userDb, gs_pwdDb, gs_nameDb);
	//Sql::Result res = conn.exec(lineSql);
	this->deleteStatFile(fileName);
	//this->renameStatFile(fileName);
}
