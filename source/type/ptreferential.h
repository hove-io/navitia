#pragma once

#include "data.h"

#include "type.pb.h"

#include <google/protobuf/descriptor.h>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_lit.hpp>

#include <boost/fusion/include/adapt_struct.hpp>

#include "indexes2.h"
#include "where.h"
#include "reflexion.h"
#include "type.pb.h"


//TODO: change!
using namespace navitia::type;

namespace qi = boost::spirit::qi;


struct Column {
    std::string table;
    std::string column;
};

BOOST_FUSION_ADAPT_STRUCT(
    Column,
    (std::string, table)
    (std::string, column)
)

struct WhereClause{
    Column col;
    std::string op;
    std::string value;
}; 

BOOST_FUSION_ADAPT_STRUCT(
    WhereClause,
    (Column,col)
    (std::string, op)
    (std::string, value)
)

struct Request {
    std::vector<Column> columns;
    std::vector<std::string> tables;
    std::vector<WhereClause> clauses;
};

BOOST_FUSION_ADAPT_STRUCT(
    Request,
    (std::vector<Column>, columns)
    (std::vector<std::string>, tables)
    (std::vector<WhereClause>, clauses)
)
        template <typename Iterator>
        struct select_r
            : qi::grammar<Iterator, Request(), qi::space_type>
{
    qi::rule<Iterator, std::string(), qi::space_type> txt; // Match une string
    qi::rule<Iterator, Column(), qi::space_type> table_col; // Match une colonne
    qi::rule<Iterator, std::vector<Column>(), qi::space_type> select; // Match toute la section SELECT
    qi::rule<Iterator, std::vector<std::string>(), qi::space_type> from; // Matche la section FROM
    qi::rule<Iterator, Request(), qi::space_type> request; // Toute la requête
    qi::rule<Iterator, std::vector<WhereClause>(), qi::space_type> where;// section Where 
    qi::rule<Iterator, std::string(), qi::space_type> bin_op; // Match une operator binaire telle que <, =...

    select_r() : select_r::base_type(request) {
        txt %= qi::lexeme[+(qi::alnum|'_')]; // Match du texte
        bin_op %=  qi::string("<=") | qi::string(">=") | qi::string("<>") | qi::string("<")  | qi::string(">") | qi::string("=") ;
        table_col %= -(txt >> '.') // (Nom de la table suivit de point) optionnel (operateur -)
                     >> txt; // Nom de la table obligatoire
        select  %= qi::lexeme["select"] >> table_col % ',' ; // La liste de table_col séparée par des ,
        from %= qi::lexeme["from"] >> txt % ',';
        where %= qi::lexeme["where"] >> (table_col >> bin_op >> txt) % qi::lexeme["and"];
        request %= select >> from >> -where;
    }

};

namespace navitia{ namespace ptref{

struct unknown_table{};

std::string unpluralize_table(const std::string& table_name);
google::protobuf::Message* get_message(pbnavitia::PTreferential* row, const std::string& table);

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
    Operator_e op;
    BOOST_FOREACH(auto clause, clauses) {
        if(clause.op == "=") op = EQ;
        else if(clause.op == "<") op = LT;
        else if(clause.op == "<=") op = LEQ;
        else if(clause.op == ">") op = GT;
        else if(clause.op == ">=") op = GEQ;
        else if(clause.op == "<>") op = NEQ;
        else throw "grrr";

        if(clause.col.column == "id")
            wh = wh && WHERE(ptr_id<T>(), op, clause.value);
        else if(clause.col.column == "idx")
            wh = wh && WHERE(ptr_idx<T>(), op, clause.value);
        else if(clause.col.column == "external_code")
            wh = wh && WHERE(ptr_external_code<T>(), op, clause.value);
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


}} //navitia::ptref
