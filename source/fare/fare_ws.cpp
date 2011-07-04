
/** Exemple de webservice : il affiche le nombre de requêtes traitées par le webservice et par le thread courant */

#include "baseworker.h"
#include "configuration.h"
#include "fare.h"
#include <iostream>
using namespace webservice;

/** Structure de données globale au webservice
 *  N'est instancié qu'une seule fois au chargement
 */
struct Data{
  int nb_threads; /// Nombre de threads. IMPORTANT ! Sans cette variable, ça ne compile pas
  boost::mutex mut; /// Un mutex pour protéger ce cout
  Fare fares; /// Contient tout le graphe capable de calculer les tarifs
  /// Constructeur par défaut, il est appelé au chargement du webservice
  Data() : nb_threads(8){
	 Configuration * conf = Configuration::get();
	conf->load_ini(conf->get_string("path") + "fare.ini");
	fares.init(conf->ini["files"]["grammar"], conf->ini["files"]["prices"]);
	if(conf->ini["files"]["stif"] != "")
		fares.load_od_stif(conf->ini["files"]["stif"]);
    std::cout << "chargement terminé avec " << boost::num_vertices(fares.g) << " nœuds et " << boost::num_edges(fares.g) << " arcs" << std::endl;
  }
};

/// Classe associée à chaque thread
class Worker : public BaseWorker<Data> {
    /** Api qui compte le nombre de fois qu'elle a été appelée */
    ResponseData count(RequestData request, Data & d) {
        std::vector<std::string> section_keys;
        std::pair<std::string, std::string> section;
        BOOST_FOREACH(section, request.params){
            section_keys.push_back(section.second);
        }
        std::vector<Ticket> tickets = d.fares.compute(section_keys);

        ResponseData rd;
        rd.response << "<html><body><h1>Fare : liste des billets à acheter</h1>\n<ul>";
        BOOST_FOREACH(Ticket t, tickets){
            rd.response << "<li>" << t.caption << " " << t.value << "</li>\n";
        }
        rd.response << "</ul></body></html>";

        rd.content_type = "text/html";
        rd.status_code = 200;
        return rd;
    }



    public:
    /** Constructeur par défaut
      *
      * On y enregistre toutes les api qu'on souhaite exposer
      */
    Worker(Data &) {
        register_api("/journeyfare",boost::bind(&Worker::count, this, _1, _2), "Effectue le calcul de tarif");
        add_default_api();
    }
};

/// Macro qui va construire soit un exectuable FastCGI, soit une DLL ISAPI
MAKE_WEBSERVICE(Data, Worker)
