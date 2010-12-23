/** Exemple de webservice : il affiche le nombre de requêtes traitées par le webservice et par le thread courant */

#include "baseworker.h"
typedef BaseWorker<int> bw;

using namespace webservice;
using namespace boost::posix_time;

/** Structure de données globale au webservice
 *  N'est instancié qu'une seule fois au chargement
 */
struct Data{
  int nb_threads; /// Nombre de threads. IMPORTANT ! Sans cette variable, ça ne compile pas
  int count; /// Notre compteur d'appels au webservice
  boost::mutex mut; /// Un mutex pour protéger ce cout
  /// Constructeur par défaut, il est appelé au chargement du webservice
  Data() : nb_threads(8), count(0){
  }
};

/// Classe associée à chaque thread
class Worker{
    int i; /// Compteur de requêtes sur le thread actuel
    public:
    Worker() : i(0) {} /// Constructeur par défaut
    
    /// Fonction appelée à chaque requête. Il faut respecter cette signature !
    ResponseData operator()(const RequestData &, Data & d){
        i++;
        ResponseData rd;
        std::stringstream ss;
        ss << "Hello world!!! Exécuté par ce thread : " << i << " executé au total : ";
        d.mut.lock();
        ss << d.count++;
        d.mut.unlock();
        
        rd.response = ss.str();
        rd.content_type = "text/html";
        rd.status_code = 200;
        return rd;
    }
};

/// Macro qui va construire soit un exectuable FastCGI, soit une DLL ISAPI
MAKE_WEBSERVICE(Data, Worker)
