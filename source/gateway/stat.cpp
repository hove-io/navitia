#include "stat.h"
#include <rapidxml.hpp>


DetailPlanJourney::DetailPlanJourney():response_ide(-1), dep_external_code(""),
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
        if (strcmp(attrName.c_str(), "DepExternalCode") != 0){
            this->dep_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "DepCoordX") != 0){
            this->dep_coord.X = atoi(attr->value());
        }
        else if (strcmp(attrName.c_str(), "DepCoordY") != 0){
            this->dep_coord.Y = atoi(attr->value());
        }
        else if (strcmp(attrName.c_str(), "LineExternalCode") != 0){
            this->line_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "ModeExternalCode") != 0){
            this->mode_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "CompanyExternalCode") != 0){
            this->company_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "NetworkExternalCode") != 0){
            this->network_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "RouteExternalCode") != 0){
            this->route_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "ArrExternalCode") != 0){
            this->arr_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "ArrCoordX") != 0){
            this->arr_coord.X = atoi(attr->value());
        }
        else if (strcmp(attrName.c_str(), "ArrCoordY") != 0){
            this->arr_coord.Y = atoi(attr->value());
        }
        else if (strcmp(attrName.c_str(), "DepDateTime") != 0){
            strDepDateTime = attr->value();
        }
        else if (strcmp(attrName.c_str(), "ArrDateTime") != 0){
            strArrDateTime = attr->value();
        }
        else if (strcmp(attrName.c_str(), "SectionType") != 0){
            this->section_type = atoi(attr->value());
        }
    }

};

std::string DetailPlanJourney::writeXML(){
    std::string result = "toto";
    return result;
};


ResponsePlanJourney::ResponsePlanJourney():response_ide(-1), interchange(0),
    total_link_time(boost::posix_time::second_clock::local_time()),
    journey_dateTime(boost::posix_time::second_clock::local_time()),
    isBest(false), isFirst(false), isLast(false),
    detail_index(-1), detail_count(0), comment_type(0){
};

