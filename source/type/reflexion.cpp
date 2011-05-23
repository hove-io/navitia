#include "reflexion.h"

member_e get_member(const std::string & member_str){
    if(member_str == "id") return id;
    else if (member_str == "idx") return idx;
    else if (member_str == "name") return name;
    else if (member_str == "external_code") return external_code;
    else return unknown_member_;
}

/*class_e get_class(const std::string & class_str){
    if(class_str == "stoparea") return stop_area;
    else if(class_str == "stoppoint") return stop_point;
    else if(class_str == "line") return line;
    else if(class_str == "validity_pattern") return validity_pattern;
    return unknown_class;
}*/
