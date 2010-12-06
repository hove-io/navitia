#include "utils.h"
#include <boost/tokenizer.hpp>


namespace webservice{
    map parse_params(const std::string & query_string) {
        boost::char_separator<char> sep("&;");
        boost::tokenizer<boost::char_separator<char> > tokens(query_string, sep);
        for(auto it = tokens.begin(); it != tokens.end(); ++it) {

        }
        return m;
    }
}

int main(int, char**){
    webservice::parse_params("hi");
}

