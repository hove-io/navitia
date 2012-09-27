#include "pb_utils.h"
#include <google/protobuf/descriptor.h>
#include <boost/lexical_cast.hpp>
#include <sstream>

std::string pb2xml(const google::protobuf::Message* response){
    std::stringstream buffer;
    std::stringstream child_buffer;

    const google::protobuf::Reflection* reflection = response->GetReflection();
    const google::protobuf::Descriptor* descriptor = response->GetDescriptor();
    std::vector<const google::protobuf::FieldDescriptor*> field_list;
    reflection->ListFields(*response, &field_list);
    buffer << "<" << descriptor->name() << " ";
    for(const google::protobuf::FieldDescriptor* field : field_list){
        if(field->is_repeated()) {
            child_buffer << "<" << field->name() << ">";
            for(int i=0; i < reflection->FieldSize(*response, field); ++i){
                switch(field->type()) {
                case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
                    child_buffer << pb2xml(&reflection->GetRepeatedMessage(*response, field, i));
                    break;
                case google::protobuf::FieldDescriptor::TYPE_INT32:
                    child_buffer << "<item>" << reflection->GetRepeatedInt32(*response, field, i) << "</item>";
                    break;
                case google::protobuf::FieldDescriptor::TYPE_STRING:
                    child_buffer << "<item>" << reflection->GetRepeatedString(*response, field, i) << "</item>";
                    break;
                default:
                    child_buffer << "other" ;
                }
            }
            child_buffer << "</" << field->name() << ">";
        }
        else if(reflection->HasField(*response, field)){
            if(field->type() == google::protobuf::FieldDescriptor::TYPE_STRING){
                buffer << field->name() << "=\"";
                buffer << reflection->GetString(*response, field) << "\" ";
            }else if(field->type() == google::protobuf::FieldDescriptor::TYPE_INT32){
                buffer << field->name() << "=\"";
                buffer << reflection->GetInt32(*response, field) << "\" ";
            }else if(field->type() == google::protobuf::FieldDescriptor::TYPE_DOUBLE){
                buffer << field->name() << "=\"";
                buffer << reflection->GetDouble(*response, field) << "\" ";
            }else if(field->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE){
                child_buffer << pb2xml(&reflection->GetMessage(*response, field));
            }else if(field->type() == google::protobuf::FieldDescriptor::TYPE_BOOL){
                buffer << field->name() << "=\"";
                buffer << reflection->GetBool(*response, field) << "\" ";
            }else {
                buffer << field->name() << "=\"";
                buffer << "type unkown\" ";
            }
        }
    }
    std::string child = child_buffer.str();
    if(child.length() > 0){
        buffer << ">" << child << "</" << descriptor->name() << ">";
    }else{
        buffer << "/>";
    }
    buffer << std::endl;
    return buffer.str();
}

void indent(std::stringstream& buffer, int depth){
    for (int i = 0; i < depth; ++i) {
        buffer << "    ";
    }
}


std::string pb2json(const google::protobuf::Message* response, int depth){
    std::stringstream buffer;
    buffer << "{";

    const google::protobuf::Reflection* reflection = response->GetReflection();
    std::vector<const google::protobuf::FieldDescriptor*> field_list;
    reflection->ListFields(*response, &field_list);

    bool is_first = true;
    for(const google::protobuf::FieldDescriptor* field : field_list){
        if(is_first){
            is_first = false;
        } else {
            buffer << ", ";
        }
        if(depth > 0){ 
            buffer << "\n";
            indent(buffer, depth);
        }
        if(field->is_repeated()) {
            buffer << "\"" << field->name() << "_list\": [";
            if(depth == 0) buffer << "\n";
            for(int i=0; i < reflection->FieldSize(*response, field); ++i){
                switch(field->type()) {
                case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
                    buffer << pb2json(&reflection->GetRepeatedMessage(*response, field, i), depth+1);
                    break;
                case google::protobuf::FieldDescriptor::TYPE_INT32:
                    buffer << reflection->GetRepeatedInt32(*response, field, i);
                    break;
                case google::protobuf::FieldDescriptor::TYPE_STRING:
                    buffer << "\"" << reflection->GetRepeatedString(*response, field, i) << "\"";
                    break;
                default:
                    buffer << "other" ;
                }

                if(i+1 < reflection->FieldSize(*response, field)){
                    buffer << ", ";
                    if(depth == 0) buffer << "\n";
                }
            }
            if(depth == 0) buffer << "\n";
            buffer <<  "]";
        }
        else if(reflection->HasField(*response, field)){
            buffer << "\"" <<field->name() << "\": ";
            switch(field->type()) {
            case google::protobuf::FieldDescriptor::TYPE_STRING:
                buffer << "\"" << reflection->GetString(*response, field) << "\"";
                break;
            case google::protobuf::FieldDescriptor::TYPE_INT32:
                buffer << reflection->GetInt32(*response, field);
                break;
            case google::protobuf::FieldDescriptor::TYPE_DOUBLE:
                buffer << reflection->GetDouble(*response, field);
                break;
            case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
                buffer << pb2json(&reflection->GetMessage(*response, field), depth + 1);
                break;
            case google::protobuf::FieldDescriptor::TYPE_ENUM:
                buffer << "\"" << reflection->GetEnum(*response, field)->name() << "\"";
                break;
            case google::protobuf::FieldDescriptor::TYPE_BOOL:
                buffer << "\"" << boost::lexical_cast<std::string>(reflection->GetBool(*response, field)) << "\"";
                break;
            default:
                buffer << "Other !, " ;
            }
        }
    }
    if(depth > 0){
        buffer << "\n";
        indent(buffer, depth-1);
    }
    buffer << "}";
    return buffer.str();
}


std::unique_ptr<pbnavitia::Response> create_pb(){
    return std::unique_ptr<pbnavitia::Response>(new pbnavitia::Response());
}
