#include "connectors/adjustit_connector.h"

#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/algorithm/string.hpp>
#include <QtCore/qvariant.h>
#include <QtCore/qlist.h>
#include <QtSql/qsqlerror.h>
#include <QtSql/qsqlrecord.h>
#include <boost/format.hpp>
#include "utils/exception.h"
#include <log4cplus/logger.h>


namespace pt = boost::posix_time;
namespace ba = boost::algorithm;
namespace nt = navitia::type;
namespace po = boost::program_options;

namespace navitia{

QSqlDatabase AtLoader::connect(const po::variables_map& params){

    QSqlDatabase at_db;
    if(QSqlDatabase::contains("at")){
        at_db = QSqlDatabase::database("at", true);
    }else{
        at_db = QSqlDatabase::addDatabase("QODBC", "at");
        at_db.setDatabaseName(QString::fromStdString(params["connect-string"].as<std::string>()));
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
    }else if(ba::iequals(object_type, "JourneyPattern")){
        return navitia::type::Type_e::JourneyPattern;
    }else if(ba::iequals(object_type, "Network")){
        return navitia::type::Type_e::Network;
    }else if(ba::iequals(object_type, "JourneyPatternPoint")){
        return navitia::type::Type_e::JourneyPatternPoint;
    }else if(ba::iequals(object_type, "Company")){
        return navitia::type::Type_e::Company;
    }else{
        throw navitia::exception((boost::format("object_type unknow: %s") % object_type).str());
    }

}

std::map<std::string, std::vector<navitia::type::Message>> AtLoader::load(const po::variables_map& params, const boost::posix_time::ptime& now){
    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
    std::map<std::string, std::vector<navitia::type::Message>> messages;
    QSqlQuery result = exec(params, now);
    while(result.next()){
        try{
            nt::Message m = parse_message(result);
            messages[m.object_uri].push_back(m);
        }catch(const std::exception& e){
            LOG4CPLUS_WARN(logger, e.what());
        }
    }
    return messages;
}


QSqlQuery AtLoader::exec(const po::variables_map& params, const pt::ptime& now){

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
    requester.bindValue(":media_lang", QVariant(QString::fromStdString(params["media-lang"].as<std::string>())));
    requester.bindValue(":media_media", QVariant(QString::fromStdString(params["media-media"].as<std::string>())));

    result = requester.exec();
    if(!result){
        throw navitia::exception("Erreur lors de l'execution de la requéte " + requester.lastError().text().toStdString());
    }

    return requester;
}

navitia::type::Message AtLoader::parse_message(const QSqlQuery& requester){
    navitia::type::Message message;

    message.uri = (boost::format("message:%d:%d") % requester.value(0).toInt() % requester.value(1).toInt()).str();

    if(!requester.isNull(2) && !requester.isNull(3)){
        pt::ptime start_date = pt::time_from_string(requester.value(2).toString().toStdString());
        pt::ptime end_date = pt::time_from_string(requester.value(3).toString().toStdString());

        message.publication_period = pt::time_period(start_date, end_date);
    }


    if(!requester.isNull(4) && !requester.isNull(5)){
        pt::ptime start_date = pt::time_from_string(requester.value(4).toString().toStdString());
        pt::ptime end_date = pt::time_from_string(requester.value(5).toString().toStdString());

        message.application_period = pt::time_period(start_date, end_date);
    }


    if(!requester.isNull(6))
        message.application_daily_start_hour = pt::duration_from_string(requester.value(6).toString().toStdString());

    if(!requester.isNull(7))
        message.application_daily_end_hour = pt::duration_from_string(requester.value(7).toString().toStdString());

    if(!requester.isNull(8)){
        message.active_days =  requester.value(8).toInt();
    }

    if(!requester.isNull(9)){
        message.object_uri = requester.value(9).toString().toStdString();
    }

    if(!requester.isNull(10)){
        message.object_type = parse_object_type(requester.value(10).toString().toStdString());
    }
    if(!requester.isNull(11)){
        message.title = requester.value(11).toString().toStdString();
    }
    if(!requester.isNull(12)){
        message.message = requester.value(12).toString().toStdString();
    }

    return message;
}

}//namespace
