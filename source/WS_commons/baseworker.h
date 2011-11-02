#pragma once
#include <string>
#include <map>
#include <boost/function.hpp>
#include "threadpool.h"
#include <boost/bind.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <vector>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/count.hpp>


#include <boost/lexical_cast.hpp>

namespace webservice
{
    
    void decode(std::string& str){
        size_t pos = -1;
        int code = 0;
        std::stringstream ss;
        std::string number;
        while((pos = str.find("%", pos+1)) != std::string::npos){
            number = str.substr(pos+1, 2);
            if(number.size() < 2){
                continue;
            }
            ss << std::hex << number;
            ss >> code;
            ss.clear();
            if(code < 32){
                continue;
            }
            str.replace(pos, 3, 1, (char)code);
        }

    }

    using namespace boost::accumulators;
    /** Contient des données statiques aux workers */
    struct StaticData {
        typedef accumulator_set<double, features<tag::max, tag::mean, tag::count> > mean_t;
        std::map<std::string, mean_t> means;
    };

    StaticData & static_data() {
        static StaticData ans;
        return ans;
    }

    /** Worker de base. Doit être hérité pour définir le comportement et les données souhaitées pour chaque thread */
    template<class Data>
    class BaseWorker {
    public: // Structures de données

        /** Type d'une fonction qui prend RequestData et Data en paramètre et qui retourne ResponseData */
        typedef boost::function<ResponseData(RequestData, Data & )> ApiFun;

        /** Ensemble des fonctions des apis indexées par leur nom */
        std::map<std::string, ApiFun> apis;

        /** Metadonnées des api indexées par leur nom */
        std::map<std::string, ApiMetadata> api_metadata;
    
    public:
        /** Fonction appelée lorsqu'une requête appelle
      *
      * Cette fonction dispatche ensuite à la bonne en fonction de l'appel
      * Elle découpe également la chaine de requêtes (en POST et GET)
      * et mets le tout dans le dictionnaire request.params
      */
        webservice::ResponseData dispatch(webservice::RequestData request, Data & d){
            decode(request.path);
            std::string raw_params = request.raw_params;
            decode(raw_params);
            decode(request.data);
            std::vector<std::string> tokens;
            boost::algorithm::split(tokens, raw_params, boost::algorithm::is_any_of("&"));
            BOOST_FOREACH(std::string token, tokens) {
                std::vector<std::string> elts;
                boost::algorithm::split(elts, token, boost::algorithm::is_any_of("="));
                if(elts.size() == 1 && elts[0] != "")
                    request.params[boost::algorithm::to_lower_copy(elts[0])] = "";
                else if(elts.size() >= 2 && elts[0] != "")
                    request.params[boost::algorithm::to_lower_copy(elts[0])] = elts[1];
            }

            std::vector<std::string> tokens2;
            boost::algorithm::split(tokens2, request.data, boost::algorithm::is_any_of("&"));
            BOOST_FOREACH(std::string token, tokens2) {
                size_t pos = token.find("=");
                if(pos != std::string::npos && token != "")
                    request.params[boost::algorithm::to_lower_copy(token)] = "";
                else if(token.size() > 0 && token[0] != '=')
                    request.params[boost::algorithm::to_lower_copy(token.substr(0, pos - 1))] = token.substr(pos +1);
            }
            

            size_t position = request.path.find_last_of('/');
            std::string api;
            if(position != std::string::npos){
                api = request.path.substr(position+1);
            }
            request.api = api;


            if(apis.find(api) == apis.end()) {
                ResponseData resp;
                resp.content_type = "text/xml";
                resp.response << "<error>API inconnue</error>";
                return resp;
            }else{
                
                api_metadata[api].parse_parameters(request);

                boost::posix_time::ptime start(boost::posix_time::microsec_clock::local_time());
                ResponseData resp = apis[api](request, d);
                //@TODO not threadsafe
                int duration = (boost::posix_time::microsec_clock::local_time() - start).total_milliseconds();
                static_data().means[api](duration);
                return resp;
            }
        }


     /** Permet de rajouter une nouvelle api
      *
      * Le nom est celui utilisé dans la requête
      * La fonction (idéalement vers un membre du Worker pour profiter des données) va effectuer le traitement
      * La description est pour fournir l'aide à l'utilisateur
      *
      * Si la fonction est une methode membre, utiliser boost::bind(&Classe::methode, this, _1, _2)
      */
        void register_api(const std::string & name, ApiFun fun, const std::string & description){
            apis[name] = fun;

            ApiMetadata api;
            api.description = description;
            api_metadata[name] = api;

        }

        /** Rajoute un paramètre à une api.
      *
      * Retourne false si l'api n'existe pas.
      * Si le paramètre avait déjà été défini, il est écrasé
      */
        bool add_param(const std::string & api, const std::string & name,
                       const std::string & description, enum ApiParameter::Type_p type, bool mandatory, const std::vector<RequestParameter::Parameter_variant>& accepted_values = std::vector<RequestParameter::Parameter_variant>()) {
            if(api_metadata.find(api) == api_metadata.end()) {
                return false;
            }
            ApiParameter param;
            param.description = description;
            param.type = type;
            param.mandatory = mandatory;
            param.accepted_values = accepted_values;
            api_metadata[api].params[name] = param;
            return true;
        }


        /** Fonction appelée à chaque requête */
        ResponseData operator()(const RequestData & request, Data & d){
            return dispatch(request,d);
        }

