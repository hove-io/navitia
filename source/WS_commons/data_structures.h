#pragma once

#include <boost/function.hpp>
#include <boost/variant.hpp>
#include <string>
#include <map>

class RequestHandle;

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

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

/** Définit le type d'un paramètre REST*/
struct RequestParameter{
    typedef boost::variant<std::string, int, double, boost::posix_time::ptime, boost::gregorian::date, std::vector<std::string> > Parameter_variant;
    Parameter_variant value;
    /// Est-ce que la valeur est valide (convertible dans le bon type, dans le bon ensemble de valeurs…)
    bool valid_value;
    /// Est-ce que la valeur est utilisée ? Il s'agirait typiquemetn d'une erreur de frappe
    bool used_value;

    RequestParameter();
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
    /// API qui a été appelée
    std::string api;
    /// Constructeur par défaut (status 200, type text/plain)
    ResponseData();
};


/** Définit un paramètre d'une API
  *
  * Un requête du type foo?param=1&var=hello
  * aura param et var comme paramètres
  */
struct ApiParameter {

    enum Type_p {
        STRING,
        INT,
        DOUBLE,
        DATE,
        TIME,
        DATETIME,
        BOOLEAN,
        STRINGLIST
    };

    std::string description; ///< Description du paramètre (pour information à l'utilisateur)
    Type_p type; ///< Type du paramètre (entier, chaîne de caractère)
    bool mandatory; ///< Est-ce que le paramètre est obligatoire
    ///liste exaustive des valeurs attendu
    std::vector<RequestParameter::Parameter_variant> accepted_values;
};


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

    /// Liste des paramètres REST parsés
    std::map<std::string, RequestParameter> parsed_params;

    /// Liste des paramètres REST obligatoires qui manquent
    std::vector<std::string> missing_params;

    /// API utilisée
    std::string api;

    /** Est-ce que les paramètres sont valide
          *
          * Le webservice doit décider de l'erreur à retourner si ceux-ci ne le sont pas
          */
    bool params_are_valid;

    RequestData();
};


/** Méta-données d'une api : paramètres obligatoires, validation des types…
  *
  * Un requête du type foo?param=1&var=hello
  * correspond à l'api foo
  */
struct ApiMetadata {
    std::string description; ///< Description de l'api (pour information à l'utilisateur)
    std::map<std::string, ApiParameter> params; ///< Liste des paramètres de l'api

    /** Analyse un paramètre et le converti au besoin dans le bon type */
    RequestParameter convert_parameter(const std::string & key, const std::string & value) const;

    /// On vérifie que tous les paramètres obligatoires sont définis
    void check_manadatory_parameters(RequestData& request);

    /// Pour chaque paramètre, on le converti dans son type et on vérifie la validité de la valeur
    void parse_parameters(RequestData& request);
};

/** Converti une string de méthode en RequestType */
RequestMethod parse_method(const std::string & method);

///visitor pour l'affichage de ParameterVariant 
struct PrintParameterVisitor : public boost::static_visitor<>
{
    std::ostream& stream;

    PrintParameterVisitor(std::ostream& stream): stream(stream){}

    template<typename T>
    void operator()(const T& value) const{
        stream << value;
    }
};

template<>
void PrintParameterVisitor::operator()(const std::vector<std::string> & vec) const;

}
