#pragma once
#include <string>
#include <vector>
#include "../type/type.h"
#include <rapidxml.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/thread.hpp>
#include "boost/regex.hpp"
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <istream>


const static std::string ks_header_xml = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n";
const static int ki_hoursPerDay   = 24;
const static int ki_minsPerHour   = 60;
const static int ki_secsPerMin    = 60;
const static int ki_mSecsPerSec   = 1000;
const static int ki_minsPerDay    = ki_hoursPerDay * ki_minsPerHour;
const static int ki_secsPerDay    = ki_minsPerDay * ki_secsPerMin;
const static int ki_mSecsPerDay   = ki_secsPerDay * ki_mSecsPerSec;

// Detail d'un itinéraire
struct DetailPlanJourney{
	int user_id;
    int wsn_id;
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

        ///Constructeur par défaut:
        DetailPlanJourney();

        void readXML(rapidxml::xml_node<> *Node);
        std::string writeXML() const;
        std::string getSql() const;
};

struct ResponsePlanJourney{
    static const std::string xml_node_name;
	int user_id;
    int wsn_id;
    int response_ide;
	int interchange;
	double total_link_time;
	int totalLink_hour;
	int totalLink_minute;
	double journey_duration;
	int journeyDuration_hour;
	int journeyDuration_minute;
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
        void readXML(rapidxml::xml_node<> *Node);
        std::string writeXML() const;
        std::string getSql() const;
		void writeTotalLinkTime();
		void writeJourneyDuration();
};

struct PlanJourney{
	int user_id;
	int wsn_id;
	std::string server_info;
	std::string script_info;
	boost::posix_time::ptime plan_dateTime;
	boost::posix_time::ptime call_dateTime;
	int depType_value;
	std::string dep_external_code;
	std::string dep_city_External_code;
	GeographicalCoord dep_coord;
    PointType depType;
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
        std::string writeXML() const;
        std::string getSql() const;
};


struct Hit{
	
	boost::posix_time::ptime dateTime;
	//Durée en milliseconds 
    int wsn_id;
    int user_id;
    int response_duration;
    std::string website;
	std::string server_ip;
	std::string client_ip;
	std::string client_login;
	std::string script_info;
	std::string action;
	int response_size;
	double api_cost;

        Hit();

        void readXML(rapidxml::xml_node<> *Node);
        std::string writeXML() const;
        std::string getSql() const;
};
struct StatNavitia{
    std::string sql_requete;
	int wsn_id;
	int user_id;
	Hit hit;
	PlanJourney planJourney;

        StatNavitia();

        std::string readXML(const std::string & reponse_navitia);
        std::string writeXML() const;
        void writeSql() const;
        void writeSQLInFile(const std::string & request) const;
};
// gestion du clock
struct ClockThread{
    boost::regex statFileFilter;
    bool th_stoped;
    boost::shared_ptr<boost::thread> m_thread;
	std::vector<std::string> fileList;
	int hit_call_count;

	ClockThread();
	void start();
	void stop();
	void work();
	void createNewFileName();
	void getFileList();
	void saveStatFromFileList();
    void saveStatFromFile(const std::string & fileName);
    void deleteStatFile(const std::string & fileName);
    void renameStatFile(const std::string & fileName);
	};

struct User{
	int wsn_id;
	int user_id;
	std::string user_ip;
	std::string user_login;
	User();
    User(int wsnid, int userid, const std::string &userip, const std::string &userlogin);
};

struct Cost{
	int api_id;
	std::string api_code;
	double api_cost;
	Cost();
    Cost(int apiid, const std::string & apicode, double apicost);
};

struct Manage_user{
	
    /// Structure contenant l'ensemble des utilisateurs
    std::vector<User> users;

	///Constructeur par défaut
	Manage_user();
	//void add(const int wsnid, const int userid, const std::string &userip, const std::string &userlogin);
    void add(const User & user);
    void fill_user_list(int wsnid);
	int grant_access(const std::string &request);
	int getUserIdByLogin(const std::string &login);

};

struct Manage_cost{
	///Structure contenant l'ensemble des coûts
	std::vector<Cost> costs;
	Manage_cost();
    void add(const Cost &cost);
	void fill_cost_list();
	double getCostByApi(const std::string & apiCode);
};





// fonction à deplacer
std::string format_double(double value, int precision = 2);
int str_to_int_def(std::string value,int default_value = -1);
double str_to_float_def(std::string value,double default_value = 0.00);

void writeLineInLogFile(const std::string & strline);
std::string formatDateTime(boost::posix_time::ptime pt);
std::string getApplicationPath();
std::string getStringByRequest(const std::string &request, const std::string &params, const std::string &separator = "&");
