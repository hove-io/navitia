#include "pool.h"

void Pool::add_navitia(Navitia* navitia){
    mutex.lock();
    navitia_list.push_back(navitia);
    std::push_heap(navitia_list.begin(), navitia_list.end(), Sorter());
    mutex.unlock();
}

void Pool::remove_navitia(Navitia* navitia){
    mutex.lock();
    std::deque<Navitia*>::iterator it = std::find_if(navitia_list.begin(), navitia_list.end(), Comparer(navitia));
    if(it == navitia_list.end()){
        mutex.unlock();
        return;
    }
    Navitia* nav = *it;
    navitia_list.erase(it);
    std::sort_heap(navitia_list.begin(), navitia_list.end(), Sorter());
    mutex.unlock();
    //utilisé un thread pour détruire le navitia quand celui ci ne serat plus utilisé?
    //TODO sinon fuite mémoire
}

