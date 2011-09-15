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
    Type_e requested_type;
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
        txt %= qi::lexeme[+(qi::alnum|'_'|'|'|':')]; // Match du texte

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
pbnavitia::PTRefResponse extract_data(std::vector<T> & table, const Request & r, std::vector<idx_t> & rows) {
    pbnavitia::PTRefResponse pb_response;

    BOOST_FOREACH(idx_t row, rows){
        pbnavitia::PTreferential * pb_row = pb_response.add_item();
        google::protobuf::Message* pb_message = get_message(pb_row, r.requested_type);
        BOOST_FOREACH(const Column & col, r.columns){
            set_value(pb_message, table.at(row), col.column);
        }
    }
    return pb_response;
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
        reflection->SetString(message, field_descriptor, boost::get<std::string>(get_value(object, column)));
    }else if(field_descriptor->type() == google::protobuf::FieldDescriptor::TYPE_INT32){
        reflection->SetInt32(message, field_descriptor, boost::get<int>(get_value(object, column)));
    }else{
        throw bad_type();
    }
}

google::protobuf::Message* get_message(pbnavitia::PTreferential* row, Type_e type){
    const google::protobuf::Reflection* reflection = row->GetReflection();
    const google::protobuf::Descriptor* descriptor = row->GetDescriptor();
    std::string field = static_data::get()->captionByType(type);
    const google::protobuf::FieldDescriptor* field_descriptor = descriptor->FindFieldByName(field);
    return reflection->MutableMessage(row, field_descriptor);
}

template<Type_e E>
std::vector<idx_t> get_indexes(std::vector<WhereClause> clauses,  Type_e requested_type, Data & d)
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

    std::vector<Type_e> path = find_path(requested_type);
    Type_e current = clauses[0].col.table;
    while(path[current] != current){
        indexes = d.get_target_by_source(current, path[current], indexes);
        std::cout << static_data::get()->captionByType(current) << " -> " << static_data::get()->captionByType(path[current]) << std::endl;
        current = path[current];
    }
    return indexes;
}

pbnavitia::PTRefResponse query(std::string request, Data & data){
    std::string::iterator begin = request.begin();
    Request r;
    pbnavitia::PTRefResponse pb_response;
    select_r<std::string::iterator> s;
    if (qi::phrase_parse(begin, request.end(), s, qi::space, r))
    {
        if(begin != request.end()) {
            std::string unparsed(begin, request.end());
            pb_response.set_error("PTReferential : On n'a pas réussi à parser toute la requête. Non-interprété : >>" + unparsed + "<<");
            return pb_response;
        }
    }
    else
        pb_response.set_error("PTReferential : Impossible de parser la requête");


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

    switch(r.requested_type){
    case eValidityPattern: return extract_data(data.validity_patterns, r, final_indexes); break;
    case eLine: return extract_data(data.lines, r, final_indexes); break;
    case eRoute: return extract_data(data.routes, r, final_indexes); break;
    case eVehicleJourney: return extract_data(data.vehicle_journeys, r, final_indexes); break;
    case eStopPoint: return extract_data(data.stop_points, r, final_indexes); break;
    case eStopArea: return extract_data(data.stop_areas, r, final_indexes); break;
    default:  break;
    }

    pb_response.set_error("Table inconnue : " + r.requested_type);
    return pb_response;
}


std::string pb2txt(pbnavitia::PTRefResponse& response){
    std::stringstream buffer;
    for(int i=0; i < response.item_size(); i++){
        pbnavitia::PTreferential* item = response.mutable_item(i);

        const google::protobuf::Reflection* item_reflection = item->GetReflection();
        std::vector<const google::protobuf::FieldDescriptor*> field_list;
        item_reflection->ListFields(*item, &field_list);
        BOOST_FOREACH(const google::protobuf::FieldDescriptor* item_field_descriptor, field_list){
            google::protobuf::Message* object = item_reflection->MutableMessage(item, item_field_descriptor);
            const google::protobuf::Descriptor* descriptor = object->GetDescriptor();
            const google::protobuf::Reflection* reflection = object->GetReflection();
            for(int field_number=0; field_number < descriptor->field_count(); field_number++){
                const google::protobuf::FieldDescriptor* field_descriptor = descriptor->field(field_number);
                if(reflection->HasField(*object, field_descriptor)){
                    buffer << field_descriptor->name() << " = ";
                    if(field_descriptor->type() == google::protobuf::FieldDescriptor::TYPE_STRING){
                        buffer << reflection->GetString(*object, field_descriptor) << "; ";
                    }else if(field_descriptor->type() == google::protobuf::FieldDescriptor::TYPE_INT32){
                        buffer << reflection->GetInt32(*object, field_descriptor) << "; ";
                    }else{
                        buffer << "type unkown; ";
                    }
                }
            }
            buffer << std::endl;
        }

    }
    return buffer.str();
}


std::string pb2xml(pbnavitia::PTRefResponse& response){
    std::stringstream buffer;
    buffer << "<list>";
    for(int i=0; i < response.item_size(); i++){
        pbnavitia::PTreferential* item = response.mutable_item(i);

        const google::protobuf::Reflection* item_reflection = item->GetReflection();
        std::vector<const google::protobuf::FieldDescriptor*> field_list;
        item_reflection->ListFields(*item, &field_list);
        BOOST_FOREACH(const google::protobuf::FieldDescriptor* item_field_descriptor, field_list){
            google::protobuf::Message* object = item_reflection->MutableMessage(item, item_field_descriptor);
            buffer << "<" << item_field_descriptor->name() << " ";
            const google::protobuf::Descriptor* descriptor = object->GetDescriptor();
            const google::protobuf::Reflection* reflection = object->GetReflection();
            for(int field_number=0; field_number < descriptor->field_count(); field_number++){
                const google::protobuf::FieldDescriptor* field_descriptor = descriptor->field(field_number);
                if(reflection->HasField(*object, field_descriptor)){
                    buffer << field_descriptor->name() << "=\"";
                    if(field_descriptor->type() == google::protobuf::FieldDescriptor::TYPE_STRING){
                        buffer << reflection->GetString(*object, field_descriptor) << "\" ";
                    }else if(field_descriptor->type() == google::protobuf::FieldDescriptor::TYPE_INT32){
                        buffer << reflection->GetInt32(*object, field_descriptor) << "\" ";
                    }else{
                        buffer << "type unkown\" ";
                    }
                }
            }
            buffer << "/>";
        }

    }
    buffer << "</list>";
    return buffer.str();
}


}} // navitia::ptref
