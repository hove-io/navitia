#pragma once

/** PTReferential permet d'explorer les données en effectuant des filtre sur tout autre objet TC
  *
  * Par exemple on veut vouloir les StopArea qui sont traversés par la ligne 42
  * On appelle "filtre" toute restriction des objets à retourner
  * On peut faire poser des filtres sur n'importe quel objet
  *
  * ptref_graph.cpp contient toutes les relations entre entités : par quelles relations on obtient
  * les stoppoints d'une commune
  *
  * ptreferential.h permet d'analyser les filtres saisis par l'utilisateur. Le parsage se fait avec
  * boost::spirit. Ce n'est pas la lib la plus simple à apprendre, mais elle est performante et puissante
  *
  * Le fonctionnement global est :
  * 1) Pour chaque filtre on trouve les indexes qui correspondent
  * 2) On fait l'intersection des indexes obtenus par chaque filtre
  * 3) On remplit le protobuf
  */

#include "type/data.h"
#include "where.h"
namespace pbnavitia { struct Response;}



using navitia::type::Type_e;
namespace navitia{ namespace ptref{

// Un filter est du type stop_area.external_code = "kikoolol"
struct Filter {
    navitia::type::Type_e navitia_type; //< Le type parsé
    std::string object; //< L'objet sous forme de chaîne de caractère ("stop_area")
    std::string attribute; //< L'attribu ("external code")
    Operator_e op; //< la comparaison ("=")
    std::string value; //< la valeur comparée ("kikoolol")

    Filter(std::string object, std::string attribute, Operator_e op, std::string value) : object(object), attribute(attribute), op(op), value(value) {}
    Filter(std::string object, std::string value) : object(object), op(HAVING), value(value) {}
    Filter() {}
};


struct ptref_parsing_error : public std::exception{
    enum error_type {
        global_error ,
        partial_error,
        unknown_object
    };

    error_type type;
    std::string more;

    ~ptref_parsing_error() throw() {}
};

/// Execute une requête et génère la sortie protobuf 
pbnavitia::Response query_pb(type::Type_e type, std::string request, const int depth, type::Data & data);

/// Exécute une requête sur les données Data : retourne les idx des objets demandés
std::vector<type::idx_t> make_query(type::Type_e requested_type, std::string request, type::Data & data);

/// Construit la réponse proto buf, une fois que l'on trouvé les indices
pbnavitia::Response extract_data(type::Data & data, Type_e requested_type, std::vector<type::idx_t> & rows, const int depth);


/// Trouve le chemin d'un type de données à un autre
/// Par exemple StopArea → StopPoint → RoutePoint
std::map<Type_e,Type_e> find_path(Type_e source);

/// À parti d'un élément, on veut retrouver tous ceux de destination
std::vector<type::idx_t> get(Type_e source, Type_e destination, type::idx_t source_idx, type::PT_Data & data);


std::vector<Filter> parse(std::string request);
}} //navitia::ptref
