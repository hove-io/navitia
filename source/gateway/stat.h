#pragma once
#include <string>
#include <vector>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "..\type\type.h"
#include <rapidxml.hpp>
#include <boost\format.hpp>
#include <boost/foreach.hpp>


// formatage des dates à deplacer
const std::locale formats[] = { std::locale(std::locale::classic(),
		new boost::posix_time::time_input_facet("%Y-%m-%d %H:%M:%S")), std::locale(std::locale::classic(),
		new boost::posix_time::time_input_facet("%Y/%m/%d %H:%M:%S")), std::locale(std::locale::classic(),
		new boost::posix_time::time_input_facet("%d.%m.%Y %H:%M:%S")), std::locale(std::locale::classic(),
		new boost::posix_time::time_input_facet("%Y-%m-%d"))}; 
const size_t formats_n = sizeof(formats)/sizeof(formats[0]);

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
const static std::string ks_param_read_ide = "Ide";
const static std::string ks_param_write_ide = " " + ks_param_read_ide + "=";
const static std::string ks_param_read_server = "Server";
const static std::string ks_param_write_server = " " + ks_param_read_server + "=";
const static std::string ks_param_read_dep_coordX = "DepCoordX";
const static std::string ks_param_write_dep_coordX = " " + ks_param_read_dep_coordX + "=";
const static std::string ks_param_read_dep_coordY = "DepCoordY";
const static std::string ks_param_write_dep_coordY = " " + ks_param_read_dep_coordY + "=";

//attributs de Hit
const static std::string ks_param_read_date="Date";
const static std::string ks_param_write_date= " " + ks_param_read_date + "=";
const static std::string ks_param_read_month="Month";
const static std::string ks_param_write_month= " " + ks_param_read_month + "=";
const static std::string ks_param_read_time="Time";
const static std::string ks_param_write_time=" " + ks_param_read_time + "=";
const static std::string ks_param_read_action="Action";
const static std::string ks_param_write_action=" "+ ks_param_read_action + "=";
const static std::string ks_param_read_durarion="Duration";
const static std::string ks_param_write_durarion=" " + ks_param_read_durarion + "=";
const static std::string ks_param_read_response_size="ResponseSize";
const static std::string ks_param_write_response_size=" " + ks_param_read_response_size + "=";
const static std::string ks_param_read_script="Script";
const static std::string ks_param_write_script= " " + ks_param_read_script + "=";
const static std::string ks_param_read_user="User";
const static std::string ks_param_write_user=" " + ks_param_read_user + "=";
const static std::string ks_param_read_wsn_id="WsnId";
const static std::string ks_param_write_wsn_id=" " + ks_param_read_wsn_id + "=";

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
const static std::string ks_paramR_interchange="Interchange";
const static std::string ks_paramW_interchange= " " + ks_paramR_interchange + "=";
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

// Ecriture du fichier des requêtes SQL
const static std::string ks_table_hit_insert = "insert into HIT_TEMP (HIT_DATE, HIT_YEAR, HIT_MONTH,HIT_DAY,HIT_HOUR,HIT_MINUTE, HIT_TIME, HIT_SERVER, USE_ID, HIT_ACTION, HIT_DURATION, HIT_RESPONSE_SIZE, HIT_SCRIPT, WSN_ID, HIT_COST, HIT_CLIENT_IP) values (";
const static std::string ks_table_hit_values = "'%1%', %2%, %3%, %4%, %5%, %6%, %7%, '%8%', %9%, '%10%', %11%, %12%, '%13%', %14%, %15%, '%16%'";