        // Quelques APIs par défaut
        /** Liste l'ensemble des APIs disponibles avec les paramètres */
        ResponseData help(RequestData, Data &){
            ResponseData rd;
            rd.content_type = "text/html";
            rd.status_code = 200;
            rd.response << "<html><head><title>Liste des API</title></head><body>\n"
                    << "<h1>Liste des APIs</h1>\n";
            BOOST_FOREACH(auto api, api_metadata){
                rd.response << "<h2>" << api.first << "</h2>\n"
                        << "<h3>Description</h3><p>" << api.second.description << "</p>\n"
                        << "<h3>Paramètres</h3><table border=1>"
                        << "<tr><th>Paramètre</th><th>Type</th><th>Description</th><th>Obligatoire</th></tr>\n";
                BOOST_FOREACH(auto param, api.second.params){
                    std::string type_caption;
                    switch(param.second.type){
                    case ApiParameter::STRING: type_caption = "Chaîne de caractères"; break;
                    case ApiParameter::INT: type_caption = "Entier"; break;
                    case ApiParameter::DOUBLE: type_caption = "Réel p.ex. 3.14"; break;
                    case ApiParameter::DATE: type_caption = "Date p.ex. 20111031"; break;
                    case ApiParameter::TIME: type_caption = "Heure p.ex 0918"; break;
                    case ApiParameter::DATETIME: type_caption = "Date-heure p.ex. 20111031T0918"; break;
                    case ApiParameter::BOOLEAN: type_caption = "Booléen 1 pour vrai, 0 pour faux"; break;
                    }

                    rd.response << "<tr><td>" << param.first << "</td><td>" << type_caption << "</td><td>" << param.second.description << "</td><td>" << param.second.mandatory << "</td></tr>\n";
                }
                rd.response << "</table>\n";
            }
            rd.response << "</body></html>";
            return rd;
        }

        /** Affiche de statistiques sur l'utilisation de chaque API */
        ResponseData stats(RequestData, Data &) {
            ResponseData rd;
            rd.content_type = "text/html";
            rd.status_code = 200;
            rd.response << "<html><head><title>Statistiques</title></head><body>\n"
                    << "<h1>Statistiques</h1>";
            BOOST_FOREACH(auto api, apis) {
                rd.response << "<h2>" << api.first << "</h2>\n"
                        << "<p>Temps moyen (ms) : " << mean(static_data().means[api.first]) << "<br/>\n"
                        << "Temps max d'appel (ms) : " << (max)(static_data().means[api.first]) << "<br/>\n"
                        << "Nombre d'appels : " << count(static_data().means[api.first]) << "<br/></p>\n";
            }
            rd.response << "</body></html>";
            return rd;
        }

        /** Analyse les paramêtres d'une requête */
        ResponseData analyze(RequestData request, Data &){
            ResponseData rd;
            rd.content_type = "text/html";
            rd.status_code = 200;
            rd.response << "<html><head><title>Statistiques</title></head><body>\n"
                    << "<h1>Analyse de requête</h1>";

            if(request.params.find("api") == request.params.end()){
                rd.response << "<p>Il faut préciser l'API que vous souhaitez interroger avec api=nom_api</p>"
                            << "</body></html>";
                return rd;
            }

            std::string api = request.params["api"];

            if(apis.find(api) == apis.end()){
                rd.response << "<p>L'API " << api << " est inconnue"
                            << "</body></html>";
                return rd;
            }

            rd.response << "<h2> Paramètres obligatoires </h2>"
                        << "<ul>";

            std::pair<std::string, ApiParameter> ap;
            BOOST_FOREACH(ap, api_metadata[api].params){
                rd.response << "<li><b>" << ap.first << "</b> : ";
                if(ap.second.mandatory){
                    if(request.params.find(ap.first) == request.params.end())
                        rd.response << " Absent ! </li>";
                    else
                        rd.response << " Présent</li>";
                }
            }

            rd.response << "</ul>"
                        << "<h2>Valeurs</h2>"
                        << "<ul>";

            std::pair<std::string, std::string> sp;
            BOOST_FOREACH(sp, request.params){
                if(sp.first != "api"){
                    RequestParameter rp = api_metadata[api].convert_parameter(sp.first, sp.second);
                    rd.response << "<li><b>" << sp.first << "</b> : ";
                    if(rp.used_value){
                        if(rp.valid_value)
                            rd.response << " format valide </li>";
                        else
                            rd.response << " format non valide </li>";
                    }else {
                        rd.response << " paramètre ignoré par l'API </li>";
                    }
                }
            }
            rd.response << "</ul>"
                        << "</body></html>";

            return rd;
        }

        /** Ajoute quelques API par défaut
          *
          * help : donne une aide listant toutes les api disponibles et leurs paramètres
          * status : donne des informations sur le webservice
          */
        void add_default_api() {
            register_api("help", boost::bind(&BaseWorker<Data>::help, this, _1, _2), "Liste des APIs utilisables");
            register_api("stats", boost::bind(&BaseWorker<Data>::stats, this, _1, _2), "Statistiques sur les appels d'api");
            register_api("analyze", boost::bind(&BaseWorker<Data>::analyze, this, _1, _2), "Analyze une requête");
            add_param("analyze", "api", "Nom de l'api à analyser. Rajouter ensuite tous les paramètres habituels", ApiParameter::STRING, true);
        }
    };

}
