#pragma once

#include <deque>
#include <boost/thread/shared_mutex.hpp>
#include "navitia.h"

class Pool{
    protected:
        boost::shared_mutex mutex;
        
        /**
         * fonctor utilisé pour le tris des instances de navitia
         * on met a la fin les instances en erreurs 
         * et on favorise les instances les moins chargé en fonction de leurs capacité en thread disponible
         *
         */
        struct Sorter{
            bool operator()(const std::shared_ptr<Navitia> a, const std::shared_ptr<Navitia> b){
                //on favorise celui qui a le moins d'erreurs
                if(!a->enable){
                    return true;
                }else if(!b->enable){
                    return false;
                }
                //si le nombre d'erreurs est identique, on favorise le moins chargé
                if(a->unused_thread == b->unused_thread){
                    return a->last_request_at > b->last_request_at;
                }else{
                    return a->unused_thread > b->unused_thread;
                }
            }
        };
        
        ///fonctor utilisé pour cherché un navitia dans la liste
        struct Comparer{
            Navitia ref;
            Comparer(const Navitia& ref) : ref(ref){}

            bool operator()(std::shared_ptr<Navitia> value) const{
                return (*value == ref);
            }
        };

    public:
        
        int nb_threads;
        ///liste des instances navitia trié sous la forme d'un tas
        std::deque<std::shared_ptr<Navitia>> navitia_list;

        Pool();
        
        /// assure la libération du navitia, et retris la liste
        inline void release_navitia(std::shared_ptr<Navitia> navitia){
            navitia->release();
            mutex.lock();
            std::make_heap(navitia_list.begin(), navitia_list.end(), Sorter());
            mutex.unlock();
        }
    
        /**
         * Methode qui assure le load Balancing
         *
         */
        inline std::shared_ptr<Navitia> next(){
            boost::lock_guard<boost::shared_mutex> lock(mutex);
            std::pop_heap(navitia_list.begin(), navitia_list.end(), Sorter());
            auto nav = navitia_list.back();
            nav->use();
            std::make_heap(navitia_list.begin(), navitia_list.end(), Sorter());

            return nav;
        }
        void add_navitia(std::shared_ptr<Navitia> navitia);

        void remove_navitia(const Navitia& navitia);
        void check_desactivated_navitia();

};
