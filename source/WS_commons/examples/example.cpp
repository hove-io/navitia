/** Exemple de webservice : il affiche le nombre de requêtes traitées par le webservice et par le thread courant */
#include "baseworker.h"
#include <iostream>
using namespace webservice;

/** Structure de données globale au webservice
 *  N'est instancié qu'une seule fois au chargement
 */
struct Data{
  int nb_threads; /// Nombre de threads. IMPORTANT ! Sans cette variable, ça ne compile pas
  int count; /// Notre compteur d'appels au webservice
  boost::mutex mut; /// Un mutex pour protéger ce cout
  /// Constructeur par défaut, il est appelé au chargement du webservice
  Data() : nb_threads(8), count(0){
      Configuration * conf = Configuration::get();
      std::cout << "Je suis l'executable " << conf->get_string("application") <<std::endl;
      std::cout << "Je réside dans le path " << conf->get_string("path") <<std::endl;
  }
};

/// Classe associée à chaque thread
class Worker : public BaseWorker<Data> {
    int i; /// Compteur de requêtes sur le thread actuel

    /** Api qui compte le nombre de fois qu'elle a été appelée */
    ResponseData count(RequestData, Data & d) {
        i++;
        ResponseData rd;        
        rd.response << "Hello world!!! Exécuté par ce thread : " << i << " executé au total : ";
        d.mut.lock();
        rd.response << d.count++;
        d.mut.unlock();

        rd.content_type = "text/html";
        rd.status_code = 200;
        return rd;
    }



    public:    
    /** Constructeur par défaut
      *
      * On y enregistre toutes les api qu'on souhaite exposer
      */
    Worker(Data &) : i(0) {
        register_api("count",boost::bind(&Worker::count, this, _1, _2), "Api qui compte le nombre d'appels effectués");
        add_default_api();
    }
};

/// Macro qui va construire soit un exectuable FastCGI, soit une DLL ISAPI
MAKE_WEBSERVICE(Data, Worker)
