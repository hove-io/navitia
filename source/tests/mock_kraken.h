#include "type/data.h"
struct mock_kraken {
    mock_kraken(navitia::type::Data& d) {

        DataManager<navitia::type::Data> data_manager;

        boost::thread_group threads;
        // Prepare our context and sockets
        zmq::context_t context(1);
        zmq::socket_t clients(context, ZMQ_ROUTER);
        std::string zmq_socket = conf->get_as<std::string>("GENERAL", "zmq_socket", "ipc:///tmp/default_navitia");
        clients.bind(zmq_socket.c_str());
        zmq::socket_t workers(context, ZMQ_DEALER);
        workers.bind("inproc://workers");

        // Launch pool of worker threads
        for(int thread_nbr = 0; thread_nbr < data_manager.get_data()->nb_threads; ++thread_nbr) {
            threads.create_thread(std::bind(&doWork, std::ref(context), std::ref(data_manager)));
        }
        threads.create_thread(navitia::MaintenanceWorker(data_manager));

        // Connect work threads to client threads via a queue
        do{
            try{
                zmq::device(ZMQ_QUEUE, clients, workers);
            }catch(zmq::error_t){}//lors d'un SIGHUP on restore la queue
        }while(true);

    }
};
