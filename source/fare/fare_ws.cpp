
/** Exemple de webservice : il affiche le nombre de requêtes traitées par le webservice et par le thread courant */

#include "baseworker.h"
#include "configuration.h"
#include "fare.h"
#include <iostream>
#include <fstream>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>
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
        try{
            conf->load_ini(conf->get_string("path") + "fare.ini");
            fares.init(conf->ini["files"]["grammar"], conf->ini["files"]["prices"]);
            if(conf->ini["files"]["stif"] != "")
                fares.load_od_stif(conf->ini["files"]["stif"]);
        }catch(std::exception e){
            boost::posix_time::ptime date(boost::posix_time::microsec_clock::local_time());
            std::ofstream f(conf->get_string("path") + "error.log", std::ios_base::app);
            f << boost::posix_time::to_simple_string(date) << " - " << e.what() << " - " << __FILE__ << ":" << __LINE__ << std::endl;
        }catch(...){
            boost::posix_time::ptime date(boost::posix_time::microsec_clock::local_time());
            std::ofstream f(conf->get_string("path") + "error.log", std::ios_base::app);
            f << boost::posix_time::to_simple_string(date) << " - error - " << __FILE__ << ":" <<__LINE__ << std::endl;
        }
    }
};

/// Classe associée à chaque thread
class Worker : public BaseWorker<Data> {

    void render(RequestData&, ResponseData& rd, std::vector<Ticket>& tickets){
        rd.content_type = "text/html";
        rd.status_code = 200;
        rd.response << "<FareList FareCount=\"" << tickets.size() << "\">";
    
        BOOST_FOREACH(Ticket t, tickets){
            rd.response << "<Fare><Cost Money=\"Euro\">" << boost::format("%2.2f") % (t.value/100.0) << "</Cost>";
            
            rd.response << "</Fare>";
        }

        rd.response << "</FareList>";
    }

    ResponseData fare(RequestData request, Data & d) {
        std::vector<std::string> section_keys;
        std::pair<std::string, std::string> section;
        BOOST_FOREACH(section, request.params){
            section_keys.push_back(section.second);
        }
        std::vector<Ticket> tickets = d.fares.compute(section_keys);

        ResponseData rd;
        render(request, rd, tickets);
        return rd;
    }




    public:
    /** Constructeur par défaut
     *
     * On y enregistre toutes les api qu'on souhaite exposer
     */
    Worker(Data &) {
        register_api("/journeyfare",boost::bind(&Worker::fare, this, _1, _2), "Effectue le calcul de tarif");
        add_default_api();
    }
};

/// Macro qui va construire soit un exectuable FastCGI, soit une DLL ISAPI
MAKE_WEBSERVICE(Data, Worker)
