#pragma once

#include <boost/function.hpp>
namespace webservice 
{
    /** Fonction appelée lorsqu'une requête arrive pour la passer au threadpoll */
    static boost::function<void(RequestHandle*)> push_request;

    /** Fonction à appeler pour arrêter le threadpool */
    static boost::function<void()> stop_threadpool;

    /// Types possibles de requètes
    enum RequestType {GET, POST};

    /** Structure contenant toutes les données liées à une requête entrante
     *
     */
    struct RequestData {
        /// Type de la requête (ex. GET ou POST)
        RequestType type;
        /// Chemin demandé (ex. "/const")
        std::string path;
        /// Paramètres passés à la requête (ex. "user=1&passwd=2")
        std::string params;
        /// Données brutes
        std::string data;
    };

    /** Structure contenant les réponses
     *
     */
    struct ResponseData {
        /// Type de la réponse (ex. "text/xml")
        std::string content_type;
        /// Réponse à proprement parler
        std::string response;
        /// Status http de la réponse (ex. 200)
        int status_code;
        /// Constructeur par défaut (status 200, type text/plain)
        ResponseData() : content_type("text/plain"), status_code(200){}
    };

};
