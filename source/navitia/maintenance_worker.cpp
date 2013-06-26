#include "maintenance_worker.h"

#include "utils/configuration.h"

#include <sys/stat.h>
#include <signal.h>


namespace nt = navitia::type;
namespace pt = boost::posix_time;
namespace bg = boost::gregorian;


namespace navitia {

std::atomic<MaintenanceWorker*> dirty_pointer;

void sighandler(int signal){
    dirty_pointer.load()->sighandler(signal);
}

bool MaintenanceWorker::load_and_switch(){
    type::Data tmp_data;
    Configuration * conf = Configuration::get();
    std::string database = conf->get_as<std::string>("GENERAL", "database", "IdF.nav");
    LOG4CPLUS_INFO(logger, "Chargement des données à partir du fichier " + database);

    if(!tmp_data.load(database)) {
        LOG4CPLUS_ERROR(logger, "Erreur lors du chargement des données: " + database);
        return false;
    } else {
        tmp_data.loaded = true;
        LOG4CPLUS_TRACE(logger, "déplacement de data");
        boost::unique_lock<boost::shared_mutex> lock(data.load_mutex);
        data = std::move(tmp_data);
        LOG4CPLUS_TRACE(logger, "Chargement des données terminé");
        return true;
    }
}

void MaintenanceWorker::load(){
    if(data.to_load.load() || !data.loaded){
        load_and_switch();
        data.to_load.store(false);

        LOG4CPLUS_INFO(logger, boost::format("Nb data stop times : %d stopTimes : %d nb foot path : %d Nombre de stop points : %d")
                % data.pt_data.stop_times.size() % data.dataRaptor.arrival_times.size()
                % data.dataRaptor.foot_path_forward.size() % data.pt_data.stop_points.size()
            );
    }
}

void MaintenanceWorker::load_rt(){
    if(pt::microsec_clock::universal_time() > next_rt_load){

        Configuration * conf = Configuration::get();
        std::string database_rt = conf->get_as<std::string>("GENERAL", "database-rt", "");
        if(!database_rt.empty()){
            LOG4CPLUS_INFO(logger, "chargement des messages AT");
            nt::MessageHolder holder;
            holder.load(database_rt);

            boost::unique_lock<boost::shared_mutex> lock(data.load_mutex);
            data.pt_data.message_holder = std::move(holder);

            LOG4CPLUS_TRACE(logger, "chargement des messages AT fini!");
        }
        next_rt_load = pt::microsec_clock::universal_time() + pt::minutes(1);
    }
}

void MaintenanceWorker::operator()(){
    LOG4CPLUS_INFO(logger, "démarrage du thread de maintenance");
    while(true){
        load();

        load_rt();

        boost::this_thread::sleep(pt::seconds(1));
    }

}

void MaintenanceWorker::sighandler(int signal){
    if(signal == SIGHUP){
        data.to_load.store(true);
    }

}

MaintenanceWorker::MaintenanceWorker(navitia::type::Data & data) : data(data),
    logger(log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("background"))), next_rt_load(pt::microsec_clock::universal_time()){
    dirty_pointer = this;

    struct sigaction action;
    action.sa_handler = navitia::sighandler;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    sigaction(SIGHUP, &action, NULL);

}

}
