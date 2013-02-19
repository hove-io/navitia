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
        boost::unique_lock<boost::shared_mutex> lock(tmp_data.load_mutex);
        data = std::move(tmp_data);
        LOG4CPLUS_TRACE(logger, "Chargement des données terminé");
        return true;
    }
}

void MaintenanceWorker::load(){
    if(data.to_load.load() || !data.loaded){
        load_and_switch();
        data.to_load.store(false);
    }
}

void MaintenanceWorker::operator()(){
    LOG4CPLUS_INFO(logger, "démarrage du thread de maintenance");
    while(true){
        load();

        boost::this_thread::sleep(pt::seconds(1));
    }

}

void MaintenanceWorker::sighandler(int signal){
    if(signal == SIGHUP){
        data.to_load.store(true);
    }

}

MaintenanceWorker::MaintenanceWorker(navitia::type::Data & data) : data(data), logger(log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("background"))){
    dirty_pointer = this;

    struct sigaction action;
    action.sa_handler = navitia::sighandler;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    sigaction(SIGHUP, &action, NULL);

}

}
