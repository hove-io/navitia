#pragma once
#include <string>
#include <vector>
#include "boost\date_time\posix_time\posix_time.hpp"
#include "../type/type.h"
#include <rapidxml.hpp>

const static std::string ks_hit_start = "<HIT";
const static std::string ks_plan_journey_start = "<PlanJourney";
const static std::string ks_response_plan_journey_start = "<ResponsePlanJourney";
const static std::string ks_detail_plan_journey_start = "<DetailPlanJourney";
const static std::string ks_hit_end = "</HIT";
const static std::string ks_plan_journey_end = "</PlanJourney";
const static std::string ks_response_plan_journey_end = "</ResponsePlanJourney";
const static std::string ks_detail_plan_journey_end = "/>";
const static std::string ks_closing_tag = ">";

//constantes pour les attributs communs 
const static std::string ks_attribute_quote = "\"";
const static std::string ks_param_ide = " Ide=";
const static std::string ks_param_server = " Server=";
const static std::string ks_param_dep_coordX = " DepCoordX=";
const static std::string ks_param_dep_coordY = " DepCoordY=";

//attributs de Hit
const static std::string ks_param_date=" Date=";
const static std::string ks_param_month=" Month=";
const static std::string ks_param_time=" Time=";
const static std::string ks_param_action=" Action=";
const static std::string ks_param_durarion=" Duration=";
const static std::string ks_param_response_size=" ResponseSize=";
const static std::string ks_param_script=" Script=";
const static std::string ks_param_user=" User=";
const static std::string ks_param_wsn_id=" WsnId=";

//attributs de PlanJourney
const static std::string ks_param_requestDate=" RequestDate=";
const static std::string ks_param_planDateTime=" PlanDateTime=";
const static std::string ks_param_depPointType=" DepPointType=";
const static std::string ks_param_depPointExternalCode=" DepPointExternalCode=";
const static std::string ks_param_depCityExternalCode=" DepCityExternalCode=";
const static std::string ks_param_destPointType=" DestPointType=";
const static std::string ks_param_destPointExternalCode=" DestPointExternalCode=";
const static std::string ks_param_destCityExternalCode=" DestCityExternalCode=";
const static std::string ks_param_destCoordX=" DestCoordX=";
const static std::string ks_param_destCoordY=" DestCoordY=";
const static std::string ks_param_sens=" Sens=";
const static std::string ks_param_criteria=" Criteria=";
const static std::string ks_param_mode=" Mode=";
const static std::string ks_param_walkSpeed=" WalkSpeed=";
const static std::string ks_param_equipment=" Equipment=";
const static std::string ks_param_vehicle=" Vehicle=";
const static std::string ks_param_cumulCalcDuration=" CumulCalcDuration=";
const static std::string ks_param_error=" error=";
const static std::string ks_param_hangDistance=" HangDistance=";
const static std::string ks_param_depHangDistance=" HangDistanceDep=";
const static std::string ks_param_destHangDistance=" HangDistanceArr=";
const static std::string ks_param_viaExternalCode=" ViaExternalCode=";
const static std::string ks_param_viaConnectionDuration=" ViaConnectionDuration=";
const static std::string ks_param_manageDisrupt=" ManageDisrupt=";
const static std::string ks_param_forbiddenSAExternalCode=" ForbiddenStopAreaExtCode=";
const static std::string ks_param_forbiddenLineExternalCode=" ForbiddenLineExtCode=";

//attributs de ResponsePlanJourney
const static std::string ks_param_interchange=" Interchange=";
const static std::string ks_param_totalLinkTime=" TotalLinkTime=";
const static std::string ks_param_journeyDuration=" JourneyDuration=";
const static std::string ks_param_isFirst=" IsFirst=";
const static std::string ks_param_isBest=" IsBest=";
const static std::string ks_param_isLast=" IsLast=";
const static std::string ks_param_journeyDateTime=" JourneyDateTime=";
const static std::string ks_param_trancheShift=" TrancheShift=";
const static std::string ks_param_trancheDuration=" TrancheDuration=";
const static std::string ks_param_commentType=" CommentType=";

