/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!
  
LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
   
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
   
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
  
Stay tuned using
twitter @navitia 
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "messages_connector.h"
#include <pqxx/pqxx>
#include <boost/make_shared.hpp>
#include "utils/exception.h"
#include "utils/logger.h"
#include "type/data.h"
#include "type/pt_data.h"
#include <boost/dynamic_bitset.hpp>

namespace ed{ namespace connectors{

namespace pt = boost::posix_time;
using nt::new_disruption::Severity;
using nt::new_disruption::Impact;
using nt::new_disruption::Effect;
using nt::new_disruption::DisruptionHolder;

std::shared_ptr<Severity>
get_or_create_severity(DisruptionHolder& disruptions, int id) {
    std::string name;
    if (id == 0) {
        name = "information";
    } else if (id == 1) {
        name = "warning";
    } else if (id == 2) {
        name = "disrupt";
    } else {
        throw navitia::exception("Disruption: invalid severity in database");
    }

    auto it = disruptions.severities.find(name);

    if (it != disruptions.severities.end()) {
        return it->second.lock(); //we acquire the weak_ptr
    }

    std::shared_ptr<Severity> severity = std::make_shared<Severity>();
    disruptions.severities.insert({name, severity});

    severity->uri = name;
    severity->effect = Effect::UNKNOWN_EFFECT; //is it right ?

    return severity;
}

std::vector<pt::time_period> split_period(pt::ptime start, pt::ptime end,
                                          pt::time_duration beg_of_day, pt::time_duration end_of_day,
                                          std::bitset<7> days) {
    if (days.all() && beg_of_day == pt::hours(0) && end_of_day == pt::hours(24)) {
        return {{start, end}};
    }
    auto period = boost::gregorian::date_period(start.date(), end.date());

    std::vector<pt::time_period> res;
    //what is the first day of the at_message bitset ? monday or sunday ? without response, I considere it to be sunday
    for(boost::gregorian::day_iterator it(period.begin()); it<period.end(); ++it) {
        auto day = (*it);
        if(! days.test(day.day_of_week())) {
            continue;
        }
        res.push_back(pt::time_period(pt::ptime(day, beg_of_day), pt::ptime(day, end_of_day)));
    }

    return res;
}

navitia::type::new_disruption::DisruptionHolder load_disruptions(
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

#if PQXX_VERSION_MAJOR < 4
        conn->prepare("messages", request)("timestamp")("INTERVAL", pqxx::prepare::treat_string);
#else
        conn->prepare("messages", request);
#endif

        std::string st_current_time = boost::posix_time::to_iso_string(current_time);
        std::string st_shift_days =  std::to_string(conf.shift_days) + " days";
        pqxx::work work(*conn, "chargement des messages");
        result = work.prepared("messages")(st_current_time)(st_shift_days).exec();
    } catch(const pqxx::pqxx_exception& e) {
        throw navitia::exception(e.base().what());
    }

    nt::new_disruption::DisruptionHolder disruptions;
    std::shared_ptr<nt::new_disruption::Impact> impact;
    std::string current_uri = "";

    for (auto cursor = result.begin(); cursor != result.end(); ++cursor) {
        //we can have several language handled by the database, so we create one message for each
        if (cursor["uri"].as<std::string>() != current_uri) {
            disruptions.disruptions.push_back(std::make_unique<nt::new_disruption::Disruption>());

            auto& disruption = disruptions.disruptions.back();
            cursor["uri"].to(current_uri);
            disruption->uri = current_uri;

            impact = std::shared_ptr<nt::new_disruption::Impact>();
            cursor["uri"].to(impact->uri);

            //we need to handle the active days to split the period
            pt::ptime start = pt::time_from_string(cursor["start_application_date"].as<std::string>());
            pt::ptime end = pt::time_from_string(cursor["end_application_date"].as<std::string>());

            auto daily_start_hour = pt::duration_from_string(
                        cursor["start_application_daily_hour"].as<std::string>());
            auto daily_end_hour = pt::duration_from_string(
                        cursor["end_application_daily_hour"].as<std::string>());

            std::bitset<7> active_days (cursor["active_days"].as<std::string>());

            impact->application_periods = split_period(start, end,
                                                       daily_start_hour, daily_end_hour,
                                                       active_days);
            disruption->impacts.push_back(impact);
            disruptions.disruptions.push_back(std::move(disruption));
        }
        auto message = nt::new_disruption::Message();
        cursor["body"].to(message.text);
        impact->messages.push_back(message);
        auto pt_object = nt::new_disruption::PtObject();
        cursor["object_uri"].to(pt_object.object_uri);
        pt_object.object_type = static_cast<navitia::type::Type_e>(cursor["object_type_id"].as<int>());
        impact->informed_entities.push_back(pt_object);

        auto severity_id = cursor["message_status_id"].as<int>();
        auto severity = get_or_create_severity(disruptions, severity_id);
        impact->severity = severity;

        // message does not have tags, notes, localization or cause
    }

    return disruptions;
}

template <typename Container>
void add_impact(Container& map, const std::string& uri, const std::shared_ptr<Impact>& impact) {
    auto it = map.find(uri);
    if(it != map.end()){
        it->second->impacts.push_back(impact);
    }
}

void apply_messages(navitia::type::Data& data){

    for (const auto& disruption: data.pt_data->disruption_holder.disruptions) {
        for (const auto& impact: disruption->impacts) {
            for (const auto& pb_object: impact->informed_entities) {
                if (pb_object.object_type == navitia::type::Type_e::StopArea) {
                    add_impact(data.pt_data->stop_areas_map, pb_object.object_uri, impact);
                }
                if (pb_object.object_type == navitia::type::Type_e::StopPoint) {
                    add_impact(data.pt_data->stop_points_map, pb_object.object_uri, impact);
                }
                if (pb_object.object_type == navitia::type::Type_e::Route) {
                    add_impact(data.pt_data->routes_map, pb_object.object_uri, impact);
                }
                if (pb_object.object_type == navitia::type::Type_e::VehicleJourney) {
                    add_impact(data.pt_data->vehicle_journeys_map, pb_object.object_uri, impact);
                }
                if (pb_object.object_type == navitia::type::Type_e::Line) {
                    add_impact(data.pt_data->lines_map, pb_object.object_uri, impact);
                }
                if (pb_object.object_type == navitia::type::Type_e::Network) {
                    add_impact(data.pt_data->networks_map, pb_object.object_uri, impact);
                }
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

#if PQXX_VERSION_MAJOR < 4
        conn->prepare("messages", request)("timestamp")("INTERVAL", pqxx::prepare::treat_string);
#else
        conn->prepare("messages", request);
#endif

        std::string st_current_time = boost::posix_time::to_iso_string(current_time);
        std::string st_shift_days =  std::to_string(conf.shift_days) + " days";
        pqxx::work work(*conn, "chargement des perturbations at");
        result = work.prepared("messages")(st_current_time)(st_shift_days).exec();
    } catch(const pqxx::pqxx_exception &e) {
        throw navitia::exception(e.base().what());
    }


    std::vector<navitia::type::AtPerturbation> perturbations;
    for(auto cursor = result.begin(); cursor != result.end(); ++cursor){
        navitia::type::AtPerturbation perturbation;
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
