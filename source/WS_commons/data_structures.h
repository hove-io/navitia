#pragma once

#include <boost/function.hpp>
namespace webservice 
{
    /** Fonction appelée lorsqu'une requête arrive pour la passer au threadpoll */
    static boost::function<void(RequestHandle*)> push_request;

    /** Fonction à appeler pour arrêter le threadpool */
    static boost::function<void()> stop_threadpool;

    /// Types possibles de requètes
    enum RequestMethod {GET, POST, UNKNOWN};

    /** Converti une string de méthode en RequestType */
    RequestMethod parse_method(const std::string & method) {
        if(method == "GET") return GET;
        if(method == "POST") return POST;
        else return UNKNOWN;
    }

    /** Structure contenant toutes les données liées à une requête entrante
     *
     */
    struct RequestData {
        /// Méthode de la requête (ex. GET ou POST)
        RequestMethod method;
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
        /// Encodage de la réponse par défaut utf-8
        std::string charset;
        /// Constructeur par défaut (status 200, type text/plain)
        ResponseData() : content_type("text/plain"), status_code(200), charset("utf-8"){}
    };

}
