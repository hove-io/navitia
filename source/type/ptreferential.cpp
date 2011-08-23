#include "ptreferential.h"

#include <iostream>
#include <fstream>
#include <algorithm>

namespace navitia{ namespace ptref{

google::protobuf::Message* get_message(pbnavitia::PTreferential* row, Type_e type){
    const google::protobuf::Reflection* reflection = row->GetReflection();
    const google::protobuf::Descriptor* descriptor = row->GetDescriptor();
    std::string field = static_data::get()->captionByType(type);
    const google::protobuf::FieldDescriptor* field_descriptor = descriptor->FindFieldByName(field);
    return reflection->MutableMessage(row, field_descriptor);
}

template<Type_e E, class T>
std::vector<idx_t> foo(std::vector<WhereClause> clauses,  Type_e requested_type, std::vector<T> & data, Data & d)
{
    //typedef typename boost::mpl::at<enum_type_map, boost::mpl::int_<E> >::type T;
    Index2<boost::fusion::vector<T> > filtered(data, build_clause<T>(clauses));
    auto offsets = filtered.get_offsets();
    std::vector<idx_t> indexes;
    indexes.reserve(filtered.size());
    BOOST_FOREACH(auto item, offsets){
        indexes.push_back(item[0]);
    }
    std::vector<Type_e> path = Jointures().find_path(requested_type);
    Type_e current = E;
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
        std::cout << r.tables.size() << std::endl;
        throw std::string("too many table");
    }
    else {
        std::cout << "Table : " << static_data::get()->captionByType(r.tables[0]) << std::endl;
    }

    // On regroupe les clauses par tables sur lequelles elles vont taper
    // Ça va se compliquer le jour où l'on voudra gérer des and/or intermélangés...
    std::map<Type_e, std::vector<WhereClause> > clauses;
    BOOST_FOREACH(WhereClause clause, r.clauses){
        clauses[clause.col.table].push_back(clause);
    }

    std::pair<Type_e, std::vector<WhereClause> > type_clauses;
    Type_e requested_type = r.tables[0];
    std::vector<idx_t> final_indexes = data.get_all_index(requested_type);
    std::vector<idx_t> indexes;
    BOOST_FOREACH(type_clauses, clauses){
        switch(type_clauses.first){
        case eLine: indexes = foo<eLine>(type_clauses.second, requested_type, data.lines,data); break;
        case eValidityPattern: indexes = foo<eValidityPattern>(type_clauses.second, requested_type, data.validity_patterns,data); break;
        case eRoute: indexes = foo<eRoute>(type_clauses.second, requested_type, data.routes,data); break;
        case eVehicleJourney: indexes = foo<eVehicleJourney>(type_clauses.second, requested_type, data.vehicle_journeys,data); break;
        case eStopPoint: indexes = foo<eStopPoint>(type_clauses.second, requested_type, data.stop_points,data); break;
        case eStopArea: indexes = foo<eStopArea>(type_clauses.second, requested_type, data.stop_areas,data); break;
        case eStopTime: indexes = foo<eStopTime>(type_clauses.second, requested_type, data.stop_times,data); break;
        case eNetwork: indexes = foo<eNetwork>(type_clauses.second, requested_type, data.networks,data); break;
        case eMode: indexes = foo<eMode>(type_clauses.second, requested_type, data.modes,data); break;
        case eModeType: indexes = foo<eModeType>(type_clauses.second, requested_type, data.mode_types,data); break;
        case eCity: indexes = foo<eCity>(type_clauses.second, requested_type, data.cities,data); break;
        case eConnection: indexes = foo<eConnection>(type_clauses.second, requested_type, data.connections,data); break;
        case eRoutePoint: indexes = foo<eRoutePoint>(type_clauses.second, requested_type, data.route_points,data); break;
        case eDistrict: indexes = foo<eDistrict>(type_clauses.second, requested_type, data.districts,data); break;
        case eDepartment: indexes = foo<eDepartment>(type_clauses.second, requested_type, data.departments,data); break;
        case eCompany: indexes = foo<eCompany>(type_clauses.second, requested_type, data.companies,data); break;
        case eVehicle: indexes = foo<eVehicle>(type_clauses.second, requested_type, data.vehicles,data); break;
        case eCountry: indexes = foo<eCountry>(type_clauses.second, requested_type, data.countries,data); break;
        case eUnknown: break;
        }
        // Attention ! les structures doivent être triées !
        std::vector<idx_t> tmp_indexes;
        std::back_insert_iterator< std::vector<idx_t> > it(tmp_indexes);
        std::set_intersection(final_indexes.begin(), final_indexes.end(), indexes.begin(), indexes.end(), it);
        final_indexes = tmp_indexes;
    }

    switch(r.tables[0]){
    case eValidityPattern: return extract_data(data.validity_patterns, r, final_indexes); break;
    case eLine: return extract_data(data.lines, r, final_indexes); break;
    case eRoute: return extract_data(data.routes, r, final_indexes); break;
    case eVehicleJourney: return extract_data(data.vehicle_journeys, r, final_indexes); break;
    case eStopPoint: return extract_data(data.stop_points, r, final_indexes); break;
    case eStopArea: return extract_data(data.stop_areas, r, final_indexes); break;
    default:  break;
    }

    throw unknown_table();
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