void ResponsePlanJourney::addDetail(const std::string reponse_navitia){

};

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
        if (strcmp(attrName.c_str(), "Interchange") != 0){
            this->interchange = atoi(attr->value());
        }
        else if (strcmp(attrName.c_str(), "TotalLinkTime") != 0){
            strTotalLinkTime = attr->value();
        }
        else if (strcmp(attrName.c_str(), "JourneyDuration") != 0){
            strJourneyDuration = attr->value();
        }
        else if (strcmp(attrName.c_str(), "IsFirst") != 0){
            strIsFirst = attr->value();
        }
        else if (strcmp(attrName.c_str(), "IsBest") != 0){
            strIsBest = attr->value();
        }
        else if (strcmp(attrName.c_str(), "IsLast") != 0){
            strIsLast = attr->value();
        }
        else if (strcmp(attrName.c_str(), "JourneyDateTime") != 0){
            strJourneyDateTime = attr->value();
        }
        else if (strcmp(attrName.c_str(), "CommentType") != 0){
            this->comment_type = atoi(attr->value());
        }
    }

    //Lire le noeud DetailPlanJourney
    for (rapidxml::xml_node<> *detailNode = Node->first_node();
            detailNode; detailNode = detailNode->next_sibling()){
        strNodeName = detailNode->name();

        if (strcmp(strNodeName.c_str(), "DetailPlanJourney") != 0){
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

PlanJourney::PlanJourney(): server_info(""), script_info(""),
                            plan_dateTime(boost::posix_time::second_clock::local_time()),
                            call_dateTime(boost::posix_time::second_clock::local_time()),
                            dep_external_code(""), dep_city_External_code(""),
                            dest_external_code(""), dest_city_External_code(""), sens(0),
                            mode(0), mode_string(""), vitesse_map(0), equipement(0), equipement_string(""),
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
        if (strcmp(attrName.c_str(), "RequestDate") != 0){
            strRequestDate = attr->value();
        }
        else if (strcmp(attrName.c_str(), "Server") != 0){
            this->server_info = attr->value();
        }
        else if (strcmp(attrName.c_str(), "PlanDateTime") != 0){
            strPlanDateTime = attr->value();
        }
        else if (strcmp(attrName.c_str(), "DepPointType") != 0){
            strDepPointType = attr->value();
        }
        else if (strcmp(attrName.c_str(), "DepPointExternalCode") != 0){
            this->dep_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "DepCityExternalCode") != 0){
            this->dep_city_External_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "DepCoordX") != 0){
            this->dep_coord.X  = atoi(attr->value());
        }
        else if (strcmp(attrName.c_str(), "DepCoordY") != 0){
            this->dep_coord.Y = atoi(attr->value());
        }
        else if (strcmp(attrName.c_str(), "DestPointType") != 0){
            strDestPointType = attr->value();
        }
        else if (strcmp(attrName.c_str(), "DestPointExternalCode") != 0){
            this->dest_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "DestCityExternalCode") != 0){
            this->dest_city_External_code = atoi(attr->value());
        }
        else if (strcmp(attrName.c_str(), "DestCoordX") != 0){
            this->dest_coord.X = atoi(attr->value());
        }
        else if (strcmp(attrName.c_str(), "DestCoordY") != 0){
            this->dest_coord.Y = atoi(attr->value());
        }
        else if (strcmp(attrName.c_str(), "Sens") != 0){
            this->sens = atoi(attr->value());
        }
        else if (strcmp(attrName.c_str(), "Criteria") != 0){
            intCriteria = atoi(attr->value());
        }
        else if (strcmp(attrName.c_str(), "Mode") != 0){
            this->mode_string = attr->value();
        }
        else if (strcmp(attrName.c_str(), "WalkSpeed") != 0){
            this->vitesse_map = atoi(attr->value());
        }
        else if (strcmp(attrName.c_str(), "Equipment") != 0){
            this->equipement_string = attr->value();
        }
        else if (strcmp(attrName.c_str(), "Vehicle") != 0){
            this->vehicle_string = attr->value();
        }
        else if (strcmp(attrName.c_str(), "CumulCalcDuration") != 0){
            strCumulCalcDuration = attr->value();
        }
        else if (strcmp(attrName.c_str(), "HangDistance") != 0){
            this->hang_distance = atoi(attr->value());
        }
        else if (strcmp(attrName.c_str(), "HangDistanceDep") != 0){
            this->dep_hang_distance= atoi(attr->value());
        }
        else if (strcmp(attrName.c_str(), "HangDistanceArr") != 0){
            this->dest_hang_distance = atoi(attr->value());
        }
        else if (strcmp(attrName.c_str(), "ViaExternalCode") != 0){
            this->via_external_code = atoi(attr->value());
        }
        else if (strcmp(attrName.c_str(), "ViaConnectionDuration") != 0){
            strViaConnectionDuration = attr->value();
        }
        else if (strcmp(attrName.c_str(), "ManageDisrupt") != 0){
            this->manage_disrupt = atoi(attr->value());
        }
        else if (strcmp(attrName.c_str(), "ForbiddenStopAreaExtCode") != 0){
            this->forbidden_SA_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "ForbiddenLineExtCode") != 0){
            this->forbidden_line_external_code = attr->value();
        }
        else if (strcmp(attrName.c_str(), "error") != 0){
            this->error = atoi(attr->value());
        }
    }

    //Lire le noeud ResponsePlanJourney
    for (rapidxml::xml_node<> *responseNode = Node->first_node();
            responseNode; responseNode = responseNode->next_sibling()){
        strNodeName = responseNode->name();

        if (strcmp(strNodeName.c_str(), "ResponsePlanJourney") != 0){
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

Hit::Hit(): dateTime(boost::posix_time::second_clock::local_time()),
            website(""), server_ip(""), client_ip(""),user_id(0),
            client_login(""), script_info(""), action(""), response_size(0), wsn_id(-1), api_cost(0){

};

void Hit::readXML(rapidxml::xml_node<> *Node){
    std::string attrName = "";
    std::string strDay = "";
    std::string strMonth = "";
    std::string strTime = "";

    for(rapidxml::xml_attribute<> * attr = Node->first_attribute(); attr; attr = attr->next_attribute()){

        attrName = attr->name();
        if (strcmp(attrName.c_str(), "Date") != 0){
        strDay = attr->value();
        }
        else if (strcmp(attrName.c_str(), "Month") != 0){
            strMonth = attr->value();
        }
        else if (strcmp(attrName.c_str(), "Time") != 0){
            strTime = attr->value();
        }
        else if (strcmp(attrName.c_str(), "Action") != 0){
            this->action = attr->value();
        }
        else if (strcmp(attrName.c_str(), "ResponseSize") != 0){
            this->response_size = atoi(attr->value());
        }
        else if (strcmp(attrName.c_str(), "Ide") != 0){
            this->user_id = atoi(attr->value());
        }
        else if (strcmp(attrName.c_str(), "WsnId") != 0){
            this->wsn_id = atoi(attr->value());
        }
    }
};

std::string Hit::writeXML(){
    std::string result = "toto";
    return result;
};

StatNavitia::StatNavitia(){
};

void StatNavitia::readXML(const std::string reponse_navitia){

    //Utilisation de RapidXML pour parser le flux XML de HIT
    rapidxml::xml_document<> xmlDoc;
    rapidxml::xml_node<> * HitNode = NULL;
    std::string nodeName = "";
    char * data_ptr = xmlDoc.allocate_string(reponse_navitia.c_str());

    xmlDoc.parse<0>(data_ptr);

    HitNode = xmlDoc.first_node("Hit");
    if (HitNode){
        //Appeler la méthode readXML de hit en passant le neoud HitNode;
        this->hit.readXML(HitNode);

        for (rapidxml::xml_node<> * PlanNode = HitNode->first_node(); PlanNode; PlanNode->next_sibling()){
            nodeName = PlanNode->name();

            //si c'est le node "PlanJourney" alors appeler la méthode readXML de Planjourney en passant
            //le neoud PlanNode;
            if (nodeName=="PlanJourney"){
                this->planJourney.readXML(PlanNode);
            }
        }
    }
};

std::string StatNavitia::writeXML(){
    std::string result = "toto";
    return result;
};


