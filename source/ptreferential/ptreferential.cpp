#include "ptreferential.h"
#include "reflexion.h"
#include "indexes.h"
#include "where.h"

#include <iostream>
#include <fstream>
#include <algorithm>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_lit.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

//TODO: change!
using namespace navitia::type;
using namespace navitia::ptref;

namespace qi = boost::spirit::qi;

// colonne d'un objet
struct Column {
    Type_e table;
    std::string column;
};

BOOST_FUSION_ADAPT_STRUCT(
    Column,
    (Type_e, table)
    (std::string, column)
)

// clause where
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
// Requête
struct Request {
    // les colonnes sélectionnées : dans SELECT
    std::vector<Column> columns;
    // l'objet utilisé pour la sélection : dans FROM
    Type_e requested_type;
    // les conditions de sélection : clause WHERE
    std::vector<WhereClause> clauses;
};

BOOST_FUSION_ADAPT_STRUCT(
    Request,
    (std::vector<Column>, columns)
    (Type_e, requested_type)
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
    qi::rule<Iterator, Type_e(), qi::space_type> from; // Matche la section FROM
    qi::rule<Iterator, Request(), qi::space_type> request; // Toute la requête
    qi::rule<Iterator, std::vector<WhereClause>(), qi::space_type> where;// section Where
    qi::rule<Iterator, Operator_e(), qi::space_type> bin_op; // Match une operator binaire telle que <, =...

    select_r() : select_r::base_type(request) {
        txt %= qi::lexeme[+(qi::alnum|'_'|'|'|':'|'-')]; // Match du texte


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

        from %= qi::lexeme["from"] >> table ;

        where %= qi::lexeme["where"] >> (table_col >> bin_op >> txt) % qi::lexeme["and"];

        request %= select >> from >> -where; // Clause where optionnelle
    }

};



namespace navitia{ namespace ptref{

// vérification d'une clause WHERE
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
        else if(clause.col.column == "name")
            wh = wh && WHERE(ptr_name<T>(), clause.op, clause.value);
        }
    return wh;
}


template<class T>
void set_value(google::protobuf::Message* message, const T& object, const std::string& column){
    const google::protobuf::Reflection* reflection = message->GetReflection();
    const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
    const google::protobuf::FieldDescriptor* field_descriptor = descriptor->FindFieldByName(column);

    //std::cout << column << std::endl;
    if(field_descriptor == NULL){
        //throw unknown_member();
        throw PTRefException("Le paramètre field_descriptor est indéfinit");
    }

    if(field_descriptor->type() == google::protobuf::FieldDescriptor::TYPE_STRING){
        reflection->SetString(message, field_descriptor, boost::get<std::string>(get_value(object, column)));
    }else if(field_descriptor->type() == google::protobuf::FieldDescriptor::TYPE_INT32){
        reflection->SetInt32(message, field_descriptor, boost::get<int>(get_value(object, column)));
    }else{
        //throw bad_type();
        throw PTRefException("Le paramètre field_descriptor->type est indéfinit");
    }
}


