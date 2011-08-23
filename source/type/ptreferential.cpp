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

    std::vector<Type_e> path = Jointures().find_path(requested_type);
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
    select_r<std::string::iterator> s;
    if (qi::phrase_parse(begin, request.end(), s, qi::space, r))
    {
        if(begin != request.end()) {
            std::cout << "Hrrrmmm on a pas tout parsé -_-'" << std::endl;
        }
    }
    else
        std::cout << "Parsage a échoué" << std::endl;

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
