#include "stat.h"
#include <rapidxml.hpp>
#include <iostream>
#include <sstream>

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
	try{
		result = boost::lexical_cast<double>(value);
	}
	catch(boost::bad_lexical_cast &){
		result = default_value;
	}
return result;

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
    std::string result = "toto";
    return result;
};


ResponsePlanJourney::ResponsePlanJourney():user_id(0), wsn_id(0), response_ide(-1), interchange(0),
    total_link_time(boost::posix_time::second_clock::local_time()),
    journey_dateTime(boost::posix_time::second_clock::local_time()),
    isBest(false), isFirst(false), isLast(false),
    detail_index(-1), detail_count(0), comment_type(0){
};

//void ResponsePlanJourney::addDetail(const std::string reponse_navitia){

//};

void ResponsePlanJourney::add(DetailPlanJourney & detail){
    this->details.push_back(detail);
};

void ResponsePlanJourney::readXML(rapidxml::xml_node<> *Node){
    std::string strTotalLinkTime="";
    std::string strJourneyDuration="";
    std::string strIsFirst="";
    std::string strIsLast="";
    std::string strIsBest="";
    std::string strJourneyDateTime = "";
    std::string strNodeName = "";
    std::string attrName = "";

    //Lire les atributs et récupérer les information de ResponsePlanJourney
    for(rapidxml::xml_attribute<> * attr = Node->first_attribute(); attr; attr = attr->next_attribute()){

        attrName = attr->name();
        if (strcmp(attrName.c_str(), "Interchange") == 0){
			this->interchange = str_to_int_def(attr->value(), -1);
        }
        else if (strcmp(attrName.c_str(), "TotalLinkTime") == 0){
            strTotalLinkTime = attr->value();
        }
        else if (strcmp(attrName.c_str(), "JourneyDuration") == 0){
            strJourneyDuration = attr->value();
        }
        else if (strcmp(attrName.c_str(), "IsFirst") == 0){
            strIsFirst = attr->value();
        }
        else if (strcmp(attrName.c_str(), "IsBest") == 0){
            strIsBest = attr->value();
        }
        else if (strcmp(attrName.c_str(), "IsLast") == 0){
            strIsLast = attr->value();
        }
        else if (strcmp(attrName.c_str(), "JourneyDateTime") == 0){
            strJourneyDateTime = attr->value();
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
            this->add(detail);
            detail.readXML(detailNode);
        }
    }

};

std::string ResponsePlanJourney::writeXML(){
    std::string result = "toto";
    return result;
};

std::string ResponsePlanJourney::getSql(){

	std::string requete_ResponsePlanJourney = "";
	/*
	requete_ResponsePlanJourney += ks_table_ResponsePlanJourney_insert + (boost::format(ks_table_ResponsePlanJourney_values)

		% "@PJO_IDE" 
		% this->journey_dateTime.date().month()
		% this->interchange
		% this->total_link_time.date()
		% this->call_dateTime.time_of_day().hours()//5
		% this->call_dateTime.time_of_day().minutes()

		% this->server_info

		% this->plan_dateTime
		% this->plan_dateTime.date().year()
		% this->plan_dateTime.date().month()//10
		% this->plan_dateTime.date().day()
		% this->plan_dateTime.time_of_day().hours()
		% this->plan_dateTime.time_of_day().minutes()

		% this->depType_value
		% this->dep_external_code//15
		% this->dep_city_External_code
		% format_double(this->dep_coord.X, 10, 2, 0.0) //17
		% format_double(this->dep_coord.Y, 10, 2, 0.0)//18

		% this->destType_value
		% this->dest_external_code//20
		% this->dest_city_External_code
		% format_double(this->dest_coord.X, 10, 2, 0.0)//22
		% format_double(this->dest_coord.Y, 10, 2, 0.0)//23

		% this->sens
		% this->criteria//25
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
		).str() + ");";
	*/
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
    std::string strRequestDate = "";
    std::string strPlanDateTime = "";
    std::string strDepPointType = "";
    std::string strDestPointType = "";
    std::string strCumulCalcDuration = "";
    std::string strViaConnectionDuration = "";
    int intCriteria;

    //Lire les atributs et récupérer les information de PlanJourney
    for(rapidxml::xml_attribute<> * attr = Node->first_attribute(); attr; attr = attr->next_attribute()){
        attrName = attr->name();
        if (strcmp(attrName.c_str(), "RequestDate") == 0){
			//this->call_dateTime = boost::posix_time::time_from_string(attr->value());
			//this->call_dateTime = boost::posix_time::time_from_string ("2010-01-31");
			this->call_dateTime = seconds_from_epoch (attr->value());
			
        }
        else if (strcmp(attrName.c_str(), "Server") == 0){
            this->server_info = attr->value();
        }
        else if (strcmp(attrName.c_str(), "PlanDateTime") == 0){
            strPlanDateTime = attr->value();
        }
        else if (strcmp(attrName.c_str(), "DepPointType") == 0){
			depType_value = str_to_int_def(attr->value(), -1);
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
			destType_value = str_to_int_def(attr->value(), -1);
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
            intCriteria = str_to_int_def(attr->value(), -1);
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
            this->add(response);
            response.readXML(responseNode);
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
	requete_PlanJourney += ks_table_planjourney_insert + (boost::format(ks_table_planjourney_values)

		% this->call_dateTime 
		% this->call_dateTime.date().year()
		% this->call_dateTime.date().month()
		% this->call_dateTime.date().day()
		% this->call_dateTime.time_of_day().hours()//5
		% this->call_dateTime.time_of_day().minutes()

		% this->server_info

		% this->plan_dateTime
		% this->plan_dateTime.date().year()
		% this->plan_dateTime.date().month()//10
		% this->plan_dateTime.date().day()
		% this->plan_dateTime.time_of_day().hours()
		% this->plan_dateTime.time_of_day().minutes()

		% this->depType_value
		% this->dep_external_code//15
		% this->dep_city_External_code
		% format_double(this->dep_coord.X, 10, 2, 0.0) //17
		% format_double(this->dep_coord.Y, 10, 2, 0.0)//18

		% this->destType_value
		% this->dest_external_code//20
		% this->dest_city_External_code
		% format_double(this->dest_coord.X, 10, 2, 0.0)//22
		% format_double(this->dest_coord.Y, 10, 2, 0.0)//23

		% this->sens
		% this->criteria//25
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
		).str() + ");";
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
            this->wsn_id = str_to_int_def(attr->value(), -1);
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
		% this->dateTime 
		//boost::posix_time::ptime
		% this->dateTime.date().year()
		% this->dateTime.date().month()
		% this->dateTime.date().day()
		% this->dateTime.time_of_day().hours()
		% this->dateTime.time_of_day().minutes()
		% this->dateTime.time_of_day()
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
		).str() + ");";
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

void StatNavitia::writeSql(){
	this->sql_requete = "";
	this->sql_requete += this->hit.getSql();
	this->sql_requete += this->planJourney.getSql();
	//this->planJourney.writeSql();
}