pbnavitia::Response extract_data(PT_Data & data, const Request & r, std::vector<idx_t> & rows) {
    pbnavitia::Response result;
    result.set_requested_api(pbnavitia::PTREFERENTIAL);
    pbnavitia::PTReferential * pb_response = result.mutable_ptref();
    //on reconstruit
    std::map<Type_e, std::vector<std::string> > columns_map;
    BOOST_FOREACH(const Column & col, r.columns){
        columns_map[col.table].push_back(col.column);
    }
 try{
    BOOST_FOREACH(idx_t row, rows){
        // "stop_area"
        //pbnavitia::PTReferential * pb_row = pb_response.add_item();
       google::protobuf::Message* pb_message = get_message(pb_response, r.requested_type);
        std::pair<Type_e, std::vector<std::string> > col;
        std::vector<Type_e> all_paths = find_path(r.requested_type);
        BOOST_FOREACH(col, columns_map){
            std::vector<Type_e> path;
            Type_e current = col.first;
            path.push_back(current);
            if(all_paths[current] == current && current != r.requested_type)
                std::cerr << "Impossible de trouver un chemin de " << static_data::get()->captionByType(r.requested_type)
                    << "->" << static_data::get()->captionByType(current) << std::endl;
            while(all_paths[current] != current){
                current = all_paths[current];
                path.push_back(current);
            }

            std::vector<idx_t> indexes;
            indexes.push_back(row);

            for(size_t i = path.size() - 1; i > 0; --i) {
                indexes = data.get_target_by_source(path[i], path[i-1], indexes);
                std::cout << static_data::get()->captionByType(path[i]) << " -> " << static_data::get()->captionByType(path[i-1]) << std::endl;
            }

            BOOST_FOREACH(idx_t idx, indexes){
                google::protobuf::Message* item;
                if(col.first != r.requested_type){
                    item = add_item(pb_message, static_data::get()->getListNameByType(col.first));
                }else{
                    item = pb_message;
                }
                BOOST_FOREACH(std::string column_name, col.second){
                    switch(col.first){
                        case eLine: set_value(item, data.lines.at(idx), column_name); break;
                        case eValidityPattern: set_value(item, data.validity_patterns.at(idx), column_name); break;
                        case eRoute: set_value(item, data.routes.at(idx), column_name); break;
                        case eVehicleJourney: set_value(item, data.vehicle_journeys.at(idx), column_name); break;
                        case eStopPoint: set_value(item, data.stop_points.at(idx), column_name); break;
                        case eStopArea: set_value(item, data.stop_areas.at(idx), column_name); break;
                        case eStopTime: set_value(item, data.stop_times.at(idx), column_name); break;
                        case eNetwork: set_value(item, data.networks.at(idx), column_name); break;
                        case eMode: set_value(item, data.modes.at(idx), column_name); break;
                        case eModeType: set_value(item, data.mode_types.at(idx), column_name); break;
                        case eCity: set_value(item, data.cities.at(idx), column_name); break;
                        case eConnection: set_value(item, data.connections.at(idx), column_name); break;
                        case eRoutePoint: set_value(item, data.route_points.at(idx), column_name); break;
                        case eDistrict: set_value(item, data.districts.at(idx), column_name); break;
                        case eDepartment: set_value(item, data.departments.at(idx), column_name); break;
                        case eCompany: set_value(item, data.companies.at(idx), column_name); break;
                        case eVehicle: set_value(item, data.vehicles.at(idx), column_name); break;
                        case eCountry: set_value(item, data.countries.at(idx), column_name); break;
                        case eUnknown: break;
                    }
                }
            }
        }
    }
    } catch(navitia::ptref::PTRefException &e){
        pb_response->set_error(e.what());
    }catch(...){
        pb_response->set_error("PTReferential: Impossible d'extraire la réponse ");
    }
    return result;
}

google::protobuf::Message* add_item(google::protobuf::Message* message, const std::string& table){
    const google::protobuf::Reflection* reflection = message->GetReflection();
    const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
    const google::protobuf::FieldDescriptor* ptref_field_descriptor = descriptor->FindFieldByName("child");
    //@TODO  table est au pluriel

    if(ptref_field_descriptor == NULL){
        throw unknown_member();
    }
    google::protobuf::Message* ptref_message = reflection->MutableMessage(message, ptref_field_descriptor);

    const google::protobuf::Descriptor* ptref_descriptor = ptref_message->GetDescriptor();
    const google::protobuf::FieldDescriptor* field_descriptor = ptref_descriptor->FindFieldByName(table);
    const google::protobuf::Reflection* ptref_reflection = ptref_message->GetReflection();
    if(field_descriptor == NULL){
        throw unknown_member();
    }

    if(field_descriptor->is_repeated()){
        return ptref_reflection->AddMessage(ptref_message, field_descriptor);
    }else if(field_descriptor->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE){
        return ptref_reflection->MutableMessage(ptref_message, field_descriptor);
    }else{
        throw bad_type();
    }
}


google::protobuf::Message* get_message(pbnavitia::PTReferential * row, Type_e type){

    const google::protobuf::Reflection * reflection = row->GetReflection();
    const google::protobuf::Descriptor* descriptor = row->GetDescriptor();
    std::string field = static_data::get()->getListNameByType(type);
    const google::protobuf::FieldDescriptor* field_descriptor = descriptor->FindFieldByName(field);
    google::protobuf::Message* message;
    if (reflection == NULL){
        throw PTRefException("Le paramètre reflection est indéfinit");
    }else{
        if (field_descriptor == NULL){
            throw PTRefException("Le paramètre field_descriptor est indéfinit");
        }else{
            message = reflection->AddMessage(row, field_descriptor);
        }
    }

    //google::protobuf::Message* message = reflection->AddMessage(row, field_descriptor);
    return message;


}

template<Type_e E>
std::vector<idx_t> get_indexes(std::vector<WhereClause> clauses,  Type_e requested_type, PT_Data & d)
{
    typedef typename boost::mpl::at<enum_type_map, boost::mpl::int_<E> >::type T;
    auto data = d.get_data<E>();

    Index<T> filtered(data, build_clause<T>(clauses));
    auto offsets = filtered.get_offsets();
    std::vector<idx_t> indexes;
    indexes.reserve(filtered.size());
    BOOST_FOREACH(auto item, offsets){
        indexes.push_back(item);
    }

    if(clauses.size() == 0)
        return indexes;

    Type_e current = clauses[0].col.table;
    std::vector<Type_e> path = find_path(requested_type);
    while(path[current] != current){
        indexes = d.get_target_by_source(current, path[current], indexes);
        std::cout << static_data::get()->captionByType(current) << " -> " << static_data::get()->captionByType(path[current]) << std::endl;
        current = path[current];
    }
    return indexes;
}

