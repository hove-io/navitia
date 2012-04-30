#pragma once

/** PTReferential permet d'explorer les données en effectuant des requêtes en pseudo-SQL
  *
  * On peut choisir quels attributs on souhaite afficher et filtrer selon des éléments externes
  * Par exemple on veut vouloir les StopArea et la ville qui sont traversés par la ligne 42
  *
  * ptref_graph.cpp contient toutes les relations entre entités : par quelles relations on obtient
  * les stoppoints d'une commune
  *
  * ptreferential.h permet d'analyser la requête saisie par l'utilisateur. Le parsage se fait avec
  * boost::spirit. Ce n'est pas la lib la plus simple à apprendre, mais elle est performante et puissante
  *
  * La syntaxe globale est :
  * SELECT table.attribut, ... FROM entité_désirée WHERE table.attribut OPÉRATEUR valeur, ...
  *
  * Le fonctionnement global est :
  * 1) Repertorier le type désiré : après la clause where (il ne peut y en avoir qu'un seul !)
  * 2) Pour chaque clause WHERE, on récupère les indexes de l'entité désirée validant la clause
  * 3) On cherche l'intersection de tous ces indexes
  * 4) On récupère les attributs qui nous intéressent (dans la clause SELECT)
  * 5) On fait un export protobuf
  */

#include "type/data.h"
#include "type/type.pb.h"

#include <google/protobuf/descriptor.h>


using navitia::type::Type_e;
namespace navitia{ namespace ptref{

struct unknown_table{};

google::protobuf::Message* get_message(pbnavitia::PTReferential * row, Type_e type);

/// Exécute une requête sur les données Data
/// Retourne une matrice 2D de chaînes de caractères
pbnavitia::PTReferential query(std::string request, navitia::type::PT_Data & data);

std::string pb2txt(const google::protobuf::Message* response);
std::string pb2xml(pbnavitia::PTReferential& response);


std::vector<Type_e> find_path(Type_e source);


google::protobuf::Message* add_item(google::protobuf::Message* message, const std::string& table);

}} //navitia::ptref
