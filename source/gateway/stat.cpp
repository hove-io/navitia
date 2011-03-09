#include "stat.h"
#include <rapidxml.hpp>
#include <rapidxml_print.hpp>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string> 
#include <boost/foreach.hpp>
//#include "../SqlServer/mssql.h"
#include "configuration.h"


#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>

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

std::string format_double(double value, int precision){
	std::stringstream oss;
    oss.precision(precision);
    oss << value;
    return oss.str();
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

PointType getpointTypeByCaption(const std::string & strPointType){
    PointType pt= Undefined;

	for (unsigned int i = 0; i< (sizeof(PointTypeCaption)/sizeof(PointTypeCaption[0])); i++){
		if (strcmp(PointTypeCaption[i].c_str(), strPointType.c_str()) == 0){
			pt=(PointType) i;
			break;
		}
	}
	return pt;
}

Criteria getCriteriaByCaption(const std::string & strCriteria){
    return static_data::get()->criterias.right.at(strCriteria);
    /*Criteria ct = cInitialization;
	for (unsigned int i = 0; i< (sizeof(CriteriaCaption)/sizeof(CriteriaCaption[0])); i++){
		if (strcmp(CriteriaCaption[i].c_str(), strCriteria.c_str()) == 0){
			ct=(Criteria) i;
			break;
		}
	}
    return ct;*/
}

bool strToBool(const std::string &strValue, bool defaultValue){
	bool result = defaultValue;
	for (unsigned int i = 0; i< (sizeof(TrueValue)/sizeof(TrueValue[0])); i++){
		if (strcmp(boost::to_upper_copy(TrueValue[i]).c_str(), boost::to_upper_copy(strValue).c_str()) == 0){
			result = true;
			break;
		}
	}
	return result;
}

/*void writeLineInLogFile(const std::string & strline){
	boost::posix_time::ptime locale_dateTime = boost::posix_time::second_clock::local_time();
	Configuration * conf = Configuration::get();    
	std::ofstream logfile(conf->get_string("path") + conf->get_string("application") + ".log", std::ios::app);
	logfile << locale_dateTime;
	logfile << "  =>  ";
	logfile << strline;
	logfile << ks_lineBreak;
}*/


DetailPlanJourney::DetailPlanJourney() : user_id(0), wsn_id(0), response_ide(-1), depType(City), arrType(City),
    dep_dateTime(boost::posix_time::second_clock::local_time()),
    arr_dateTime(boost::posix_time::second_clock::local_time()), section_type(0){
}

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
            this->dep_coord.x = str_to_float_def(attr->value(), 0.00);
        }
        else if (strcmp(attrName.c_str(), "DepCoordY") == 0){
            this->dep_coord.y = str_to_float_def(attr->value(), 0.00);
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
            this->arr_coord.x = str_to_float_def(attr->value(),0.00);
        }
        else if (strcmp(attrName.c_str(), "ArrCoordY") == 0){
            this->arr_coord.y = str_to_float_def(attr->value(),0.00);
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

}

std::string DetailPlanJourney::writeXML() const{
    std::string result = "toto";
    return result;
}

