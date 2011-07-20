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

    public:
        
        int nb_threads;
        std::deque<Navitia*> navitia_list;

        Pool() : nb_threads(8){
            navitia_list.push_back(new Navitia("http://localhost:81/1", 8));
            std::make_heap(navitia_list.begin(), navitia_list.end(), Sorter());
        }

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

};
