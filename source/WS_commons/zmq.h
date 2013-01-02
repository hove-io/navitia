#pragma once
namespace webservice {typedef int RequestHandle; /**< Handle de la requête*/}
#include "utils/configuration.h"
#include "type/type.pb.h"
#include <zmq.hpp>
#include <boost/thread.hpp>
#include "data_structures.h"
#include <functional>

#include <string>
#include <iostream>

template<typename Data, typename Worker>
void doWork(zmq::context_t & context, Data & data) {
    zmq::socket_t socket (context, ZMQ_REP);
    socket.connect ("inproc://workers");
    Worker worker(data);
    while (true) {
        // Wait for next request from client
        zmq::message_t request;
        socket.recv (&request);
        pbnavitia::req pb_req;
        pb_req.ParseFromArray(request.data(), request.size());


        webservice::RequestData req;
        req.path = pb_req.api();
        req.raw_params = pb_req.params();
        webservice::ResponseData resp = worker(req, data);
        std::string response_str = resp.response.str();
        // Send reply back to client
        zmq::message_t reply (response_str.size());
        memcpy ((void *) reply.data (),response_str.c_str(), response_str.size());
        socket.send (reply);
    }
}

template<typename Data, typename Worker>
void main_loop(Data & data) {
    boost::thread_group threads;
    // Prepare our context and sockets
    zmq::context_t context (1);
    zmq::socket_t clients (context, ZMQ_ROUTER);
    clients.bind ("ipc:///tmp/diediedie");
    zmq::socket_t workers (context, ZMQ_DEALER);
    workers.bind ("inproc://workers");

    // Launch pool of worker threads
    for (int thread_nbr = 0; thread_nbr < data.nb_threads; ++thread_nbr) {
        threads.create_thread(std::bind(&doWork<Data, Worker>, std::ref(context), std::ref(data)));
    }
    // Connect work threads to client threads via a queue
    zmq::device (ZMQ_QUEUE, clients, workers);
}

#define MAKE_WEBSERVICE(Data, Worker) int main(int argc, char** argv){\
    Configuration * conf = Configuration::get();\
    std::string::size_type posSlash = std::string(argv[0]).find_last_of( "\\/" );\
    conf->set_string("application", std::string(argv[0]).substr(posSlash+1));\
    char buf[256];\
    if(getcwd(buf, 256)) conf->set_string("path",std::string(buf) + "/"); else conf->set_string("path", "unknown");\
    Data d;\
    main_loop<Data,Worker>(d);\
    return 0;\
}

