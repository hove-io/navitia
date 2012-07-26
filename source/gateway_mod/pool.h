#pragma once

#include <vector>
#include <boost/thread/shared_mutex.hpp>
#include "navitia.h"

namespace navitia{ namespace gateway{

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
            //operator <
            bool operator()(std::shared_ptr<Navitia> a, std::shared_ptr<Navitia> b){
                //on favorise celui qui a le moins d'erreurs
                bool result;
                if(!a->enable){
                    result = false;//true;
                }else if(!b->enable){
                    result = true;//false;
                }else if(a->unused_thread == b->unused_thread){
                //si le nombre d'erreurs est identique, on favorise le moins chargé
                    result = (a->last_request_at < b->last_request_at);
                }else{
                    result = (a->unused_thread < b->unused_thread);
                }
                return result;
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
        std::vector<std::shared_ptr<Navitia>> navitia_list;

        Pool();

        Pool(Pool& other);
        
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
            auto nav = navitia_list.front();
            nav->use();
            std::make_heap(navitia_list.begin(), navitia_list.end(), Sorter());

            return nav;
        }
        void add_navitia(std::shared_ptr<Navitia> navitia);

        void remove_navitia(const Navitia& navitia);
        void check_desactivated_navitia();

};

}}
