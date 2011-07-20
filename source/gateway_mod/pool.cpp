#include "pool.h"

void Pool::add_navitia(Navitia* navitia){
    mutex.lock();
    navitia_list.push_back(navitia);
    std::push_heap(navitia_list.begin(), navitia_list.end(), Sorter());
    mutex.unlock();
}
