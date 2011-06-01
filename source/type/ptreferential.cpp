#include "ptreferential.h"

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_lit.hpp>

#include <boost/fusion/include/adapt_struct.hpp>
#include <iostream>
#include "indexes2.h"
#include "where.h"
#include "reflexion.h"
#include "proto/type.pb.h"

#include <google/protobuf/descriptor.h>


namespace qi = boost::spirit::qi;
using namespace navitia::type;
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


std::string unpluralize_table(const std::string& table_name){
    if(table_name == "cities"){
        return "city";
    }else{
        return table_name.substr(0, table_name.length() - 1);
    }
}

template<class T>
void set_value(google::protobuf::Message* message, const T& object, const std::string& column){
    const google::protobuf::Reflection* reflection = message->GetReflection();
    const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
    const google::protobuf::FieldDescriptor* field_descriptor = descriptor->FindFieldByName(column);

    if(field_descriptor == NULL){
        throw unknown_member();
    }

    if(field_descriptor->type() == google::protobuf::FieldDescriptor::TYPE_STRING){
        reflection->SetString(message, field_descriptor, get_string_value(object, column));
    }else if(field_descriptor->type() == google::protobuf::FieldDescriptor::TYPE_INT32){
        reflection->SetInt32(message, field_descriptor, get_int_value(object, column));
    }else{
        throw bad_type();
    }
}

google::protobuf::Message* get_message(pbnavitia::PTreferential* row, const std::string& table){
    const google::protobuf::Reflection* reflection = row->GetReflection();
    const google::protobuf::Descriptor* descriptor = row->GetDescriptor();
    std::string field = unpluralize_table(table);
    const google::protobuf::FieldDescriptor* field_descriptor = descriptor->FindFieldByName(field);
    return reflection->MutableMessage(row, field_descriptor);
}

template<class T>
std::vector< std::vector<col_t> > extract_data(std::vector<T> & rows, const Request & r) {
    pbnavitia::PTRefResponse pb_response;

    std::vector< std::vector<col_t> > result;
    Index2<boost::fusion::vector<T> > filtered(rows, build_clause<T>(r.clauses));
    BOOST_FOREACH(auto item, filtered){
        std::vector<col_t> row;
        pbnavitia::PTreferential * pb_row = pb_response.add_item();

        google::protobuf::Message* pb_message = get_message(pb_row, r.tables.at(0));


        BOOST_FOREACH(const Column & col, r.columns){
            row.push_back(get_value(*(boost::fusion::at_c<0>(item)), col.column));
            set_value(pb_message, *(boost::fusion::at_c<0>(item)), col.column);
        }
        result.push_back(row);
    }
    std::cout << "J'ai généré un protocol buffer de taille " <<     pb_response.ByteSize() << std::endl;
    //std::cout << pb_response.SerializeToOstream(&std::cout) << std::endl;
    return result;
}

struct unknown_table{};
std::vector< std::vector<col_t> > query(std::string request, Data & data){
    std::vector< std::vector<col_t> > result;
    std::string::iterator begin = request.begin();
    Request r;
    select_r<std::string::iterator> s;
    if (qi::phrase_parse(begin, request.end(), s, qi::space, r))
    {
        if(begin != request.end()) {
            std::cout << "Hrrrmmm on a pas tout parsé -_-'" << std::endl;
        }
    }
    else
        std::cout << "Parsage a échoué" << std::endl;

    if(r.tables.size() != 1){
        std::cout << "Pour l'instant on ne supporte que exactement une table" << std::endl;
        return result;
    }
    else {
        std::cout << "Table : " << r.tables[0] << std::endl;
    }

    std::string table = r.tables[0];

    /*if(table == "validity_pattern") {
        return extract_data(data.validity_patterns, r);
    }
    else*/ if(table == "lines") {
        return extract_data(data.lines, r);
    }
    else if(table == "routes") {
        return extract_data(data.routes, r);
    }
    /*else if(table == "vehicle_journey") {
        return extract_data(data.vehicle_journeys, r);
    }*/
    else if(table == "stop_points") {
        return extract_data(data.stop_points, r);
    }
    else if(table == "stop_areas") {
        return extract_data(data.stop_areas, r);
    }
  /*  else if(table == "stop_times"){
        return extract_data(data.stop_times, r);
    }*/
    
    throw unknown_table();
}


int main(int argc, char** argv){
    std::cout << "Chargement des données..." << std::flush;
    Data d;
    d.load_flz("data.nav.flz");
    std::cout << " effectué" << std::endl << std::endl;

    std::cout
            << "Statistiques :" << std::endl
            << "    Nombre de StopAreas : " << d.stop_areas.size() << std::endl
            << "    Nombre de StopPoints : " << d.stop_points.size() << std::endl
            << "    Nombre de lignes : " << d.lines.size() << std::endl
            << "    Nombre d'horaires : " << d.stop_times.size() << std::endl << std::endl;


    if(argc != 2)
        std::cout << "Il faut exactement un paramètre" << std::endl;
    else {
        auto result = query(argv[1], d);
        std::cout << "Il y a " << result.size() << " lignes de résultat" << std::endl;
        BOOST_FOREACH(auto row, result){
            BOOST_FOREACH(auto col, row){
                std::cout << col << ",\t";
            }
            std::cout << std::endl;
        }
    }
    return 0;
}