std::string DetailPlanJourney::getSql() const{
    std::string request = (boost::format("insert into  DPJO_TEMP (PJO_IDE, RPJ_IDE, PJO_REQUEST_MONTH, TRJ_DEP_ETERNAL_CODE, TRJ_DEP_COORD_X, TRJ_DEP_COORD_Y, TRJ_LINE_EXTERNAL_CODE, TRJ_MODE_EXTERNAL_CODE, TRJ_COMPANY_EXTERNAL_CODE, TRJ_NETWORK_EXTERNAL_CODE, TRJ_ROUTE_EXTERNAL_CODE, TRJ_ARR_EXTERNAL_CODE, TRJ_ARR_COORD_X, TRJ_ARR_COORD_Y, TRJ_DEP_DATETIME, TRJ_ARR_DATETIME, TRJ_DEP_YEAR, TRJ_DEP_MONTH, TRJ_DEP_DAY, TRJ_DEP_HOUR, TRJ_DEP_MINUTE, TRJ_ARR_YEAR, TRJ_ARR_MONTH, TRJ_ARR_DAY, TRJ_ARR_HOUR, TRJ_ARR_MINUTE, TRJ_SECTIONTYPE, USE_ID, WSN_ID) values (%1%, %2%, %3%, '%4%', %5%, %6%, '%7%', '%8%', '%9%', '%10%', '%11%', '%12%', %13%, %14%, '%15%', '%16%', %17%, %18%, %19%, %20%, %21%, %22%, %23%, %24%, %25%, %26%, %27%, %28%, %29%);\n SET @ERR = @@ERROR;\n IF @ERR <> 0 GOTO sortie;\n")
        % "@PJO_IDE" //1
		% "@RPJO_IDE"
		% "@PJO_MONTH"
		
		% this->dep_external_code //4
        % format_double(this->dep_coord.x)
        % format_double(this->dep_coord.y)
		% this->line_external_code
		% this->mode_external_code
		% this->company_external_code
		% this->network_external_code
		% this->route_external_code
		
		% this->arr_external_code //12
        % format_double(this->arr_coord.x)
        % format_double(this->arr_coord.y)

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
        ).str();
    return request;
}


const std::string ResponsePlanJourney::xml_node_name = "ResponsePlanJourney";
ResponsePlanJourney::ResponsePlanJourney() : user_id(0), wsn_id(0), response_ide(-1), interchange(0),
    total_link_time(0.0),totalLink_hour(0),totalLink_minute(0),journey_duration(0.0), journeyDuration_hour(0),journeyDuration_minute(0),
    journey_dateTime(boost::posix_time::second_clock::local_time()),
    isBest(false), isFirst(false), isLast(false),
    detail_index(-1), detail_count(0), comment_type(0){
}

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
}

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

}

std::string ResponsePlanJourney::writeXML() const{
    std::string result = "toto";
    return result;
}

std::string ResponsePlanJourney::getSql() const{

    std::string request = (boost::format("insert into RPJO_TEMP (PJO_IDE, PJO_REQUEST_MONTH, RPJ_INTERCHANGE, RPJ_TOTAL_LINK_TIME, RPJ_TOTAL_LINK_TIME_HOUR, RPJ_TOTAL_LINK_TIME_MINUTE, RPJ_JOURNEY_DURATION, RPJ_JOURNEY_DURATION_HOUR, RPJ_JOURNEY_DURATION_MINUTE, RPJ_IS_FIRST, RPJ_IS_BEST, RPJ_IS_LAST, RPJ_JOURNEY_DATE_TIME, RPJ_JOURNEY_YEAR, RPJ_JOURNEY_MONTH, RPJ_JOURNEY_DAY, RPJ_JOURNEY_HOUR, RPJ_JOURNEY_MINUTE, RPJ_TRANCHE_SHIFT,RPJ_TRANCHE_DURATION,RPJ_COMMENT_TYPE,USE_ID,WSN_ID) values (%1%, %2%, %3%, %4%, %5%, %6%, %7%, %8%, %9%, %10%, %11%, %12%,'%13%', %14%,%15%,%16%,%17%,%18%, %19%, %20%,%21%, %22%, %23%);\nSET @RPJO_IDE = (SELECT SCOPE_IDENTITY());\n SET @ERR = @@ERROR;\n IF @ERR <> 0 GOTO sortie;\n")

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
        ).str();

    BOOST_FOREACH(const DetailPlanJourney & dpj, this->details) {
        request += dpj.getSql();
    }
    return request;
}

PlanJourney::PlanJourney(): user_id(0), wsn_id(0), plan_dateTime(boost::posix_time::second_clock::local_time()),
                            call_dateTime(boost::posix_time::second_clock::local_time()),
                            depType_value(0), depType(City), destType(City), destType_value(0),
                            sens(0), criteria(Initialization), mode(0), walk_speed(0), equipement(0),
                            vehicle(0), total_plan_duration(0),
                            error(0), hang_distance(0), dep_hang_distance(0), dest_hang_distance(0),
                            via_connection_duration(0), manage_disrupt(false)
{
    //Initialiser les propriétés

}

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
            this->dep_coord.x  = str_to_float_def(attr->value(), 0.00);
        }
        else if (strcmp(attrName.c_str(), "DepCoordY") == 0){
            this->dep_coord.y = str_to_float_def(attr->value(), 0.00);
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
            this->dest_coord.x = str_to_float_def(attr->value(), 0.00);
        }
        else if (strcmp(attrName.c_str(), "DestCoordY") == 0){
            this->dest_coord.y = str_to_float_def(attr->value(), 0.00);
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
}



