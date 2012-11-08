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
namespace pbnavitia { struct Response;}

using navitia::type::Type_e;
namespace navitia{ namespace ptref{

struct ptref_parsing_error : std::exception{
    enum error_type {
        global_error = 1,
        partial_error = 2
    };

    error_type type;
    std::string more;
};

struct ptref_unknown_object : std::exception {
    std::string more;
};

/// Exécute une requête sur les données Data
std::vector<type::idx_t> make_query(type::Type_e requested_type, std::string request, type::Data & data);
std::vector<type::idx_t> query_idx(type::Type_e requested_type, std::string request, type::Data & data);
pbnavitia::Response query_pb(type::Type_e type, std::string request, type::Data & data);

/// Trouve le chemin d'un type de données à un autre
/// Par exemple StopArea → StopPoint → RoutePoint
std::map<Type_e,Type_e> find_path(Type_e source);

/// À parti d'un élément, on veut retrouver tous ceux de destination
std::vector<type::idx_t> get(Type_e source, Type_e destination, type::idx_t source_idx, type::PT_Data & data);

}} //navitia::ptref
