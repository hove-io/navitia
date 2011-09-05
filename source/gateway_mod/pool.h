#pragma once

#include <deque>
#include <boost/thread/shared_mutex.hpp>
#include "navitia.h"

class Pool{
    protected:
        boost::shared_mutex mutex;

        struct Sorter{
            bool operator()(const Navitia* a, const Navitia* b){
                if(a->unused_thread == b->unused_thread){
                    return a->last_request_at > b->last_request_at;
                }else{
                    return a->unused_thread > b->unused_thread;
                }
            }
        };

        struct Comparer{
            Navitia ref;
            Comparer(const Navitia& ref) : ref(ref){}

            bool operator()(Navitia* value) const{
                return (*value == ref);
            }
        };

    public:
        
        int nb_threads;
        std::deque<Navitia*> navitia_list;

        Pool();

        inline void release_navitia(Navitia* navitia){
            navitia->release();
            mutex.lock();
            std::sort_heap(navitia_list.begin(), navitia_list.end(), Sorter());
            mutex.unlock();
        }

        inline Navitia* next(){
            boost::lock_guard<boost::shared_mutex> lock(mutex);
            std::pop_heap(navitia_list.begin(), navitia_list.end(), Sorter());
            Navitia* nav = navitia_list.back();
            nav->use();
            std::sort_heap(navitia_list.begin(), navitia_list.end(), Sorter());

            return nav;
        }

        void add_navitia(Navitia* navitia);
        void remove_navitia(const Navitia& navitia);

};
