#include "maintenance_worker.h"

#include "utils/configuration.h"

#include <sys/stat.h>
#include <signal.h>
#include "type/task.pb.h"

namespace nt = navitia::type;
namespace pt = boost::posix_time;
namespace bg = boost::gregorian;


namespace navitia {


bool MaintenanceWorker::load_and_switch(){
    type::Data * tmp_data = new type::Data();
    Configuration * conf = Configuration::get();
    std::string database = conf->get_as<std::string>("GENERAL", "database", "IdF.nav");
    LOG4CPLUS_INFO(logger, "Chargement des données à partir du fichier " + database);

    bool return_= false;
    if(!tmp_data->load(database)) {
        LOG4CPLUS_ERROR(logger, "Erreur lors du chargement des données: " + database);
        return_= false;
    } else {
        tmp_data->loaded = true;
        LOG4CPLUS_TRACE(logger, "déplacement de data");
        boost::unique_lock<boost::shared_mutex> lock((*data)->load_mutex);
        std::swap(*data, tmp_data);
        LOG4CPLUS_TRACE(logger, "Chargement des données terminé");
        return_ = true;
    }
    delete tmp_data;
    return return_;

}

void MaintenanceWorker::load(){
    load_and_switch();

    LOG4CPLUS_INFO(logger, boost::format("Nb data stop times : %d stopTimes : %d nb foot path : %d Nombre de stop points : %d")
            % (*data)->pt_data.stop_times.size() % (*data)->dataRaptor.arrival_times.size()
            % (*data)->dataRaptor.foot_path_forward.size() % (*data)->pt_data.stop_points.size()
            );
}


void MaintenanceWorker::operator()(){
    LOG4CPLUS_INFO(logger, "démarrage du thread de maintenance");
    //
    load();
    //@TODO dans la prochaine version de simpleAMQPclient on devrait ne pas avoir a spécifier la queue
    auto consumer_tag = this->channel->BasicConsume(this->queue_name);

    bool running = true;
    LOG4CPLUS_TRACE(logger, "début de la boucle d'evenement");
    while(running){
        auto envelope = this->channel->BasicConsumeMessage(consumer_tag);
        LOG4CPLUS_TRACE(logger, "Message reçu");
        pbnavitia::Task task;
        bool result = task.ParseFromString(envelope->Message()->Body());
        if(!result){
            LOG4CPLUS_WARN(logger, "impossible de parser le protobuf reçu");
            continue;
        }
        if(task.action() == pbnavitia::RELOAD){
            load();
        }
    }

}

void MaintenanceWorker::init_rabbitmq(){
    Configuration * conf = Configuration::get();
    std::string instance_name = conf->get_as<std::string>("GENERAL", "instance_name", "");
    std::string exchange_name = conf->get_as<std::string>("BROKER", "exchange", "navitia");
    std::string host = conf->get_as<std::string>("BROKER", "host", "localhost");
    int port = conf->get_as<int>("BROKER", "port", 5672);
    std::string username = conf->get_as<std::string>("BROKER", "username", "guest");
    std::string password = conf->get_as<std::string>("BROKER", "password", "guest");
    std::string vhost = conf->get_as<std::string>("BROKER", "vhost", "/");
    //connection
    LOG4CPLUS_DEBUG(logger, "connection à rabbitmq");
    this->channel = AmqpClient::Channel::Create(host, port, username,
            password, vhost);

    //création de l'exchange (il devrait deja exister, mais dans l'doute)
    this->channel->DeclareExchange(exchange_name, "topic", false, true, false);

    //création d'une queue temporaire
    this->queue_name = channel->DeclareQueue("", false, false, true, true);
    //on se bind sur l'echange precedement créer, et on prend tous les messages possible pôur cette instance
    channel->BindQueue(queue_name, exchange_name, instance_name+".task.*");
    LOG4CPLUS_TRACE(logger, "connecté à rabbitmq");
}

MaintenanceWorker::MaintenanceWorker(type::Data** data) : data(data),
        logger(log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("background"))),
        next_rt_load(pt::microsec_clock::universal_time()){
    init_rabbitmq();
}

}