void PlanJourney::add(ResponsePlanJourney & response){
     this->responses.push_back(response);
}

std::string PlanJourney::writeXML() const{
    std::string result = "toto";
    return result;
}
//

std::string PlanJourney::getSql() const{
    std::string request = (boost::format("insert into PJO_TEMP (PJO_REQUEST_DATE, PJO_REQUEST_YEAR, PJO_REQUEST_MONTH, PJO_REQUEST_DAY, PJO_REQUEST_HOUR, PJO_REQUEST_MINUTE, PJO_SERVER, PJO_PLAN_DATE_TIME, PJO_PLAN_YEAR, PJO_PLAN_MONTH, PJO_PLAN_DAY, PJO_PLAN_HOUR, PJO_PLAN_MINUTE, PJO_DEP_POINT_TYPE, PJO_DEP_POINT_EXTERNAL_CODE, PJO_DEP_CITY_EXTERNAL_CODE, PJO_DEP_COORD_X, PJO_DEP_COORD_Y, PJO_DEST_POINT_TYPE, PJO_DEST_POINT_EXTERNAL_CODE, PJO_DEST_CITY_EXTERNAL_CODE, PJO_DEST_COORD_X, PJO_DEST_COORD_Y, PJO_SENS, PJO_CRITERIA, PJO_MODE, PJO_WALK_SPEED, PJO_EQUIPMENT, PJO_VEHICLE, PJO_CUMUL_DUREE_CALC, PJO_HANG, PJO_DEP_HANG, PJO_DEST_HANG, PJO_VIA_EXTERNAL_CODE, PJO_VIA_CONNECTION_DURATION, PJO_MANAGE_DISRUPT,PJO_FORBIDDEN_SA_EXTERNAL_CODE, PJO_FORBIDDEN_LINE_EXTERNAL_CODE, HIT_IDE,PJO_ERROR,USE_ID,WSN_ID ) values ('%1%', %2%, %3%, %4%, %5%, %6%, '%7%', '%8%',%9%, %10%,%11%,%12%,%13%, %14%, '%15%', '%16%', %17%, %18%, %19%, '%20%','%21%', %22%, %23%, %24%, '%25%','%26%', %27%, '%28%', '%29%', %30%, %31%, %32%, %33%, '%34%', %35%, %36%, '%37%','%38%', %39%, %40%, %41%, %42%);\nSET @PJO_MONTH=%43%;\n IF ((@PJO_MONTH IS NULL) OR (@PJO_MONTH=0) OR (@PJO_MONTH>12)) SET @PJO_MONTH=1;\nSET @PJO_IDE = (SELECT SCOPE_IDENTITY());\nSET @ERR = @@ERROR; IF @ERR <> 0 GOTO sortie;\n")

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
        % format_double(this->dep_coord.x) //17
        % format_double(this->dep_coord.y)//18

		% this->destType
		% this->dest_external_code//20
		% this->dest_city_External_code
        % format_double(this->dest_coord.x)//22
        % format_double(this->dest_coord.y)//23

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
        % this->call_dateTime.date().month().as_number()
        ).str();
	
	// liste des réponses
    BOOST_FOREACH(const ResponsePlanJourney & rpj, this->responses) {
        request += rpj.getSql();
    }
    return request;
}

Hit::Hit(): dateTime(boost::posix_time::second_clock::local_time()), wsn_id(0), user_id(0), response_duration(0), response_size(0), api_cost(0){

}

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

}

std::string Hit::writeXML() const{
    std::string result = "toto";
    return result;
}

