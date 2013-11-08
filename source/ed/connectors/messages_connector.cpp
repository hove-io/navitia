#include "messages_connector.h"
#include <pqxx/pqxx>
#include <boost/make_shared.hpp>
#include "utils/exception.h"
#include "utils/logger.h"
#include "type/data.h"

namespace ed{ namespace connectors{

namespace pt = boost::posix_time;

std::map<std::string, boost::shared_ptr<navitia::type::Message>> load_messages(
        const RealtimeLoaderConfig& conf,
        const boost::posix_time::ptime& current_time){
    //pour le moment on vire les timezone et on considére que c'est de l'heure local
    std::string request = "SELECT id, uri, start_publication_date::timestamp, "
        "end_publication_date::timestamp, start_application_date::timestamp, "
        "end_application_date::timestamp, start_application_daily_hour::time, "
        "end_application_daily_hour::time, active_days, object_uri, "
        "object_type_id, language, body, title, message_status_id "
        "FROM realtime.message m "
        "JOIN realtime.localized_message l ON l.message_id = m.id "
        "where end_publication_date >= ($1::timestamp - $2::interval) "
        "order by id";
    //on tris par id pour les regrouper ensemble
    pqxx::result result;
    std::unique_ptr<pqxx::connection> conn;
    try{
        conn = std::unique_ptr<pqxx::connection>(new pqxx::connection(conf.connection_string));
        (*conn).prepare("messages", request)("timestamp")("INTERVAL", pqxx::prepare::treat_string);
        std::string st_current_time = boost::posix_time::to_iso_string(current_time);
        std::string st_shift_days =  std::to_string(conf.shift_days) + " days";
        pqxx::work work(*conn, "chargement des messages");
        result = work.prepared("messages")(st_current_time)(st_shift_days).exec();
    }catch(const pqxx::pqxx_exception &e){
        throw navitia::exception(e.base().what());

    }catch(...){
              throw navitia::exception("Unable to prepare the sql query");
          }

    std::map<std::string, boost::shared_ptr<navitia::type::Message>> messages;
    boost::shared_ptr<navitia::type::Message> message;
    std::string current_uri = "";

    for(auto cursor = result.begin(); cursor != result.end(); ++cursor){
        if(cursor["uri"].as<std::string>() != current_uri){//on traite un nouveau message
            if(message){//si on a un message précédent, on le rajoute au map de résultat
                messages[message->uri] = message;
            }
            cursor["uri"].to(current_uri);
            //on construit le message
            message = boost::make_shared<navitia::type::Message>();
            message->uri = current_uri;
            cursor["object_uri"].to(message->object_uri);

            message->object_type = static_cast<navitia::type::Type_e>(
                    cursor["object_type_id"].as<int>());

            message->message_status = static_cast<navitia::type::MessageStatus>(
                    cursor["message_status_id"].as<int>());

            message->application_daily_start_hour = pt::duration_from_string(
                    cursor["start_application_daily_hour"].as<std::string>());

            message->application_daily_end_hour = pt::duration_from_string(
                    cursor["end_application_daily_hour"].as<std::string>());

            pt::ptime start = pt::time_from_string(
                    cursor["start_application_date"].as<std::string>());

            pt::ptime end = pt::time_from_string(
                    cursor["end_application_date"].as<std::string>());
            message->application_period = pt::time_period(start, end);

            start = pt::time_from_string(
                    cursor["start_publication_date"].as<std::string>());

            end = pt::time_from_string(
                    cursor["end_publication_date"].as<std::string>());
            message->publication_period = pt::time_period(start, end);

            message->active_days = std::bitset<8>(
                    cursor["active_days"].as<std::string>());
        }
        std::string language = cursor["language"].as<std::string>();
        cursor["body"].to(message->localized_messages[language].body);
        cursor["title"].to(message->localized_messages[language].title);
    }
    if(message){//on ajoute le dernier message traité
        messages[message->uri] = message;
    }

    return messages;
}

void apply_messages(navitia::type::Data& data){
    for(const auto message_pair : data.pt_data.message_holder.messages){
        if(message_pair.second->object_type ==  navitia::type::Type_e::StopArea){
            auto it = data.pt_data.stop_areas_map.find(message_pair.second->object_uri);
            if(it != data.pt_data.stop_areas_map.end()){
                it->second->messages.push_back(message_pair.second);
            }

        }

        if(message_pair.second->object_type ==  navitia::type::Type_e::StopPoint){
            auto it = data.pt_data.stop_points_map.find(message_pair.second->object_uri);
            if(it != data.pt_data.stop_points_map.end()){
                it->second->messages.push_back(message_pair.second);
            }
        }

        if(message_pair.second->object_type ==  navitia::type::Type_e::Route){
            auto it = data.pt_data.routes_map.find(message_pair.second->object_uri);
            if(it != data.pt_data.routes_map.end()){
                it->second->messages.push_back(message_pair.second);
            }
        }

        if(message_pair.second->object_type ==  navitia::type::Type_e::VehicleJourney){
            auto it = data.pt_data.vehicle_journeys_map.find(message_pair.second->object_uri);
            if(it != data.pt_data.vehicle_journeys_map.end()){
                it->second->messages.push_back(message_pair.second);
            }
        }

        if(message_pair.second->object_type ==  navitia::type::Type_e::Line){
            auto it = data.pt_data.lines_map.find(message_pair.second->object_uri);
            if(it != data.pt_data.lines_map.end()){
                it->second->messages.push_back(message_pair.second);
            }
        }
    }
}

std::vector<navitia::type::AtPerturbation> load_at_perturbations(
        const RealtimeLoaderConfig& conf,
        const boost::posix_time::ptime& current_time){
    //pour le moment on vire les timezone et on considére que c'est de l'heure local
    std::string request = "SELECT id, uri, start_application_date::timestamp, "
        "end_application_date::timestamp, start_application_daily_hour::time, "
        "end_application_daily_hour::time, active_days, object_uri, "
        "object_type_id "
        "FROM realtime.at_perturbation "
        "where end_application_date >= ($1::timestamp - $2::interval) ";
    pqxx::result result;
    std::unique_ptr<pqxx::connection> conn;
    try{
        conn = std::unique_ptr<pqxx::connection>(new pqxx::connection(conf.connection_string));
        (*conn).prepare("messages", request)("timestamp")("INTERVAL", pqxx::prepare::treat_string);
        std::string st_current_time = boost::posix_time::to_iso_string(current_time);
        std::string st_shift_days =  std::to_string(conf.shift_days) + " days";
        pqxx::work work(*conn, "chargement des perturbations at");
        result = work.prepared("messages")(st_current_time)(st_shift_days).exec();

    }catch(const pqxx::pqxx_exception &e){
        throw navitia::exception(e.base().what());

    }catch(...){
        throw navitia::exception("Unable to prepare the sql query");
    }


    std::vector<navitia::type::AtPerturbation> perturbations;
    for(auto cursor = result.begin(); cursor != result.end(); ++cursor){
        nt::AtPerturbation perturbation;
        cursor["uri"].to(perturbation.uri);
        //on construit le message
        cursor["object_uri"].to(perturbation.object_uri);

        perturbation.object_type = static_cast<navitia::type::Type_e>(
                cursor["object_type_id"].as<int>());

        perturbation.application_daily_start_hour = pt::duration_from_string(
                cursor["start_application_daily_hour"].as<std::string>());

        perturbation.application_daily_end_hour = pt::duration_from_string(
                cursor["end_application_daily_hour"].as<std::string>());

        pt::ptime start = pt::time_from_string(
                cursor["start_application_date"].as<std::string>());

        pt::ptime end = pt::time_from_string(
                cursor["end_application_date"].as<std::string>());
        perturbation.application_period = pt::time_period(start, end);

        perturbation.active_days = std::bitset<8>(
                cursor["active_days"].as<std::string>());

        perturbations.push_back(perturbation);
    }

    return perturbations;
}

}}//namespace
