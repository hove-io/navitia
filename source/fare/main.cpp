#include "fare.h"
#include <iostream>
#include <boost/lexical_cast.hpp>
int main(int, char**){
    std::vector<std::string> keys;
    keys.push_back("Filbleu;FILURSE-2;FILNav31;FILGATO-2;2011|06|01;02|06;02|10;1;1;metro");
    Fare f;
    f.init("/home/tristram/idf.fares", "/home/tristram/prix.csv");
    f.load_od_stif("/home/tristram/tarifs_od.csv");
    f.compute(keys);
    std::cout << f.fare_map.size() << std::endl;
}
