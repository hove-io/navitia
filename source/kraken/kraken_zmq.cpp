#include "type/type.pb.h"
#include <google/protobuf/descriptor.h>

#include <boost/thread.hpp>
#include <functional>
#include <string>
#include <iostream>
#include "utils/init.h"
#include "kraken_zmq.h"


int main(int, char** argv){
    navitia::init_app();
    Configuration* conf = Configuration::get();

    std::string::size_type posSlash = std::string(argv[0]).find_last_of( "\\/" );
    conf->set_string("application", std::string(argv[0]).substr(posSlash+1));
    char buf[256];
    if(getcwd(buf, sizeof(buf))){
        conf->set_string("path",std::string(buf) + "/");
    }else{
        perror("getcwd");
        return 1;
    }

    std::string conf_file = conf->get_string("path") + conf->get_string("application") + ".ini";
    conf->load_ini(conf_file);
    init_logger(conf_file);

    DataManager<navitia::type::Data> data_manager;

    boost::thread_group threads;
    // Prepare our context and sockets
    zmq::context_t context(1);
    zmq::socket_t clients(context, ZMQ_ROUTER);
    std::string zmq_socket = conf->get_as<std::string>("GENERAL", "zmq_socket", "ipc:///tmp/default_navitia");
    clients.bind(zmq_socket.c_str());
    zmq::socket_t workers(context, ZMQ_DEALER);
    workers.bind("inproc://workers");

    threads.create_thread(navitia::MaintenanceWorker(data_manager));

    int nb_threads = conf->get_as<int>("GENERAL", "nb_threads", 1);
    // Launch pool of worker threads
    for(int thread_nbr = 0; thread_nbr < nb_threads; ++thread_nbr) {
        threads.create_thread(std::bind(&doWork, std::ref(context), std::ref(data_manager)));
    }

    // Connect work threads to client threads via a queue
    do{
        try{
            zmq::device(ZMQ_QUEUE, clients, workers);
        }catch(zmq::error_t){}//lors d'un SIGHUP on restore la queue
    }while(true);

    return 0;
}

