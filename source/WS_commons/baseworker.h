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



/** Worker de base. Doit être hérité pour définir le comportement et les données souhaitées pour chaque thread */
template<class Data>
class BaseWorker {
    /// Correspond à l'ensemble des paramètres (clef-valeurs)
    typedef std::map<std::string, std::string> Parameters;

    /// Fonction associée à une api : prend en paramètre un Parameters et retourne une chaîne de caractères correspondant au retour
    typedef boost::function<std::string (Parameters)> ApiFun;

    /// Fonction membre d'un worker associée à une api
    typedef boost::function<std::string (BaseWorker *, Parameters)> ApiMemFun;

    /** Définit un paramètre d'une API
      *
      * Un requête du type foo?param=1&var=hello
      * aura param et var comme paramètres
      */
    struct Parameter {
        std::string description; ///< Description du paramètre (pour information à l'utilisateur)
        std::string type; ///< Type du paramètre (entier, chaîne de caractère)
        bool mandatory; ///< Est-ce que le paramètre est obligatoire
    };

    /** Définit une api
      *
      * Un requête du type foo?param=1&var=hello
      * correspond à l'api foo
      */
    struct Api {
        std::string description; ///< Description de l'api (pour information à l'utilisateur)
        std::map<std::string, Parameter> params; ///< Liste des paramètres de l'api
        ApiFun fun;///< Fonction associée à la requète
    };


    std::map<std::string, Api> apis;
public:
    /** Fonction appelée lorsqu'une requête appelle
      *
      * Cette fonction dispatche ensuite à la bonne en fonction de l'appel
      */
    webservice::ResponseData dispatch(const webservice::RequestData & data, Data d){
        Parameters params;
        std::vector<std::string> tokens;
        boost::algorithm::split(tokens, data.params, boost::algorithm::is_any_of("&"));
        BOOST_FOREACH(std::string token, tokens) {
            std::vector<std::string> elts;
            boost::algorithm::split(elts, token, boost::algorithm::is_any_of("="));
            if(elts.size() == 1)
                params[elts[0]] = 0;
            else
                params[elts[0]] = elts[1];
        }
        return webservice::ResponseData();
    }

    /** Permet de rajouter une nouvelle api
      *
      * Le nom est celui utilisé dans la requête
      * La fonction (idéalement vers un membre du Worker pour profiter des données) va effectuer le traitement
      * La description est pour fournir l'aide à l'utilisateur
      */
    void register_api(const std::string & name, ApiFun fun, const std::string & description){
        Api api;
        api.fun = fun;
        api.description = description;
        apis[name] = api;
    }

    /** Même fonction, sauf qu'on passe une fonction membre en paramètre */
    void register_api(const std::string & name, ApiMemFun mem_fun, const std::string & description){
        ApiFun fun = boost::bind(mem_fun, this, _1);
        register_api(name, fun, description);
    }

    /** Rajoute un paramètre à une api.
      *
      * Retourne false si l'api n'existe pas.
      * Si le paramètre avait déjà été défini, il est écrasé
      */
    bool add_param(const std::string & api, const std::string & name,
                   const std::string & description, const std::string & type, bool mandatory) {
        if(apis.find(api) == apis.end()) {
            return false;
        }
        Parameter param;
        param.description = description;
        param.type = type;
        param.mandatory = mandatory;
        apis[api].params[name] = param;
        return true;
    }

    /** Ajoute quelques API par défaut
      *
      * help : donne une aide listant toutes les api disponibles et leurs paramètres
      * status : donne des informations sur le webservice
      */
    void add_default_api();
};

