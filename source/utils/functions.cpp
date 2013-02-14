#include "functions.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/split.hpp>


double str_to_double(std::string str){
    boost::trim(str);
    try{
        boost::replace_all(str, ",", ".");
        return boost::lexical_cast<double>(str);
    } catch(...){        
        return -1;
    }
}

int str_to_int(std::string str){    
    boost::trim(str);
    if (str.empty()){
        return -1;
    }
    try{
        return boost::lexical_cast<int>(str);
    } catch(...){
        return -1;
    }
}

std::vector<std::string> split_string(const std::string& str,const std::string & separator){
    std::vector< std::string > SplitVec;
    split( SplitVec, str, boost::is_any_of(separator), boost::token_compress_on );
    return SplitVec;
}
