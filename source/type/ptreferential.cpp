#include "ptreferential.h"

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_lit.hpp>

#include <boost/fusion/include/adapt_struct.hpp>
#include <iostream>
#include "indexes2.h"

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

template<class T>
Where<T> build_clause(std::string & member) {
    Where<T> wh;
    boost::variant<int T::*, double T::*, std::string T::*> ptr = T::get2(member);
    BOOST_FOREACH(auto clause, r.clauses) {
        if(clause.op == "=")
            wh = wh && WHERE(&T::idx, EQ, clause.value);
        else if(clause.op == "<")
            wh = wh && WHERE(&T::idx, LT, clause.value);
        else if(clause.op == "<=")
            wh = wh && WHERE(&T::idx, LEQ, clause.value);
        else if(clause.op == ">")
            wh = wh && WHERE(&T::idx, GT, clause.value);
        else if(clause.op == ">=")
            wh = wh && WHERE(&T::idx, GEQ, clause.value);
        else if(clause.op == "<>")
            wh = wh && WHERE(&T::idx, NEQ, clause.value);
        }
    }
}

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
        std::cout << "Columns : ";
        BOOST_FOREACH(Column & col, r.columns)
                std::cout << col.table << ":" << col.column << " ";
        std::cout << std::endl;

        std::cout << "Clauses where : ";
        BOOST_FOREACH(WhereClause & w, r.clauses)
                std::cout << w.col.column << w.op << w.value << " ";
        std::cout << std::endl;

        if(r.tables.size() != 1){
            std::cout << "Pour l'instant on ne supporte que exactement une table" << std::endl;
            return result;
        }
        else {
            std::cout << "Table : " << r.tables[0] << std::endl;
        }



    std::string table = r.tables[0];
    if(table == "validity_pattern") {
        BOOST_FOREACH(const ValidityPattern & vp, data.validity_patterns){
            std::vector<col_t> row;
            BOOST_FOREACH(Column & col, r.columns)
               row.push_back(vp.get(col.column)); 
            result.push_back(row);
        }
    }
    else if(table == "lines") {
        BOOST_FOREACH(const Line & line, data.lines){
            std::vector<col_t> row;
            BOOST_FOREACH(Column & col, r.columns)
               row.push_back(line.get(col.column)); 
            result.push_back(row);
        }
    }
    else if(table == "routes") {
        BOOST_FOREACH(const Route & route, data.routes){
            std::vector<col_t> row;
            BOOST_FOREACH(Column & col, r.columns)
               row.push_back(route.get(col.column)); 
            result.push_back(row);
        }
    }
    else if(table == "vehicle_journey") {
        BOOST_FOREACH(const VehicleJourney & vj, data.vehicle_journeys){
            std::vector<col_t> row;
            BOOST_FOREACH(Column & col, r.columns)
               row.push_back(vj.get(col.column)); 
            result.push_back(row);
        }
    }
    else if(table == "stop_points") {
        BOOST_FOREACH(const StopPoint & sp, data.stop_points){
            std::vector<col_t> row;
            BOOST_FOREACH(Column & col, r.columns)
               row.push_back(sp.get(col.column)); 
            result.push_back(row);
        }
    }
    else if(table == "stop_areas") {


            BOOST_FOREACH(auto vect, Index2<boost::fusion::vector<StopArea> >(data.stop_areas, wh)){
                const StopArea & sa = *(boost::fusion::at_c<0>(vect));
            std::vector<col_t> row;
            BOOST_FOREACH(Column & col, r.columns)
               row.push_back(sa.get(col.column)); 
            result.push_back(row);
        }
    }
    else if(table == "stop_times"){
        BOOST_FOREACH(const StopArea & sa, data.stop_areas){
            std::vector<col_t> row;
            BOOST_FOREACH(Column & col, r.columns)
               row.push_back(sa.get(col.column)); 
            result.push_back(row);
        }
    }
    
    return result;
}


int main(int argc, char** argv){
    Data d;
    d.load_bin("data.nav");
/*    Index2<boost::fusion::vector<StopArea> > x2(d.stop_areas, WHERE(&StopArea::idx, GT, 504) && WHERE(&StopArea::idx, LET, 505));
    std::cout << x2.nb_types() << " " << x2.size() << std::endl;

    BOOST_FOREACH(auto bleh, x2) {
        std::cout << boost::fusion::at_c<0>(bleh)->name << std::endl;
    }*/

    if(argc != 2)
        std::cout << "Il faut exactement un paramètre" << std::endl;
    else {
        auto result = query(argv[1], d);
        std::cout << "Il y a " << result.size() << " lignes" << std::endl;
        BOOST_FOREACH(auto row, result){
            BOOST_FOREACH(auto col, row){
                std::cout << col << ",\t";
            }
            std::cout << std::endl;
        }
    }
    return 0;
}