//attributs de DetailPlanJourney
const static std::string ks_param_depExternalCode=" DepExternalCode=";
const static std::string ks_param_lineExternalCode=" LineExternalCode=";
const static std::string ks_param_modeExternalCode=" ModeExternalCode=";
const static std::string ks_param_companyExternalCode=" CompanyExternalCode=";
const static std::string ks_param_networkExternalCode=" NetworkExternalCode=";
const static std::string ks_param_routeExternalCode=" RouteExternalCode=";
const static std::string ks_param_arrExternalCode=" ArrExternalCode=";
const static std::string ks_param_arrCoordX=" ArrCoordX=";
const static std::string ks_param_arrCoordY=" ArrCoordY=";
const static std::string ks_param_depDateTime=" DepDateTime=";
const static std::string ks_param_arrDateTime=" ArrDateTime=";
const static std::string ks_param_sectionType=" SectionType=";


// Detail d'un itinéraire
struct DetailPlanJourney{
	int response_ide;
	PointType depType;
	std::string dep_external_code;
	GeographicalCoord dep_coord;
	std::string line_external_code;
	std::string mode_external_code;
	std::string company_external_code;
	std::string network_external_code;
	std::string route_external_code;
	PointType arrType;
	std::string arr_external_code;
        GeographicalCoord arr_coord;
        boost::posix_time::ptime dep_dateTime;
        boost::posix_time::ptime arr_dateTime;
	int section_type;

        ///Constucteur par défaut:
        DetailPlanJourney();
        void readXML(rapidxml::xml_node<> *Node);
        std::string writeXML();
};

struct ResponsePlanJourney{
	int response_ide;
	int interchange;
	boost::posix_time::ptime total_link_time;
	boost::posix_time::ptime journey_duration;
	boost::posix_time::ptime journey_dateTime;
	bool isBest;
	bool isFirst;
	bool isLast;
	int detail_index;
	int detail_count;
	int comment_type;

        /// Structure contenant l'ensemble des détails
        std::vector<DetailPlanJourney> details;

        ResponsePlanJourney();
        void add(DetailPlanJourney & detail);
        void addDetail(const std::string reponse_navitia);
        void readXML(rapidxml::xml_node<> *Node);
        std::string writeXML();
};

struct PlanJourney{
	std::string server_info;
	std::string script_info;
	boost::posix_time::ptime plan_dateTime;
	boost::posix_time::ptime call_dateTime;
	PointType depType;
	std::string dep_external_code;
	std::string dep_city_External_code;
	GeographicalCoord dep_coord;
	PointType destType;
	std::string dest_external_code;
	std::string dest_city_External_code;
	GeographicalCoord dest_coord;
	int sens;
	Criteria criteria;
	int mode;
	std::string mode_string;
	int vitesse_map;
	int equipement;
	std::string equipement_string;
	int vehicle;
	std::string vehicle_string;
	boost::posix_time::ptime total_plan_duration;
	int error;
	int hang_distance;
	int dep_hang_distance;
	int dest_hang_distance;
	std::string via_external_code;
	int via_connection_duration;
	bool manage_disrupt;
	std::string forbidden_SA_external_code;
	std::string forbidden_line_external_code;

        /// Structure contenant l'ensemble des réponses
        std::vector<ResponsePlanJourney> responses;

        PlanJourney();
	
        void add(ResponsePlanJourney & response);
        void readXML(rapidxml::xml_node<> *Node);
        std::string writeXML();
};


struct Hit{
	boost::posix_time::ptime dateTime;
	boost::posix_time::ptime response_duration;
        std::string website;
	std::string server_ip;
	std::string client_ip;
	int user_id;
	std::string client_login;
	std::string script_info;
	std::string action;
	int response_size;
	int wsn_id;
	double api_cost;

        Hit();

        void readXML(rapidxml::xml_node<> *Node);
        std::string writeXML();
};
struct StatNavitia{
	Hit hit;
	PlanJourney planJourney;

        StatNavitia();

        void readXML(const std::string reponse_navitia);
        std::string writeXML();
};

