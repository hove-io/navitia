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
#include "fcgiapp.h"

namespace webservice
{
    /// Types possibles de requètes
    enum RequestType {GET, POST};

    /// Type d'interface possible
    enum InterfaceType {ISAPI, FCGI};

    /** Structure contenant toutes les données liées à une requête entrante
     *
     */
    struct RequestData {
        /// Type de la requête (ex. GET ou POST)
        RequestType type;
        /// Chemin demandé (ex. "/const")
        std::string path;
        /// Paramètres passés à la requête (ex. "user=1&passwd=2")
        std::string params;
        /// Données brutes
        std::string data;
    };

    /** Données de requêtes étendues pour pouvoir répondre au serveur selon le type d'interface */
    template<InterfaceType it> struct RequestHandle{};
    template<> struct RequestHandle<ISAPI>{

    };
    template<> struct RequestHandle<FCGI>{
        FCGX_Request * request;
    };

    /** Structure contenant les réponses
     *
     */
    struct ResponseData {
        /// Type de la réponse (ex. "text/xml")
        std::string content_type;
        /// Réponse à proprement parler
        std::string response;
        /// Status http de la réponse (ex. 200)
        int status_code;
        /// Constructeur par défaut (status 200, type text/plain)
        ResponseData() : content_type("text/plain"), status_code(200){}
    };

    /// Gère la requête (lecture des paramètres)
    template<class Worker, class Data> void fcgi_request_parser(RequestHandle<FCGI> handle, Worker & w, Data & data){
        int len = 0;
        RequestData request_data;
        char *contentLength = FCGX_GetParam("CONTENT_LENGTH", handle.request->envp);
        if (contentLength != NULL)
            len = strtol(contentLength, NULL, 10);

        if(len > 0) {
            char * tmp_str = new char[len + 1];
            FCGX_GetStr(tmp_str, len, handle.request->in);
            request_data.data = tmp_str;
            delete tmp_str;
        }
        request_data.path = FCGX_GetParam("SCRIPT_FILENAME", handle.request->envp);
        request_data.params = FCGX_GetParam("QUERY_STRING", handle.request->envp);

        ResponseData resp = w(request_data, data);


        std::stringstream ss;
        ss << "Status: " << resp.status_code << "\r\n"
                << "Content-Type: " << resp.content_type << "\r\n\r\n"
                << resp.response;

        FCGX_FPrintF(handle.request->out, ss.str().c_str());
        FCGX_Finish_r(handle.request);
        delete handle.request;
    }


    /** Classe principale gérant les threads et dispatchant les requêtes
     *
     * L'utilisateur doit définir deux classes :
     * — Worker qui effectue le traitement à proprement parler (une instance par thread)
     * — Data qui contient les données globales (une unique instance pour l'application)
     */
    template<class Data, class Worker, InterfaceType it> class ThreadPool {
        /// Nombre de thread pour traiter les requêtes
        int nb_threads;

        /// Taille maximum de la queue de requêtes (par défaut 1 : le serveur web la gère)
        int max_queue_length;

        /// Est-ce que les threads doivent continuer à s'executer
        bool run;

        /// Queue de requêtes à traiter
        std::queue<RequestHandle<it> > queue;

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
        void push(const RequestHandle<it> & request){
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
                RequestHandle<it> request = queue.front();
                queue.pop();
                lock.unlock();
                request_poped_cond.notify_one();
                if(it == FCGI)
                    fcgi_request_parser(request, w, data);
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
            for(int i = 0; i < nb_threads; ++i){
                thread_group.create_thread(boost::bind(&ThreadPool<Data, Worker,it>::worker, this));
            }
        }


        /// Lance la boucle de traitement fastcgi
        void run_fastcgi(){
            FCGX_Init();
            int rc;

            while(run)
            {
                FCGX_Request * request = new FCGX_Request();
                FCGX_InitRequest(request, 0, 0);
                rc = FCGX_Accept_r(request);
                if(rc<0)
                    break;

                RequestHandle<FCGI> handle;
                handle.request = request;
                push(handle);

            }
        }
    };
};

