#include "worker.h"
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
    while (run) {
        // Wait for next request from client
        zmq::message_t request;
        socket.recv(&request);
        pbnavitia::Request pb_req;
        pb_req.ParseFromArray(request.data(), request.size());

        auto result = w.dispatch(pb_req);

        zmq::message_t reply(result.ByteSize());
        result.SerializeToArray(reply.data(), result.ByteSize());
        socket.send(reply);
    }
}

void main_loop() {
    navitia::type::Data data;
    navitia::Worker init_worker(data);
    init_worker.load();

    boost::thread_group threads;
    // Prepare our context and sockets
    zmq::context_t context (1);
    zmq::socket_t clients (context, ZMQ_ROUTER);
    clients.bind ("ipc:///tmp/diediedie");
    zmq::socket_t workers (context, ZMQ_DEALER);
    workers.bind ("inproc://workers");

    // Launch pool of worker threads
    for (int thread_nbr = 0; thread_nbr < data.nb_threads; ++thread_nbr) {
        threads.create_thread(std::bind(&doWork, std::ref(context), std::ref(data)));
    }
    // Connect work threads to client threads via a queue
    zmq::device (ZMQ_QUEUE, clients, workers);
}

int main(int argc, char** argv){
    Configuration * conf = Configuration::get();
    std::string::size_type posSlash = std::string(argv[0]).find_last_of( "\\/" );
    conf->set_string("application", std::string(argv[0]).substr(posSlash+1));
    char buf[256];
    if(getcwd(buf, 256)) conf->set_string("path",std::string(buf) + "/"); else conf->set_string("path", "unknown");
    main_loop();
    return 0;
}

