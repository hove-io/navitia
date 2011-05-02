#include <iostream>

#include "connectors.h"

int main (int argc, char const* argv[])
{
    BO::connectors::CsvFusio connector("/home/kinou/Public/BOD/");
    BO::Data data;
    connector.fill(data);

    std::cout << data.lines[2]->name << std::endl;
    std::cout << data.lines[2]->network->name << std::endl;
    std::cout << data.lines[2]->mode_type->name << std::endl;
    
    return 0;
}
