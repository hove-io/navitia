#pragma once

#include <boost/function.hpp>
#include <boost/variant.hpp>
#include <string>
#include <sstream>
#include <map>
//#include <threadpool.h>
class RequestHandle;

#include <boost/date_time/posix_time/posix_time.hpp>

#ifdef WIN32
#include <windows.h>
extern HINSTANCE hinstance;
#endif

namespace webservice 
{
    /** Fonction appelée lorsqu'une requête arrive pour la passer au threadpoll */
    static boost::function<void(RequestHandle*)> push_request;

    /** Fonction à appeler pour arrêter le threadpool */
    static boost::function<void()> stop_threadpool;

    /// Types possibles de requètes
    enum RequestMethod {GET, POST, UNKNOWN};

    /// Correspond à l'ensemble des paramètres (clef-valeurs)
    typedef std::map<std::string, std::string> Parameters;

    struct Parameter{
        typedef boost::variant<std::string, int, double, boost::posix_time::ptime> Parameter_variant;
        Parameter_variant value;
        bool is_valid;

    };

    /** Converti une string de méthode en RequestType */
    RequestMethod parse_method(const std::string & method);
    /** Structure contenant toutes les données liées à une requête entrante
     *
     */
    struct RequestData {
        /// Méthode de la requête (ex. GET ou POST)
        RequestMethod method;
        /// Chemin demandé (ex. "/const")
        std::string path;
        /// Paramètres passés à la requête (ex. "user=1&passwd=2")
        std::string raw_params;
        /// Données brutes
        std::string data;
        /// Paramètres parsés
        Parameters params;

        std::map<std::string, Parameter> parsed_params;
        ///API utilisée
        std::string api;

        bool params_is_valid;
    };

    /** Structure contenant les réponses
     *
     */
    struct ResponseData {
        ResponseData(const ResponseData & resp);
        /// Type de la réponse (ex. "text/xml")
        std::string content_type;
        /// Réponse à proprement parler
        std::stringstream response;
        /// Status http de la réponse (ex. 200)
        int status_code;
        /// Encodage de la réponse par défaut utf-8
        std::string charset;
        /// Constructeur par défaut (status 200, type text/plain)
        ResponseData();
    };


}
