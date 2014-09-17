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

namespace nt = navitia::type;
namespace pt = boost::posix_time;
namespace bg = boost::gregorian;


namespace navitia {


void MaintenanceWorker::load(){
    std::string database = conf.vm["GENERAL.database"].as<std::string>();
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
            LOG4CPLUS_ERROR(logger, std::string("connection to rabbitmq fail: ")
                    + ex.what());
            data_manager.get_data()->is_connected_to_rabbitmq = false;
            sleep(10);
        }
    }
}

void MaintenanceWorker::listen_rabbitmq(){
    auto consumer_tag = this->channel->BasicConsume(this->queue_name);

    LOG4CPLUS_INFO(logger, "start event loop");
    data_manager.get_data()->is_connected_to_rabbitmq = true;
    while(true){
        auto envelope = this->channel->BasicConsumeMessage(consumer_tag);
        LOG4CPLUS_TRACE(logger, "Message received");
        pbnavitia::Task task;
        bool result = task.ParseFromString(envelope->Message()->Body());
        if(!result){
            LOG4CPLUS_WARN(logger, "protobuf not valid!");
            continue;
        }
        switch(task.action()){
            case pbnavitia::RELOAD:
                load(); break;
            default:
                LOG4CPLUS_TRACE(logger, "Message ignored");
        }
    }
}

void MaintenanceWorker::init_rabbitmq(){
    std::string instance_name = conf.vm["GENERAL.instance_name"].as<std::string>();
    std::string exchange_name = conf.vm["BROKER.exchange"].as<std::string>();
    std::string host = conf.vm["BROKER.host"].as<std::string>();
    int port = conf.vm["BROKER.port"].as<int>();
    std::string username = conf.vm["BROKER.username"].as<std::string>();
    std::string password = conf.vm["BROKER.password"].as<std::string>();
    std::string vhost = conf.vm["BROKER.vhost"].as<std::string>();
    //connection
    LOG4CPLUS_DEBUG(logger, boost::format("connection to rabbitmq: %s@%s:%s/%s") % username % host % port % vhost);
    this->channel = AmqpClient::Channel::Create(host, port, username, password, vhost);

    this->channel->DeclareExchange(exchange_name, "topic", false, true, false);

    //creation of a tempory queue for this kraken
    this->queue_name = channel->DeclareQueue("", false, false, true, true);
    //binding the queue to the exchange for all task for this instance
    channel->BindQueue(queue_name, exchange_name, instance_name+".task.*");
    LOG4CPLUS_DEBUG(logger, "connected to rabbitmq");
}

MaintenanceWorker::MaintenanceWorker(DataManager<type::Data>& data_manager, kraken::Configuration conf) :
        data_manager(data_manager), conf(conf),
        logger(log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("background"))){}

}
