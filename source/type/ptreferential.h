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

#include "data.h"

#include "type.pb.h"

#include <google/protobuf/descriptor.h>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include <boost/graph/adjacency_list.hpp>
#include "indexes2.h"
#include "where.h"
#include "reflexion.h"
#include "type.pb.h"


//TODO: change!
using namespace navitia::type;
using namespace navitia::ptref;

namespace qi = boost::spirit::qi;


struct Column {
    Type_e table;
    std::string column;
};

BOOST_FUSION_ADAPT_STRUCT(
    Column,
    (Type_e, table)
    (std::string, column)
)

struct WhereClause{
    Column col;
    Operator_e op;
    std::string value;
}; 

BOOST_FUSION_ADAPT_STRUCT(
    WhereClause,
    (Column,col)
    (Operator_e, op)
    (std::string, value)
)

struct Request {
    std::vector<Column> columns;
    std::vector<Type_e> tables;
    std::vector<WhereClause> clauses;
};

BOOST_FUSION_ADAPT_STRUCT(
    Request,
    (std::vector<Column>, columns)
    (std::vector<Type_e>, tables)
    (std::vector<WhereClause>, clauses)
)

        template <typename Iterator>
        struct select_r
            : qi::grammar<Iterator, Request(), qi::space_type>
{
    qi::rule<Iterator, std::string(), qi::space_type> txt; // Match une string
    qi::rule<Iterator, Type_e(), qi::space_type> table;
    qi::rule<Iterator, Column(), qi::space_type> table_col; // Match une colonne
    qi::rule<Iterator, std::vector<Column>(), qi::space_type> select; // Match toute la section SELECT
    qi::rule<Iterator, std::vector<Type_e>(), qi::space_type> from; // Matche la section FROM
    qi::rule<Iterator, Request(), qi::space_type> request; // Toute la requête
    qi::rule<Iterator, std::vector<WhereClause>(), qi::space_type> where;// section Where 
    qi::rule<Iterator, Operator_e(), qi::space_type> bin_op; // Match une operator binaire telle que <, =...

    select_r() : select_r::base_type(request) {
        txt %= qi::lexeme[+(qi::alnum|'_')]; // Match du texte

        table =   qi::string("stop_areas")[qi::_val = eStopArea]
                | qi::string("stop_points")[qi::_val = eStopPoint]
                | qi::string("lines")[qi::_val = eLine]
                | qi::string("routes")[qi::_val = eRoute]
                | qi::string("validity_patterns")[qi::_val = eValidityPattern]
                | qi::string("vehicle_journeys")[qi::_val = eVehicleJourney]
                | qi::string("stop_times")[qi::_val = eStopTime]
                | qi::string("networks")[qi::_val = eNetwork]
                | qi::string("modes")[qi::_val = eMode]
                | qi::string("mode_types")[qi::_val = eModeType]
                | qi::string("cities")[qi::_val = eCity]
                | qi::string("connections")[qi::_val = eConnection]
                | qi::string("route_points")[qi::_val = eRoutePoint]
                | qi::string("districts")[qi::_val = eDistrict]
                | qi::string("departments")[qi::_val = eDepartment]
                | qi::string("companies")[qi::_val = eCompany]
                | qi::string("vehicles")[qi::_val = eVehicle]
                | qi::string("countries")[qi::_val = eCountry];

        bin_op =  qi::string("<=")[qi::_val = LEQ]
                | qi::string(">=")[qi::_val = GEQ]
                | qi::string("<>")[qi::_val = NEQ]
                | qi::string("<") [qi::_val = LT]
                | qi::string(">") [qi::_val = GT]
                | qi::string("=") [qi::_val = EQ];

        table_col %= table >> '.' >> txt; // Nom de la table suivit de point et Nom de la colonne obligatoire

        select  %= qi::lexeme["select"] >> table_col % ',' ; // La liste de table_col séparée par des ,

        from %= qi::lexeme["from"] >> table % ',';

        where %= qi::lexeme["where"] >> (table_col >> bin_op >> txt) % qi::lexeme["and"];

        request %= select >> from >> -where; // Clause where optionnelle
    }

};

namespace navitia{ namespace ptref{

struct unknown_table{};

google::protobuf::Message* get_message(pbnavitia::PTreferential* row, Type_e type);

/// Exécute une requête sur les données Data
/// Retourne une matrice 2D de chaînes de caractères
pbnavitia::PTRefResponse query(std::string request, navitia::type::Data & data);

std::string pb2txt(pbnavitia::PTRefResponse& response);
std::string pb2xml(pbnavitia::PTRefResponse& response);

template<class T>
void set_value(google::protobuf::Message* message, const T& object, const std::string& column){
    const google::protobuf::Reflection* reflection = message->GetReflection();
    const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
    const google::protobuf::FieldDescriptor* field_descriptor = descriptor->FindFieldByName(column);

    if(field_descriptor == NULL){
        throw unknown_member();
    }

    if(field_descriptor->type() == google::protobuf::FieldDescriptor::TYPE_STRING){
        reflection->SetString(message, field_descriptor, boost::get<std::string>(get_value(object, column)));
    }else if(field_descriptor->type() == google::protobuf::FieldDescriptor::TYPE_INT32){
        reflection->SetInt32(message, field_descriptor, boost::get<int>(get_value(object, column)));
    }else{
        throw bad_type();
    }
}

template<class T>
WhereWrapper<T> build_clause(std::vector<WhereClause> clauses) {
    WhereWrapper<T> wh(new BaseWhere<T>());
    BOOST_FOREACH(auto clause, clauses) {
        if(clause.col.column == "id")
            wh = wh && WHERE(ptr_id<T>(), clause.op, clause.value);
        else if(clause.col.column == "idx")
            wh = wh && WHERE(ptr_idx<T>(), clause.op, clause.value);
        else if(clause.col.column == "external_code")
            wh = wh && WHERE(ptr_external_code<T>(), clause.op, clause.value);
    }
    return wh;
}



template<class T>
pbnavitia::PTRefResponse extract_data(std::vector<T> & rows, const Request & r) {
    pbnavitia::PTRefResponse pb_response;

    Index2<boost::fusion::vector<T> > filtered(rows, build_clause<T>(r.clauses));
    BOOST_FOREACH(auto item, filtered){
        pbnavitia::PTreferential * pb_row = pb_response.add_item();

        google::protobuf::Message* pb_message = get_message(pb_row, r.tables.at(0));


        BOOST_FOREACH(const Column & col, r.columns){
            set_value(pb_message, *(boost::fusion::at_c<0>(item)), col.column);
        }
    }
    /*std::ofstream file("response.pb");
    pb_response.SerializeToOstream(&file);*/
    return pb_response;
}

struct Edge {
    float weight;
    Edge() : weight(1){}
};

struct Node {
    Type_e type;
    Node(Type_e type) : type(type) {}
};

/// Contient le graph des transitions
typedef boost::adjacency_list<boost::listS, boost::vecS, boost::directedS, Type_e, Edge > Graph;
typedef boost::graph_traits<Graph>::vertex_descriptor vertex_t;
typedef boost::graph_traits<Graph>::edge_descriptor edge_t;

struct Jointures {
    std::map<Type_e, vertex_t> vertex_map;
    Graph g;

    Jointures();

    std::vector<Type_e> find_path(Type_e source);

};

}} //navitia::ptref