std::vector<idx_t> get(Type_e source, Type_e destination, idx_t source_idx, PT_Data & d){
    std::vector<Type_e> tree = find_path(source);
    std::vector<Type_e> path;

    Type_e current = destination;
    while(tree[current] != current){
        path.push_back(current);
    }

    std::vector<idx_t> indexes = d.get_target_by_one_source(source, path[source], source_idx);
    for(size_t i = path.size() - 1; i > 0; --i){
        indexes = d.get_target_by_source(path[i], path[i-1], indexes);
    }

    return indexes;
}



pbnavitia::Response query(std::string request, PT_Data & data){
    std::string::iterator begin = request.begin();
    Request r;
    pbnavitia::Response pb_response;
    pb_response.set_requested_api(pbnavitia::PTREFERENTIAL);
    pbnavitia::PTReferential * pb_ptref = pb_response.mutable_ptref();
    select_r<std::string::iterator> s;

    // récupération des colonnes sélectionnées, les objets utilisés et les colonnes dans la clause WHERE
    if (qi::phrase_parse(begin, request.end(), s, qi::space, r))
    {
        if(begin != request.end()) {
            std::string unparsed(begin, request.end());
            pb_ptref->set_error("PTReferential : On n'a pas réussi à parser toute la requête. Non-interprété : >>" + unparsed + "<<");
            return pb_response;
        }
    }
    else{                
        pb_ptref->set_error("PTReferential : Impossible de parser la requête");
        return pb_response;
    }



    std::cout << "Requested Type: " << static_data::get()->captionByType(r.requested_type) << std::endl;

    // On regroupe les clauses par tables sur lequelles elles vont taper
    // Ça va se compliquer le jour où l'on voudra gérer des and/or intermélangés...
    std::map<Type_e, std::vector<WhereClause> > clauses;
    BOOST_FOREACH(WhereClause clause, r.clauses){
        clauses[clause.col.table].push_back(clause);
    }

    std::pair<Type_e, std::vector<WhereClause> > type_clauses;
    std::vector<idx_t> final_indexes = data.get_all_index(r.requested_type);
    std::vector<idx_t> indexes;
    BOOST_FOREACH(type_clauses, clauses){
        switch(type_clauses.first){
        case eLine: indexes = get_indexes<eLine>(type_clauses.second, r.requested_type,data); break;
        case eValidityPattern: indexes = get_indexes<eValidityPattern>(type_clauses.second, r.requested_type, data); break;
        case eRoute: indexes = get_indexes<eRoute>(type_clauses.second, r.requested_type, data); break;
        case eVehicleJourney: indexes = get_indexes<eVehicleJourney>(type_clauses.second, r.requested_type, data); break;
        case eStopPoint: indexes = get_indexes<eStopPoint>(type_clauses.second, r.requested_type, data); break;
        case eStopArea: indexes = get_indexes<eStopArea>(type_clauses.second, r.requested_type, data); break;
        case eStopTime: indexes = get_indexes<eStopTime>(type_clauses.second, r.requested_type, data); break;
        case eNetwork: indexes = get_indexes<eNetwork>(type_clauses.second, r.requested_type, data); break;
        case eMode: indexes = get_indexes<eMode>(type_clauses.second, r.requested_type, data); break;
        case eModeType: indexes = get_indexes<eModeType>(type_clauses.second, r.requested_type, data); break;
        case eCity: indexes = get_indexes<eCity>(type_clauses.second, r.requested_type, data); break;
        case eConnection: indexes = get_indexes<eConnection>(type_clauses.second, r.requested_type, data); break;
        case eRoutePoint: indexes = get_indexes<eRoutePoint>(type_clauses.second, r.requested_type, data); break;
        case eDistrict: indexes = get_indexes<eDistrict>(type_clauses.second, r.requested_type, data); break;
        case eDepartment: indexes = get_indexes<eDepartment>(type_clauses.second, r.requested_type, data); break;
        case eCompany: indexes = get_indexes<eCompany>(type_clauses.second, r.requested_type, data); break;
        case eVehicle: indexes = get_indexes<eVehicle>(type_clauses.second, r.requested_type, data); break;
        case eCountry: indexes = get_indexes<eCountry>(type_clauses.second, r.requested_type, data); break;
        case eUnknown: break;
        }
        // Attention ! les structures doivent être triées !
        std::vector<idx_t> tmp_indexes;
        std::back_insert_iterator< std::vector<idx_t> > it(tmp_indexes);
        std::set_intersection(final_indexes.begin(), final_indexes.end(), indexes.begin(), indexes.end(), it);
        final_indexes = tmp_indexes;
    }

    return extract_data(data, r, final_indexes);
}


}} // navitia::ptref
