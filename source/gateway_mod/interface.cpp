#include "interface.h"
#include <google/protobuf/descriptor.h>
#include <sstream>
#include <boost/foreach.hpp>


std::string pb2xml(pbnavitia::PTRefResponse* response){
    std::stringstream buffer;
    buffer << "<list>";
    for(int i=0; i < response->item_size(); i++){
        pbnavitia::PTreferential* item = response->mutable_item(i);

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
