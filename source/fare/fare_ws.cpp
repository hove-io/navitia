
/** Exemple de webservice : il affiche le nombre de requêtes traitées par le webservice et par le thread courant */

#include "baseworker.h"
#include "utils/configuration.h"
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
            if(conf->ini["files"]["od"] != "")
                fares.load_od(conf->ini["files"]["od"]);
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

    void render(RequestData& request, ResponseData& rd, std::vector<Ticket>& tickets){
        rd.content_type = "text/xml";
        rd.status_code = 200;
        rd.response << "<FareList FareCount=\"" << tickets.size() << "\">\n";
        rd.response << "<Params Function=\"Fare\">\n";
        BOOST_FOREACH(auto params, request.params){
            rd.response << "<" << params.first << ">" << params.second << "</" << params.first << ">\n";
        }
        rd.response << "</Params>\n";
    
        BOOST_FOREACH(Ticket t, tickets){
            rd.response << "<Fare><Network>" << (t.sections.size() > 0 ? t.sections.at(0).network : "Unknown") << "</Network>\n";
            rd.response << "<Cost Money=\"Euro\">" << boost::format("%2.2f") % (t.value/100.0) << "</Cost>\n";
            rd.response << "<Label>" << t.caption << "</Label>\n";
            rd.response << "<Comment>" << t.comment  << "</Comment>\n";
            rd.response << "<SectionList SectionCount=\"" << t.sections.size() << "\">\n";
            BOOST_FOREACH(SectionKey section, t.sections){
                rd.response << "<Section SectionKey=\"" << section.section << "\"/>\n";
            }
            rd.response << "</SectionList>\n";
            rd.response << "</Fare>\n";
        }

        time_t rawtime;
        struct tm * timeinfo;

        time ( &rawtime );
        timeinfo = localtime ( &rawtime );

        if(request.params["safemode"] == "0")
            rd.response << "<Hit Ide=\"%d\" "
                        << "Date=\""<< timeinfo->tm_mday << "\" "
                        << "Month=\"" << (timeinfo->tm_mon + 1)<< "\" "
                        << "Time=\"" << timeinfo->tm_hour <<":" << timeinfo->tm_min << ":" << timeinfo->tm_sec <<"\" "
                        << "Server=\"\" Action=\"journeyfare\" Duration=\"00:00:00\" "
                        << "ResponseSize=\"" << rd.response.str().size() << "\" "
                        << "Script=\"/" << request.path << "\" WsnId=\"%d\"/>";
        rd.response << "</FareList>\n";
    }

    ResponseData fare(RequestData request, Data & d) {
        ResponseData rd;
        try{
            std::vector<std::string> section_keys;
            std::pair<std::string, std::string> section;
            BOOST_FOREACH(section, request.params){
                if(boost::to_lower_copy(section.first).find("section") == 0)
                section_keys.push_back(section.second);
            }
            std::vector<Ticket> tickets = d.fares.compute(section_keys);
            render(request, rd, tickets);
        }catch(std::string s){
            rd.response.clear();
            rd.content_type = "text/xml";
            rd.response << "<error>" << s << "</error>";
        }catch(std::exception e){
            rd.response.clear();
            rd.content_type = "text/xml";
            rd.response << "<error>" << e.what() << "</error>";
        }catch(...){
            rd.response.clear();
            rd.content_type = "text/xml";
            rd.response << "<error>Unknown</error>";
        }

        return rd;
    }




    public:
    /** Constructeur par défaut
     *
     * On y enregistre toutes les api qu'on souhaite exposer
     */
    Worker(Data &) {
        register_api("fare",boost::bind(&Worker::fare, this, _1, _2), "Effectue le calcul de tarif");
        add_default_api();
    }
};

/// Macro qui va construire soit un exectuable FastCGI, soit une DLL ISAPI
MAKE_WEBSERVICE(Data, Worker)
