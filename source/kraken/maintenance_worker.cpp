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

#include "fill_disruption_from_chaos.h"
#include "realtime.h"
#include "type/task.pb.h"
#include "type/pt_data.h"
#include <boost/algorithm/string/join.hpp>
#include <boost/optional.hpp>
#include <sys/stat.h>
#include <signal.h>
#include <SimpleAmqpClient/Envelope.h>
#include <chrono>
#include <thread>
#include "utils/get_hostname.h"

namespace nt = navitia::type;
namespace pt = boost::posix_time;
namespace bg = boost::gregorian;


namespace navitia {


void MaintenanceWorker::load(){
    const std::string database = conf.databases_path();
    auto chaos_database = conf.chaos_database();
    auto contributors = conf.rt_topics();
    LOG4CPLUS_INFO(logger, "Loading database from file: " + database);
    if(this->data_manager.load(database, chaos_database, contributors)){
        auto data = data_manager.get_data();
        data->meta->instance_name = conf.instance_name();
    }
}


void MaintenanceWorker::operator()(){
    LOG4CPLUS_INFO(logger, "Starting background thread");

    try{
        this->listen_rabbitmq();
    }catch(const std::runtime_error& ex){
        LOG4CPLUS_ERROR(logger, "Connection to rabbitmq failed: " << ex.what());
        data_manager.get_data()->is_connected_to_rabbitmq = false;
        sleep(10);
    }
    while(true){
        try{
            this->init_rabbitmq();
            this->listen_rabbitmq();
        }catch(const std::runtime_error& ex){
            LOG4CPLUS_ERROR(logger, "Connection to rabbitmq failed: " << ex.what());
            data_manager.get_data()->is_connected_to_rabbitmq = false;
            sleep(10);
        }
    }
}

void MaintenanceWorker::handle_task_in_batch(const std::vector<AmqpClient::Envelope::ptr_t>& envelopes){
    for (auto& envelope: envelopes) {
        LOG4CPLUS_TRACE(logger, "Task received");
        pbnavitia::Task task;
        bool result = task.ParseFromString(envelope->Message()->Body());
        if(!result){
            LOG4CPLUS_WARN(logger, "Protobuf is not valid!");
            return;
        }
        switch(task.action()){
            case pbnavitia::RELOAD:
                load(); break;
            default:
                LOG4CPLUS_TRACE(logger, "Task ignored");
        }
    }
}


void MaintenanceWorker::handle_rt_in_batch(const std::vector<AmqpClient::Envelope::ptr_t>& envelopes){
    auto data = boost::shared_ptr<nt::Data>{nullptr};
    for (auto& envelope: envelopes) {
        LOG4CPLUS_DEBUG(logger, "realtime info received!");
        assert(envelope);
        transit_realtime::FeedMessage feed_message;
        if(! feed_message.ParseFromString(envelope->Message()->Body())){
            LOG4CPLUS_WARN(logger, "protobuf not valid!");
            return;
        }
        LOG4CPLUS_TRACE(logger, "received entity: " << feed_message.DebugString());
        for(const auto& entity: feed_message.entity()){
            if (!data) {
                data = data_manager.get_data_clone();
                data->last_rt_data_loaded = pt::microsec_clock::universal_time();
            }
            if (entity.is_deleted()) {
                LOG4CPLUS_DEBUG(logger, "deletion of disruption " << entity.id());
                delete_disruption(entity.id(), *data->pt_data, *data->meta);
            } else if(entity.HasExtension(chaos::disruption)) {
                LOG4CPLUS_DEBUG(logger, "add/update of disruption " << entity.id());
                add_disruption(entity.GetExtension(chaos::disruption), *data->pt_data, *data->meta);
            } else if(entity.has_trip_update()) {
                LOG4CPLUS_DEBUG(logger, "RT trip update" << entity.id());
                handle_realtime(entity.trip_update(), *data);
            } else {
                LOG4CPLUS_WARN(logger, "unsupported gtfs rt feed");
            }
        }
    }
    if (data) {
        LOG4CPLUS_INFO(logger, "rebuilding data raptor");
        data->build_raptor();
        data_manager.set_data(std::move(data));
        LOG4CPLUS_INFO(logger, "data updated");
    }
}

std::vector<AmqpClient::Envelope::ptr_t>
MaintenanceWorker::consume_in_batch(const std::string& consume_tag,
        size_t max_nb,
        size_t timeout_ms){
    assert(consume_tag != "");
    assert(max_nb);

    bool not_empty = true;
    std::vector<AmqpClient::Envelope::ptr_t> envelopes;
    envelopes.reserve(max_nb);
    size_t consumed_nb = 0;
    while(not_empty && consumed_nb < max_nb) {
        AmqpClient::Envelope::ptr_t envelope = nullptr;
        not_empty = channel->BasicConsumeMessage(consume_tag, envelope, timeout_ms);
        if (not_empty && envelope) {
            envelopes.push_back(envelope);
            channel->BasicAck(envelope);
            ++ consumed_nb;
        }
    }
    return envelopes;
}

void MaintenanceWorker::listen_rabbitmq(){
    if(!channel){
        throw std::runtime_error("not connected to rabbitmq");
    }
    bool no_local = true;
    bool no_ack = false;
    std::string task_tag = this->channel->BasicConsume(this->queue_name_task, "", no_local, no_ack);
    std::string rt_tag = this->channel->BasicConsume(this->queue_name_rt, "", no_local, no_ack);

    LOG4CPLUS_INFO(logger, "start event loop");
    data_manager.get_data()->is_connected_to_rabbitmq = true;
    while(true){
        size_t timeout_ms = conf.broker_timeout();
        size_t max_batch_nb = 100; // Magic Number :)

        auto rt_envelopes = consume_in_batch(rt_tag, max_batch_nb, timeout_ms);
        handle_rt_in_batch(rt_envelopes);

        auto task_envelopes = consume_in_batch(task_tag, 1, timeout_ms);
        handle_task_in_batch(task_envelopes);

        // Since consume_in_batch is non blocking, we don't want that the worker loops for nothing, when the
        // queue is empty.
        std::this_thread::sleep_for(std::chrono::seconds(conf.broker_sleeptime()));
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

    std::string hostname = get_hostname();
    this->queue_name_task = (boost::format("kraken_%s_%s_task") % hostname % instance_name).str();
    this->queue_name_rt = (boost::format("kraken_%s_%s_rt") % hostname % instance_name).str();
    //connection
    LOG4CPLUS_DEBUG(logger, boost::format("connection to rabbitmq: %s@%s:%s/%s") % username % host % port % vhost);
    this->channel = AmqpClient::Channel::Create(host, port, username, password, vhost);
    if(!is_initialized){
        //first we have to delete the queues, binding can change between two run, and it's don't seem possible
        //to unbind a queue if we don't know at what topic it's subscribed
        //if the queue doesn't exist an exception is throw...
        try{
            channel->DeleteQueue(queue_name_task);
        }catch(const std::runtime_error&){}
        try{
            channel->DeleteQueue(queue_name_rt);
        }catch(const std::runtime_error&){}

        this->channel->DeclareExchange(exchange_name, "topic", false, true, false);

        //creation of queues for this kraken
        channel->DeclareQueue(this->queue_name_task, false, true, false, false);
        LOG4CPLUS_INFO(logger, "queue for tasks: " << this->queue_name_task);
        //binding the queue to the exchange for all task for this instance
        channel->BindQueue(queue_name_task, exchange_name, instance_name+".task.*");

        channel->DeclareQueue(this->queue_name_rt, false, true, false, false);
        LOG4CPLUS_INFO(logger, "queue for disruptions: " << this->queue_name_rt);
        //binding the queue to the exchange for all task for this instance
        LOG4CPLUS_INFO(logger, "subscribing to [" << boost::algorithm::join(conf.rt_topics(), ", ") << "]");
        for(auto topic: conf.rt_topics()){
            channel->BindQueue(queue_name_rt, exchange_name, topic);
        }
        is_initialized = true;
    }
    LOG4CPLUS_DEBUG(logger, "connected to rabbitmq");
}

MaintenanceWorker::MaintenanceWorker(DataManager<type::Data>& data_manager, kraken::Configuration conf) :
        data_manager(data_manager),
        logger(log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("background"))),
        conf(conf){
    try{
        this->init_rabbitmq();
    }catch(const std::runtime_error& ex){
        LOG4CPLUS_ERROR(logger, "Connection to rabbitmq failed: " << ex.what());
        data_manager.get_data()->is_connected_to_rabbitmq = false;
    }
    load();
}

}
