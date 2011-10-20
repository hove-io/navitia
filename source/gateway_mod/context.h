#pragma once
#include <google/protobuf/message.h>
#include <memory>



/**
 * Classe servant a transmettre des données entre les différents modules de la gateway
 *
 */
struct Context {
    enum Service{
        UNKNOWN,
        BAD_RESPONSE,
        PTREF
    };

    ///la réponse de navitia
    std::unique_ptr<google::protobuf::Message> pb;

    ///flag pour définir le services utilisé
    Service service;


    std::string str;



    Context() : service(UNKNOWN){}
};
