#include <iostream>
#include <boost/foreach.hpp>

#include "connectors.h"

int main (int argc, char const* argv[])
{
    BO::connectors::CsvFusio connector("/home/kinou/Public/BOD/");
    BO::Data data;
    connector.fill(data);
    BOOST_FOREACH(BO::types::Line* line, data.lines){
        std::cout << line->name << std::endl;
        if(line->network != NULL)
            std::cout << line->network->name << std::endl;
        if(line->mode_type != NULL)
            std::cout << line->mode_type->name << std::endl;
    }
    return 0;
}
