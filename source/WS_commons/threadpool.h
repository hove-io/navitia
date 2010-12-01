/** Composant de base pour tous les WebServices
 *
 * Est utilisé pour générer des DLL ISAPI ou des executables FastCGI.
 * Le nombre de threads est configurable.
 *
 * Pour l'utiliser, il faut définir une classe Worker qui contient
 */

#pragma once

#include <string>
#include <queue>
#include <sstream>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition.hpp> 
#ifdef FCGI
#include "fcgi.h"
#endif
#ifdef ISAPI
#include "isapi.h"
#endif

namespace webservice
{
    /** Classe principale gérant les threads et dispatchant les requêtes
     *
     * L'utilisateur doit définir deux classes :
     * — Worker qui effectue le traitement à proprement parler (une instance par thread)
     * — Data qui contient les données globales (une unique instance pour l'application)
     */
    template<class Data, class Worker> class ThreadPool {
        /// Nombre de thread pour traiter les requêtes
        int nb_threads;

        /// Taille maximum de la queue de requêtes (par défaut 1 : le serveur web la gère)
        int max_queue_length;

        /// Est-ce que les threads doivent continuer à s'executer
        bool run;

        /// Queue de requêtes à traiter
        std::queue<RequestHandle*> queue;

        /// Mutex global pour accéder à la queue
        boost::mutex queue_mutex;

        /// Condition permettant de signaler aux thread qu'une requête est disponible
        boost::condition_variable request_pushed_cond;

        /// Condition permettant de signaler à la queue qu'un thread vient de prendre une requête
        boost::condition_variable request_poped_cond;

        /// Groupe de threads
        boost::thread_group thread_group;

        /// Données globales
        Data & data;

        public:
        /// Rajoute une requête dans queue. Si la queue est pleine, la fonction est bloquante.
        void push(RequestHandle* request){
            boost::unique_lock<boost::mutex> lock(queue_mutex);
            while(queue.size() >= max_queue_length){
                request_poped_cond.wait(lock);
            }
            queue.push(request);
            request_pushed_cond.notify_one();
        } 


        /// Fonction associée à chaque thread. Pop la queue et lance le traitement de la requête
        void worker(){
            Worker w;
            while(run) {
                boost::unique_lock<boost::mutex> lock(queue_mutex);
                while(queue.empty() && run){
                    request_pushed_cond.wait(lock);
                }
                if(!run)
                    return;
                BOOST_ASSERT(!queue.empty());
                RequestHandle * request = queue.front();
                queue.pop();
                lock.unlock();
                request_poped_cond.notify_one();
                request_parser(request, w, data);
            }
        }

        /// Arrête le pool de threads
        void stop(){
            run = false;
            request_pushed_cond.notify_all();
            thread_group.join_all();
        }

        /// Interrompt tous les threads en cours
        ~ThreadPool(){
            queue_mutex.lock();
            thread_group.interrupt_all();
            queue_mutex.unlock();
        }

        /// Constructeur, on passe une instance de Data
        ThreadPool(Data & data, int nb_threads = 2, int max_queue_length = 1):
            nb_threads(nb_threads),
            max_queue_length(max_queue_length),
            run(true),
            data(data)
        {
                push_request = boost::bind(&ThreadPool<Data, Worker>::push, this, _1);
                stop_threadpool = boost::bind(&ThreadPool<Data, Worker>::stop, this);
            for(int i = 0; i < nb_threads; ++i){
                thread_group.create_thread(boost::bind(&ThreadPool<Data, Worker>::worker, this));
            }
        }

    };
};

