#include "ptreferential.h"

#include <iostream>

#include <fstream>

std::string unpluralize_table(const std::string& table_name){
    if(table_name == "cities"){
        return "city";
    }else{
        return table_name.substr(0, table_name.length() - 1);
    }
}


google::protobuf::Message* get_message(pbnavitia::PTreferential* row, const std::string& table){
    const google::protobuf::Reflection* reflection = row->GetReflection();
    const google::protobuf::Descriptor* descriptor = row->GetDescriptor();
    std::string field = unpluralize_table(table);
    const google::protobuf::FieldDescriptor* field_descriptor = descriptor->FindFieldByName(field);
    return reflection->MutableMessage(row, field_descriptor);
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
        throw std::string("too many table");
    }
    else {
        std::cout << "Table : " << r.tables[0] << std::endl;
    }

    std::string table = r.tables[0];

    if(table == "validity_pattern") {
        return extract_data(data.validity_patterns, r);
    }
    else if(table == "lines") {
        return extract_data(data.lines, r);
    }
    else if(table == "routes") {
        return extract_data(data.routes, r);
    }
    else if(table == "vehicle_journey") {
        return extract_data(data.vehicle_journeys, r);
    }
    else if(table == "stop_points") {
        return extract_data(data.stop_points, r);
    }
    else if(table == "stop_areas") {
        return extract_data(data.stop_areas, r);
    }
    else if(table == "stop_times"){
        return extract_data(data.stop_times, r);
    }
    
    throw unknown_table();
}


