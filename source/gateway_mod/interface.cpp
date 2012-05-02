#include "interface.h"
#include <google/protobuf/descriptor.h>
#include <sstream>
#include <boost/foreach.hpp>


std::string pb2xml(const google::protobuf::Message* response){
    std::stringstream buffer;
    std::stringstream child_buffer;

    const google::protobuf::Reflection* reflection = response->GetReflection();
    const google::protobuf::Descriptor* descriptor = response->GetDescriptor();
    std::vector<const google::protobuf::FieldDescriptor*> field_list;
    reflection->ListFields(*response, &field_list);
    buffer << "<" << descriptor->name() << " ";
    BOOST_FOREACH(const google::protobuf::FieldDescriptor* field, field_list){
        if(field->is_repeated()) {
            child_buffer << "<" << field->name() << ">";
            for(int i=0; i < reflection->FieldSize(*response, field); ++i){
                child_buffer << pb2xml(&reflection->GetRepeatedMessage(*response, field, i));
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


std::string pb2txt(const google::protobuf::Message* response){
    std::stringstream buffer;

    const google::protobuf::Reflection* reflection = response->GetReflection();
    std::vector<const google::protobuf::FieldDescriptor*> field_list;
    reflection->ListFields(*response, &field_list);

    BOOST_FOREACH(const google::protobuf::FieldDescriptor* field, field_list){
        if(field->is_repeated()) {
            buffer << field->name() << " : [";
            for(int i=0; i < reflection->FieldSize(*response, field); ++i){
                buffer << pb2txt(&reflection->GetRepeatedMessage(*response, field, i)) << " ,";
            }
            buffer << "]\n";
        }
        else if(reflection->HasField(*response, field)){
            buffer << field->name() << " = ";
            if(field->type() == google::protobuf::FieldDescriptor::TYPE_STRING){
                buffer << reflection->GetString(*response, field) << "; ";
            }else if(field->type() == google::protobuf::FieldDescriptor::TYPE_INT32){
                buffer << reflection->GetInt32(*response, field) << "; ";
            }else if(field->type() == google::protobuf::FieldDescriptor::TYPE_DOUBLE){
                buffer << reflection->GetDouble(*response, field) << "; ";
            }else if(field->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE){
                buffer << "\n\t" << pb2txt(&reflection->GetMessage(*response, field)) << "\n";
            }else {
                buffer << "type unkown; ";
            }
        }

        buffer << std::endl;


    }
    return buffer.str();
}

std::string pb2json(const google::protobuf::Message* response, int depth){
    std::stringstream buffer;
    if(depth == 1) buffer.width(5);
    buffer << "{";

    const google::protobuf::Reflection* reflection = response->GetReflection();
    std::vector<const google::protobuf::FieldDescriptor*> field_list;
    reflection->ListFields(*response, &field_list);

    bool is_first = true;
    BOOST_FOREACH(const google::protobuf::FieldDescriptor* field, field_list){
        if(is_first){
            is_first = false;
        } else {
            buffer << ", ";
        }
        if(depth == 1) buffer << "\n      ";
        if(field->is_repeated()) {
            buffer << "\"" << field->name() << "\": [";
            if(depth == 0) buffer << "\n";
            for(int i=0; i < reflection->FieldSize(*response, field); ++i){
                switch(field->type()) {
                case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
                    buffer << pb2json(&reflection->GetRepeatedMessage(*response, field, i), depth+1);
                    break;
                case google::protobuf::FieldDescriptor::TYPE_INT32:
                    buffer << reflection->GetRepeatedInt32(*response, field, i);
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
            default:
                buffer << "Other !, " ;
            }
        }
    }
    if(depth == 1) buffer << "\n    }";
    else buffer << "}";
    return buffer.str();
}

void render(webservice::RequestData& request, webservice::ResponseData& response, Context& context){
    switch(context.service){
        case Context::PTREF:
            if(request.params["format"] == "txt"){
                response.response << pb2txt(context.pb.get());
                response.content_type = "text/html";
                response.status_code = 200;
            }else if(request.params["format"] == "json"){
                response.response << pb2json(context.pb.get(),0);
                response.content_type = "text/plain";
                response.status_code = 200;
            }else {
                //par défaut xml
                response.response << pb2xml(context.pb.get());
                response.content_type = "text/xml";
                response.status_code = 200;
            }
            break;
        case Context::BAD_RESPONSE:
            response.response << context.str;
            break;
        default:
            response.response << "<error/>";
            response.content_type = "text/xml";
    }
}


void render_status(webservice::RequestData& request, webservice::ResponseData& response, Context& context, Pool& pool){
    response.response << "<GatewayStatus><NavitiaList Count=\"" << pool.navitia_list.size() << "\">";
    BOOST_FOREACH(Navitia* nav, pool.navitia_list){
        response.response << "<Navitia unusedThread=\"" << nav->unused_thread << "\" currentRequest=\"";
        response.response << nav->current_thread <<  "\">" << nav->url << "</Navitia>";
    }
    response.response << "</NavitiaList></GatewayStatus>";
    response.content_type = "text/xml";
}