const static std::string ks_table_planjourney_insert = "insert into PJO_TEMP (PJO_REQUEST_DATE, PJO_REQUEST_YEAR, PJO_REQUEST_MONTH, PJO_REQUEST_DAY, PJO_REQUEST_HOUR, PJO_REQUEST_MINUTE, PJO_SERVER, PJO_PLAN_DATE_TIME, PJO_PLAN_YEAR, PJO_PLAN_MONTH, PJO_PLAN_DAY, PJO_PLAN_HOUR, PJO_PLAN_MINUTE, PJO_DEP_POINT_TYPE, PJO_DEP_POINT_EXTERNAL_CODE, PJO_DEP_CITY_EXTERNAL_CODE, PJO_DEP_COORD_X, PJO_DEP_COORD_Y, PJO_DEST_POINT_TYPE, PJO_DEST_POINT_EXTERNAL_CODE, PJO_DEST_CITY_EXTERNAL_CODE, PJO_DEST_COORD_X, PJO_DEST_COORD_Y, PJO_SENS, PJO_CRITERIA, PJO_MODE, PJO_WALK_SPEED, PJO_EQUIPMENT, PJO_VEHICLE, PJO_CUMUL_DUREE_CALC, PJO_HANG, PJO_DEP_HANG, PJO_DEST_HANG, PJO_VIA_EXTERNAL_CODE, PJO_VIA_CONNECTION_DURATION, PJO_MANAGE_DISRUPT,PJO_FORBIDDEN_SA_EXTERNAL_CODE, PJO_FORBIDDEN_LINE_EXTERNAL_CODE, HIT_IDE,PJO_ERROR,USE_ID,WSN_ID ) values (";
const static std::string ks_table_planjourney_values = "'%1%', %2%, %3%, %4%, %5%, %6%, '%7%', '%8%',%9%, %10%,%11%,%12%,%13%, %14%, '%15%', '%16%', %17%, %18%, %19%, '%20%',%21%, %22%, %23%, %24%, %25%,'%26%', %27%, '%28%', '%29%', %30%, %31%, %32%, %33%, '%34%', %35%, %36%, '%37%','%38%', %39%, %40%, %41%, %42%";

const static std::string ks_table_ResponsePlanJourney_insert = "insert into RPJO_TEMP (PJO_IDE, PJO_REQUEST_MONTH, RPJ_INTERCHANGE, RPJ_TOTAL_LINK_TIME, RPJ_TOTAL_LINK_TIME_HOUR, RPJ_TOTAL_LINK_TIME_MINUTE, RPJ_JOURNEY_DURATION, RPJ_JOURNEY_DURATION_HOUR, RPJ_JOURNEY_DURATION_MINUTE, RPJ_IS_FIRST, RPJ_IS_BEST, RPJ_IS_LAST, RPJ_JOURNEY_DATE_TIME, RPJ_JOURNEY_YEAR, RPJ_JOURNEY_MONTH, RPJ_JOURNEY_DAY, RPJ_JOURNEY_HOUR, RPJ_JOURNEY_MINUTE, RPJ_TRANCHE_SHIFT,RPJ_TRANCHE_DURATION,RPJ_COMMENT_TYPE,USE_ID,WSN_ID) values (";
const static std::string ks_table_ResponsePlanJourney_values = "%1%, %2%, %3%, %4%, %5%, %6%, %7%, %8%, %9%, %10%, %11%, %12%,'%13%', %14%,%15%,%16%,%17%,%18%, %19%, %20%,%21%, %22%, %23%";


// Detail d'un itinéraire
struct DetailPlanJourney{
	int response_ide;
	int user_id;
	int wsn_id;
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

        ///Constructeur par défaut:
        DetailPlanJourney();
        void readXML(rapidxml::xml_node<> *Node);
        std::string writeXML();
		std::string getSql();
};

struct ResponsePlanJourney{
	int response_ide;
	int user_id;
	int wsn_id;
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
        //void addDetail(const std::string reponse_navitia);
        void readXML(rapidxml::xml_node<> *Node);
        std::string writeXML();
		std::string getSql();
};

struct PlanJourney{
	int user_id;
	int wsn_id;
	std::string server_info;
	std::string script_info;
	boost::posix_time::ptime plan_dateTime;
	boost::posix_time::ptime call_dateTime;
	PointType depType;
	int depType_value;
	std::string dep_external_code;
	std::string dep_city_External_code;
	GeographicalCoord dep_coord;
	PointType destType;
	int destType_value;
	std::string dest_external_code;
	std::string dest_city_External_code;
	GeographicalCoord dest_coord;
	int sens;
	Criteria criteria;
	int mode;
	std::string mode_string;
	int walk_speed;
	int equipement;
	std::string equipement_string;
	int vehicle;
	std::string vehicle_string;
	int total_plan_duration;
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
		std::string getSql();
};


struct Hit{
	
	boost::posix_time::ptime dateTime;
	//Durée en milliseconds 
	int response_duration;
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
		std::string getSql();
};
struct StatNavitia{
	std::string sql_requete;
	Hit hit;
	PlanJourney planJourney;

        StatNavitia();

        void readXML(const std::string reponse_navitia);
        std::string writeXML();
		void writeSql();
};

// fonction à deplacer
std::string format_double(double value, int lenth, int precesion, double default_value);
int str_to_int_def(std::string value,int default_value = -1);
double str_to_float_def(std::string value,double default_value = 0.00);