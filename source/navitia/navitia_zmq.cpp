#include "worker.h"
#include "maintenance_worker.h"
#include "type/type.pb.h"

#include <zmq.hpp>
#include <boost/thread.hpp>
#include <functional>
#include <string>
#include <iostream>

void doWork(zmq::context_t & context, navitia::type::Data & data) {
    zmq::socket_t socket (context, ZMQ_REP);
    socket.connect ("inproc://workers");
    bool run = true;
    navitia::Worker w(data);
    while(run) {
        zmq::message_t request;
        try{
            // Wait for next request from client
            socket.recv(&request);
        }catch(zmq::error_t){
            //on gére le cas du sighup durant un recv
            continue;
        }

        pbnavitia::Request pb_req;
        pb_req.ParseFromArray(request.data(), request.size());

        auto result = w.dispatch(pb_req);

        zmq::message_t reply(result.ByteSize());
        result.SerializeToArray(reply.data(), result.ByteSize());
        socket.send(reply);
    }
}


int main(int, char** argv){
    Configuration * conf = Configuration::get();
    std::string::size_type posSlash = std::string(argv[0]).find_last_of( "\\/" );
    conf->set_string("application", std::string(argv[0]).substr(posSlash+1));
    char buf[256];
    if(getcwd(buf, 256)) conf->set_string("path",std::string(buf) + "/"); else conf->set_string("path", "unknown");


    navitia::type::Data data;


    boost::thread_group threads;
    // Prepare our context and sockets
    zmq::context_t context(1);
    zmq::socket_t clients(context, ZMQ_ROUTER);
    std::string zmq_socket = conf->get_as<std::string>("GENERAL", "zmq_socket", "ipc:///tmp/default_navitia");
    clients.bind(zmq_socket.c_str());
    zmq::socket_t workers(context, ZMQ_DEALER);
    workers.bind("inproc://workers");

    // Launch pool of worker threads
    for(int thread_nbr = 0; thread_nbr < data.nb_threads; ++thread_nbr) {
        threads.create_thread(std::bind(&doWork, std::ref(context), std::ref(data)));
    }

    threads.create_thread(navitia::MaintenanceWorker(std::ref(data)));

    // Connect work threads to client threads via a queue
    do{
        try{
            zmq::device(ZMQ_QUEUE, clients, workers);
        }catch(zmq::error_t){}//lors d'un SIGHUP on restore la queue
    }while(true);

    return 0;
}

