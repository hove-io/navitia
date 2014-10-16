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

#include "maintenance_worker.h"

#include <sys/stat.h>
#include <signal.h>
#include "type/task.pb.h"
#include "type/chaos.pb.h"
#include "type/gtfs-realtime.pb.h"
#include "type/pt_data.h"
#include "type/message.h"
#include <boost/algorithm/string/join.hpp>

namespace nt = navitia::type;
namespace pt = boost::posix_time;
namespace bg = boost::gregorian;


namespace navitia {


void MaintenanceWorker::load(){
    std::string database = conf.databases_path();
    LOG4CPLUS_INFO(logger, "Chargement des données à partir du fichier " + database);
    if(this->data_manager.load(database)){
        auto data = data_manager.get_data();

    }
}


void MaintenanceWorker::operator()(){
    LOG4CPLUS_INFO(logger, "starting background thread");
    load();

    while(true){
        try{
            this->init_rabbitmq();
            this->listen_rabbitmq();
        }catch(const std::runtime_error& ex){
            LOG4CPLUS_ERROR(logger, std::string("connection to rabbitmq fail: ") << ex.what());
            data_manager.get_data()->is_connected_to_rabbitmq = false;
            sleep(10);
        }
    }
}

void MaintenanceWorker::handle_task(AmqpClient::Envelope::ptr_t envelope){
    LOG4CPLUS_TRACE(logger, "task received");
    pbnavitia::Task task;
    bool result = task.ParseFromString(envelope->Message()->Body());
    if(!result){
        LOG4CPLUS_WARN(logger, "protobuf not valid!");
        return;
    }
    switch(task.action()){
        case pbnavitia::RELOAD:
            load(); break;
        default:
            LOG4CPLUS_TRACE(logger, "task ignored");
    }
}

static std::shared_ptr<nt::new_disruption::Tag>
make_tag(const chaos::Tag& chaos_tag, nt::new_disruption::DisruptionHolder& holder) {
    auto from_posix = navitia::from_posix_timestamp;

    auto& weak_tag = holder.tags[chaos_tag.id()];
    if (auto tag = weak_tag.lock()) { return std::move(tag); }

    auto tag = std::make_shared<nt::new_disruption::Tag>();
    tag->uri = chaos_tag.id();
    tag->name = chaos_tag.name();
    tag->created_at = from_posix(chaos_tag.created_at());
    tag->updated_at = from_posix(chaos_tag.updated_at());

    weak_tag = tag;
    return std::move(tag);
}

static std::shared_ptr<nt::new_disruption::Cause>
make_cause(const chaos::Cause& chaos_cause, nt::new_disruption::DisruptionHolder& holder) {
    auto from_posix = navitia::from_posix_timestamp;

    auto& weak_cause = holder.causes[chaos_cause.id()];
    if (auto cause = weak_cause.lock()) { return std::move(cause); }

    auto cause = std::make_shared<nt::new_disruption::Cause>();
    cause->uri = chaos_cause.id();
    cause->wording = chaos_cause.wording();
    cause->created_at = from_posix(chaos_cause.created_at());
    cause->updated_at = from_posix(chaos_cause.updated_at());

    weak_cause = cause;
    return std::move(cause);

}

static std::shared_ptr<nt::new_disruption::Severity>
make_severity(const chaos::Severity& chaos_severity, nt::new_disruption::DisruptionHolder& holder) {
    namespace tr = transit_realtime;
    namespace new_disr = nt::new_disruption;
    auto from_posix = navitia::from_posix_timestamp;

    auto& weak_severity = holder.severities[chaos_severity.id()];
    if (auto severity = weak_severity.lock()) { return std::move(severity); }

    auto severity = std::make_shared<new_disr::Severity>();
    severity->uri = chaos_severity.id();
    severity->wording = chaos_severity.wording();
    severity->created_at = from_posix(chaos_severity.created_at());
    severity->updated_at = from_posix(chaos_severity.updated_at());
    severity->color = chaos_severity.color();
    severity->priority = chaos_severity.priority();
    switch (chaos_severity.effect()) {
#define EFFECT_ENUM_CONVERSION(e) \
        case tr::Alert_Effect_##e: severity->effect = new_disr::Effect::e; break

        EFFECT_ENUM_CONVERSION(NO_SERVICE);
        EFFECT_ENUM_CONVERSION(REDUCED_SERVICE);
        EFFECT_ENUM_CONVERSION(SIGNIFICANT_DELAYS);
        EFFECT_ENUM_CONVERSION(DETOUR);
        EFFECT_ENUM_CONVERSION(ADDITIONAL_SERVICE);
        EFFECT_ENUM_CONVERSION(MODIFIED_SERVICE);
        EFFECT_ENUM_CONVERSION(OTHER_EFFECT);
        EFFECT_ENUM_CONVERSION(UNKNOWN_EFFECT);
        EFFECT_ENUM_CONVERSION(STOP_MOVED);

#undef EFFECT_ENUM_CONVERSION
    }

    return std::move(severity);
}

static std::vector<nt::new_disruption::PtObject>
make_pt_objects(const google::protobuf::RepeatedPtrField<chaos::PtObject>& chaos_pt_objects) {
    std::vector<nt::new_disruption::PtObject> res;
    for (const auto& chaos_pt_object: chaos_pt_objects) {
        nt::Type_e type = nt::Type_e::Unknown;
        switch (chaos_pt_object.pt_object_type()) {
        case chaos::PtObject_Type_network:      type = nt::Type_e::Network;        break;
        case chaos::PtObject_Type_stop_area:    type = nt::Type_e::StopArea;       break;
        case chaos::PtObject_Type_line_section: type = nt::Type_e::Line /* ??? */; break;
        case chaos::PtObject_Type_line:         type = nt::Type_e::Line;           break;
        case chaos::PtObject_Type_route:        type = nt::Type_e::Route;          break;
        case chaos::PtObject_Type_unkown_type:  type = nt::Type_e::Unknown;        break;
        }
        res.push_back({type, chaos_pt_object.uri()});
        // no created_at and updated_at?
    }
    return res;
}

static std::vector<nt::new_disruption::PtObject>
make_pt_objects(const google::protobuf::RepeatedPtrField<transit_realtime::EntitySelector>& chaos_pt_objects) {
    std::vector<nt::new_disruption::PtObject> res;
    for (const auto& chaos_selector: chaos_pt_objects) {
        if (chaos_selector.has_agency_id()) {
            res.push_back({nt::Type_e::Contributor /* ??? */, chaos_selector.agency_id()});
            continue;
        }
        if (chaos_selector.has_route_id()) {
            res.push_back({nt::Type_e::Route , chaos_selector.route_id()});
            continue;
        }
        if (chaos_selector.has_route_type()) {
            // gloups, what to do here???
            //res.push_back({nt::Type_e::Route /* ??? */ , to_string(chaos_selector.route_type())});
            continue;
        }
        if (chaos_selector.has_trip()) {
            // there is much more information in here...
            res.push_back({nt::Type_e::VehicleJourney , chaos_selector.trip().trip_id()});
            continue;
        }
        if (chaos_selector.has_stop_id()) {
            res.push_back({nt::Type_e::StopPoint /* or StopArea??? */ , chaos_selector.stop_id()});
            continue;
        }
    }
    return res;
}

static std::shared_ptr<nt::new_disruption::Impact>
make_impact(const chaos::Impact& chaos_impact, nt::new_disruption::DisruptionHolder& holder) {
    auto from_posix = navitia::from_posix_timestamp;

    auto impact = std::make_shared<nt::new_disruption::Impact>();
    impact->uri = chaos_impact.id();
    impact->created_at = from_posix(chaos_impact.created_at());
    impact->updated_at = from_posix(chaos_impact.updated_at());
    for (const auto& chaos_ap: chaos_impact.application_periods()) {
        impact->application_periods.emplace_back(from_posix(chaos_ap.start()), from_posix(chaos_ap.end()));
    }
    impact->severity = make_severity(chaos_impact.severity(), holder);
    impact->informed_entities = make_pt_objects(chaos_impact.informed_entities());
    for (const auto& chaos_message: chaos_impact.messages()) {
        impact->messages.push_back({
            chaos_message.text(),
            from_posix(chaos_message.created_at()),
            from_posix(chaos_message.updated_at()),
        });
    }

    return std::move(impact);
}

static void add_disruption(nt::new_disruption::DisruptionHolder& holder,
                           const chaos::Disruption& chaos_disruption) {
    auto from_posix = navitia::from_posix_timestamp;

    auto disruption = std::make_unique<nt::new_disruption::Disruption>();
    disruption->uri = chaos_disruption.id();
    disruption->reference = chaos_disruption.reference();
    disruption->publication_period = {
        from_posix(chaos_disruption.publication_period().start()),
        from_posix(chaos_disruption.publication_period().end())
    };
    disruption->created_at = from_posix(chaos_disruption.created_at());
    disruption->updated_at = from_posix(chaos_disruption.updated_at());
    disruption->cause = make_cause(chaos_disruption.cause(), holder);
    for (const auto& chaos_impact: chaos_disruption.impacts()) {
        disruption->impacts.push_back(make_impact(chaos_impact, holder));
    }
    disruption->localization = make_pt_objects(chaos_disruption.localization());
    for (const auto& chaos_tag: chaos_disruption.tags()) {
        disruption->tags.push_back(make_tag(chaos_tag, holder));
    }
    disruption->note = chaos_disruption.note();

    holder.disruptions.push_back(std::move(disruption));
}

void MaintenanceWorker::handle_rt(AmqpClient::Envelope::ptr_t envelope){
    LOG4CPLUS_TRACE(logger, "realtime info received!");
    transit_realtime::FeedMessage feed_message;
    if(! feed_message.ParseFromString(envelope->Message()->Body())){
        LOG4CPLUS_WARN(logger, "protobuf not valid!");
        return;
    }
    std::shared_ptr<nt::Data> data;
    for(const auto& entity: feed_message.entity()){
        if(entity.HasExtension(chaos::disruption)){
            LOG4CPLUS_WARN(logger, "has_extension");
            if (!data) { data = data_manager.get_data_clone(); }
            add_disruption(data->pt_data->disruption_holder, entity.GetExtension(chaos::disruption));
            LOG4CPLUS_DEBUG(logger, "end");
        }else{
            LOG4CPLUS_WARN(logger, "unsupported gtfs rt feed");
        }
    }
    if (data) { data_manager.set_data(std::move(data)); }
}

void MaintenanceWorker::listen_rabbitmq(){
    std::string task_tag = this->channel->BasicConsume(this->queue_name_task);
    std::string rt_tag = this->channel->BasicConsume(this->queue_name_rt);

    LOG4CPLUS_INFO(logger, "start event loop");
    data_manager.get_data()->is_connected_to_rabbitmq = true;
    while(true){
        auto envelope = this->channel->BasicConsumeMessage(std::vector<std::string>({task_tag, rt_tag}));
        if(envelope->ConsumerTag() == task_tag){
            handle_task(envelope);
        }else if(envelope->ConsumerTag() == rt_tag){
            handle_rt(envelope);
        }
    }
}

void MaintenanceWorker::init_rabbitmq(){
    std::string instance_name = conf.instance_name();
    std::string exchange_name = conf.broker_exchange();
    std::string host = conf.broker_host();
    int port = conf.broker_port();
    std::string username = conf.broker_username();
    std::string password = conf.broker_password();
    std::string vhost = conf.broker_vhost();
    //connection
    LOG4CPLUS_DEBUG(logger, boost::format("connection to rabbitmq: %s@%s:%s/%s") % username % host % port % vhost);
    this->channel = AmqpClient::Channel::Create(host, port, username, password, vhost);

    this->channel->DeclareExchange(exchange_name, "topic", false, true, false);

    //creation of a tempory queue for this kraken
    this->queue_name_task = channel->DeclareQueue("", false, false, true, true);
    //binding the queue to the exchange for all task for this instance
    channel->BindQueue(queue_name_task, exchange_name, instance_name+".task.*");

    this->queue_name_rt = channel->DeclareQueue("", false, false, true, true);
    //binding the queue to the exchange for all task for this instance
    LOG4CPLUS_INFO(logger, "subscribing to [" << boost::algorithm::join(conf.rt_topics(), ", ") << "]");
    for(auto topic: conf.rt_topics()){
        channel->BindQueue(queue_name_rt, exchange_name, topic);
    }
    LOG4CPLUS_DEBUG(logger, "connected to rabbitmq");
}

MaintenanceWorker::MaintenanceWorker(DataManager<type::Data>& data_manager, kraken::Configuration conf) :
        data_manager(data_manager),
        logger(log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("background"))),
        conf(conf){}

}
