#include "data.h"
#include <iostream>

using namespace navimake;

void Data::sort(){

    std::sort(networks.begin(), networks.end(), Less<navimake::types::Network>());
    std::for_each(networks.begin(), networks.end(), Indexer<navimake::types::Network>());

    std::sort(cities.begin(), cities.end(), Less<navimake::types::City>());
    std::for_each(cities.begin(), cities.end(), Indexer<navimake::types::City>());
}