std::string Hit::getSql() const{
    std::string request = (boost::format("insert into HIT_TEMP (HIT_DATE, HIT_YEAR, HIT_MONTH,HIT_DAY,HIT_HOUR,HIT_MINUTE, HIT_TIME, HIT_SERVER, USE_ID, HIT_ACTION, HIT_DURATION, HIT_RESPONSE_SIZE, HIT_SCRIPT, WSN_ID, HIT_COST, HIT_CLIENT_IP) values ('%1%', %2%, %3%, %4%, %5%, %6%, '%7%', '%8%', %9%, '%10%', %11%, %12%, '%13%', %14%, %15%, '%16%');\nSET @HIT_IDE = (SELECT SCOPE_IDENTITY());\nSET @ERR = @@ERROR;\n IF @ERR <> 0 GOTO sortie;\n")
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
        ).str();

    return request;
	
}

StatNavitia::StatNavitia(){
    //stats_file = Configuration::get()->strings["stats_file"];
}

void StatNavitia::readXML(const std::string & reponse_navitia){

    //Utilisation de RapidXML pour parser le flux XML de HIT
    rapidxml::xml_document<> xmlDoc;
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
}

std::string StatNavitia::writeXML() const{
    std::string result = "toto";
    return result;
}

void StatNavitia::writeSQLInFile(const std::string & request) const{
    std::string stats_file = Configuration::get()->get_string("stats_file");
    std::ofstream statfile(stats_file, std::ios::app);
    statfile<< request;
}

void StatNavitia::writeSql() const{
    std::string request = this->hit.getSql();
    request += this->planJourney.getSql();
    this->writeSQLInFile(request);
}

std::string StatNavitia::delete_node_hit(std::string & response_navitia){
	
	//Utilisation de RapidXML pour parser le flux XML de HIT
	std::stringstream ss;
	rapidxml::xml_document<> xmlDoc;
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
    Configuration * conf = Configuration::get();
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
    ss<< conf->get_int("wsn_id");
    ss <<".txt";
    conf->set_string("stats_file", conf->get_string("path") + ss.str());
}
void ClockThread::work(){

    int timer = Configuration::get()->get_int("clock_timer");
	std::stringstream ss;
	boost::xtime xt; 
	while (!th_stoped){
		// le premier traitement : Attendre jusqu'à la fin du cycle
		//Début du cycle
		boost::xtime_get(&xt, boost::TIME_UTC); 
        xt.sec += 1 * timer;
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
		//writeLineInLogFile(ss.str());
	} // Fin du clock
}
void ClockThread::getFileList(){
    Configuration * conf = Configuration::get();
	boost::filesystem::directory_iterator end_itr; 
	// Default ctor yields past-the-end 
	fileList.clear();
    for( boost::filesystem::directory_iterator i( conf->get_string("path") ); i != end_itr; i++ ) {
		// Skip if not a file
		if( !boost::filesystem::is_regular_file( i->status() ) ) 
			continue;
		boost::smatch what;
		// Skip if no match     
		if( !boost::regex_match( i->filename(), what, gs_statFileFilter ) ) 
			continue;      
		// File matches, store it
        if (i->leaf() != conf->get_string("application")){
			fileList.push_back( i->filename() ); 
		}
	} 
}
void ClockThread::saveStatFromFileList(){

	BOOST_FOREACH(std::string & fileName, this->fileList) {
		this->saveStatFromFile(fileName);
	}
}
void ClockThread::deleteStatFile(const std::string & fileName){
    std::string path = Configuration::get()->get_string("path");
    if(boost::filesystem::exists(path+fileName)){
       // boost::filesystem::remove(path+fileName);
		std::stringstream ss;
		ss<<fileName;
        log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
        LOG4CPLUS_WARN(logger, "Fichier de stat supprimé : " + ss.str());
	}
}
void ClockThread::renameStatFile(const std::string & fileName){
    std::string path = Configuration::get()->get_string("path");
    if(boost::filesystem::exists(path+fileName)){
        boost::filesystem::rename(path+fileName, path + (boost::format("ERROR_%1%") % fileName).str());
	}
}
void ClockThread::saveStatFromFile(const std::string & fileName){
	std::string lineSql; 
	std::stringstream ss;
    std::ifstream file(Configuration::get()->get_string("path")+fileName);
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
