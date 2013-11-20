#include "adjustit_connector.h"

#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/algorithm/string.hpp>
#include <QtCore/qvariant.h>
#include <QtCore/qlist.h>
#include <QtSql/qsqlerror.h>
#include <QtSql/qsqlrecord.h>
#include <boost/format.hpp>
#include "utils/exception.h"
#include "utils/logger.h"


namespace pt = boost::posix_time;
namespace ba = boost::algorithm;
namespace nt = navitia::type;
namespace po = boost::program_options;

namespace ed{ namespace connectors{

QSqlDatabase AtLoader::connect(const Config& params){

    QSqlDatabase at_db;
    if(QSqlDatabase::contains("at")){
        at_db = QSqlDatabase::database("at", true);
    }else{
        at_db = QSqlDatabase::addDatabase("QODBC", "at");
        at_db.setDatabaseName(QString::fromStdString(params.connect_string));
    }

    if(!at_db.open()){
        throw navitia::exception(at_db.lastError().text().toStdString());
    }
    return at_db;
}

navitia::type::Type_e parse_object_type(std::string object_type){
    if(ba::iequals(object_type, "StopArea")){
        return navitia::type::Type_e::StopArea;
    }else if(ba::iequals(object_type, "StopPoint")){
        return navitia::type::Type_e::StopPoint;
    }else if(ba::iequals(object_type, "Line")){
        return navitia::type::Type_e::Line;
    }else if(ba::iequals(object_type, "VehicleJourney")){
        return navitia::type::Type_e::VehicleJourney;
    }else if(ba::iequals(object_type, "route")){
        return navitia::type::Type_e::Route;
    }else if(ba::iequals(object_type, "Network")){
        return navitia::type::Type_e::Network;
    }else if(ba::iequals(object_type, "RoutePoint")){
        return navitia::type::Type_e::JourneyPatternPoint;
    }else if(ba::iequals(object_type, "Company")){
        return navitia::type::Type_e::Company;
    }else{
        throw navitia::exception((boost::format("object_type unknow: %s") % object_type).str());
    }

}

std::map<std::string, int> AtLoader::get_mapping(const QSqlRecord& record){
    std::map<std::string, int> result;
    for(int i=0; i<record.count(); ++i){
        result[record.fieldName(i).toStdString()] = i;
    }
    return result;

}

std::map<std::string, std::vector<navitia::type::Message>> AtLoader::load(const Config& params, const boost::posix_time::ptime& now){
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
    std::map<std::string, std::vector<navitia::type::Message>> messages;
    QSqlQuery result = find_all(params, now);
    auto field_mapping = get_mapping(result.record());
    while(result.next()){
        try{
            nt::Message m = parse_message(result, field_mapping);
            messages[m.object_uri].push_back(m);
        }catch(const std::exception& e){
            LOG4CPLUS_WARN(logger, e.what());
        }
    }
    return messages;
}


std::map<std::string, std::vector<navitia::type::Message>> AtLoader::load_disrupt(const Config& params, const boost::posix_time::ptime& now){
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
    std::map<std::string, std::vector<navitia::type::Message>> messages;
    QSqlQuery result = find_disrupt(params, now);
    auto field_mapping = get_mapping(result.record());
    while(result.next()){
        try{
            nt::Message m = parse_message(result, field_mapping);
            messages[m.object_uri].push_back(m);
        }catch(const std::exception& e){
            LOG4CPLUS_WARN(logger, e.what());
        }
    }
    return messages;
}


QSqlQuery AtLoader::find_disrupt(const Config& params, const boost::posix_time::ptime& now){
    QSqlQuery requester(this->connect(params));
    requester.setForwardOnly(true);
    bool result = requester.prepare("SELECT "
            "event.event_id AS event_id, " //0
            "impact.impact_id AS impact_id, " //1
            "convert(varchar, event.event_publicationstartdate, 121) AS  publication_start_date, "//2
            "convert(varchar, event.event_publicationenddate, 121) AS  publication_end_date, "//3
            "convert(varchar, impact.impact_eventstartdate, 121) AS application_start_date, "//4
            "convert(varchar, impact.impact_eventenddate, 121) AS application_end_date, "//5
            "convert(varchar, impact.impact_dailystartdate, 108) AS application_daily_start_hour, "//6
            "convert(varchar, impact.impact_dailyenddate, 108) AS application_daily_end_hour, "//7
            "impact.impact_activedays AS active_days, "//8
            "tcobjectref.tcobjectcodeext AS object_external_code, "//9
            "tcobjectref.tcobjecttype AS object_type "//10
            //"impactBroadcast.impact_title AS title, "//11
            //"ImpactBroadcast.impact_msg AS message "//12
        "FROM "
            "event "
            "INNER JOIN impact ON (event.event_id = impact.event_id) "
            "INNER JOIN tcobjectref ON (impact.tcobjectref_id = tcobjectref.tcobjectref_id) "
        "WHERE "
            "event.event_publicationenddate >= CONVERT(DATETIME, :date_pub, 126) "
            "AND (event.event_closedate IS NULL OR event.event_closedate > CONVERT(DATETIME, :date_clo, 126)) "
            "AND impact_state = 'disrupt'"
    );

    if(!result){
        throw navitia::exception("Erreur lors de la préparation de la requéte : " + requester.lastError().text().toStdString());
    }
    requester.bindValue(":date_pub", QVariant(QString::fromStdString(pt::to_iso_extended_string(now))));
    requester.bindValue(":date_clo", QVariant(QString::fromStdString(pt::to_iso_extended_string(now))));

    result = requester.exec();
    if(!result){
        throw navitia::exception("Erreur lors de l'execution de la requéte " + requester.lastError().text().toStdString());
    }

    return requester;
}

QSqlQuery AtLoader::find_all(const Config& params, const pt::ptime& now){

    QSqlQuery requester(this->connect(params));
    requester.setForwardOnly(true);
    bool result = requester.prepare("SELECT "
            "event.event_id AS event_id, " //0
            "impact.impact_id AS impact_id, " //1
            "convert(varchar, event.event_publicationstartdate, 121) AS  publication_start_date, "//2
            "convert(varchar, event.event_publicationenddate, 121) AS  publication_end_date, "//3
            "convert(varchar, impact.impact_eventstartdate, 121) AS application_start_date, "//4
            "convert(varchar, impact.impact_eventenddate, 121) AS application_end_date, "//5
            "convert(varchar, impact.impact_dailystartdate, 108) AS application_daily_start_hour, "//6
            "convert(varchar, impact.impact_dailyenddate, 108) AS application_daily_end_hour, "//7
            "impact.impact_activedays AS active_days, "//8
            "tcobjectref.tcobjectcodeext AS object_external_code, "//9
            "tcobjectref.tcobjecttype AS object_type, "//10
            "impactBroadcast.impact_title AS title, "//11
            "ImpactBroadcast.impact_msg AS message "//12
        "FROM "
            "event "
            "INNER JOIN impact ON (event.event_id = impact.event_id) "
            "INNER JOIN tcobjectref ON (impact.tcobjectref_id = tcobjectref.tcobjectref_id) "
            "INNER JOIN impactbroadcast ON (impactbroadcast.Impact_ID = impact.Impact_ID) "
            "INNER JOIN msgmedia ON (impactbroadcast.msgmedia_id = msgmedia.msgmedia_id) "
        "WHERE "
            "event.event_publicationenddate >= CONVERT(DATETIME, :date_pub, 126) "
            "AND (event.event_closedate IS NULL OR event.event_closedate > CONVERT(DATETIME, :date_clo, 126)) "
            "AND msgmedia.msgmedia_lang = :media_lang "
            "AND msgmedia.msgmedia_media = :media_media"
    );

    if(!result){
        throw navitia::exception("Erreur lors de la préparation de la requéte : " + requester.lastError().text().toStdString());
    }
    requester.bindValue(":date_pub", QVariant(QString::fromStdString(pt::to_iso_extended_string(now))));
    requester.bindValue(":date_clo", QVariant(QString::fromStdString(pt::to_iso_extended_string(now))));
    requester.bindValue(":media_lang", QVariant(QString::fromStdString(params.media_lang)));
    requester.bindValue(":media_media", QVariant(QString::fromStdString(params.media_media)));

    result = requester.exec();
    if(!result){
        throw navitia::exception("Erreur lors de l'execution de la requéte " + requester.lastError().text().toStdString());
    }

    return requester;
}

std::string get_string_field(const std::string& field, const QSqlQuery& requester, const std::map<std::string, int>& mapping,
        const std::string& default_value){
    auto it = mapping.find(field);
    if(it == mapping.end() || requester.isNull(it->second)){
        return default_value;
    }
    return requester.value(it->second).toString().toStdString();
}

int get_int_field(const std::string& field, const QSqlQuery& requester, const std::map<std::string, int>& mapping,
        int default_value){
    auto it = mapping.find(field);
    if(it == mapping.end() || requester.isNull(it->second)){
        return default_value;
    }
    return requester.value(it->second).toInt();
}

navitia::type::Message AtLoader::parse_message(const QSqlQuery& requester, const std::map<std::string, int>& mapping){
    navitia::type::Message message;

    int event_id = get_int_field("event_id", requester, mapping);
    int impact_id = get_int_field("impact_id", requester, mapping);

    message.uri = (boost::format("message:%d:%d") % event_id % impact_id).str();

    std::string begin = get_string_field("publication_start_date", requester, mapping);
    std::string end = get_string_field("publication_end_date", requester, mapping);


    if(begin != "" && end != ""){
        pt::ptime start_date = pt::time_from_string(begin);
        pt::ptime end_date = pt::time_from_string(end);

        message.publication_period = pt::time_period(start_date, end_date);
    }


    begin = get_string_field("application_start_date", requester, mapping);
    end = get_string_field("application_end_date", requester, mapping);

    if(begin != "" && end != ""){
        pt::ptime start_date = pt::time_from_string(begin);
        pt::ptime end_date = pt::time_from_string(end);

        message.application_period = pt::time_period(start_date, end_date);
    }

    begin = get_string_field("application_daily_start_hour", requester, mapping);
    if(begin != "")
        message.application_daily_start_hour = pt::duration_from_string(begin);


    end = get_string_field("application_daily_end_hour", requester, mapping);
    if(end != "")
        message.application_daily_end_hour = pt::duration_from_string(end);

    message.active_days = get_int_field("active_days", requester, mapping, 127);

    message.object_uri = get_string_field("object_external_code", requester, mapping);

    message.object_type = parse_object_type(get_string_field("object_type", requester, mapping));

    message.title = get_string_field("title", requester, mapping);

    message.message = get_string_field("message", requester, mapping);

    return message;
}

}}//namespace
