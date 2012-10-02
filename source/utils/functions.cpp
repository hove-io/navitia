#include "functions.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

double str_to_double(std::string str){
    try{
        boost::replace_all(str, ",", ".");
        return boost::lexical_cast<double>(str);
    } catch(...){
        std::cout << "Str_To_Double : " << str << std::endl;
        return -1;
    }
}

int str_to_int(std::string str){    
    try{
        if (str.empty()){
            return -1;
        }
        else{ return boost::lexical_cast<int>(str);}
    } catch(...){
        std::cout << "str_to_int : " << str << std::endl;
        return -1;
    }
}
