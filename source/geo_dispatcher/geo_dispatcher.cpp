#include "baseworker.h"
#include "data_structures.h"
#include "utils/logger.h"
#include "utils/csv.h"
#include "type/type.h"

#include <curl/curl.h>
#include <geos_c.h>

#include <stdarg.h>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

using namespace webservice;

/* On obtient les communes à partir de la base GEOFLA de l'insee
 *
 * On les converti en CSV avec ogr2ogr du paquet GDAL :
 * ogr2ogr -f csv communes COMMUNE.SHP -lco GEOMETRY=AS_WKT -s_srs EPSG:2154 -t_srs EPSG:4326
 * ogr2ogr -f csv communes COMMUNE.SHP -lco GEOMETRY=AS_WKT
 *
 */

struct Geometry {
    GEOSGeometry * geom;

    boost::mutex mutex;


    Geometry() : geom(nullptr) {}


    Geometry(Geometry && other) {
        this->geom = other.geom;
        other.geom = nullptr;
    }
    /**
     *@param handle handle utilisé par GEOS pour le multi thread
     *
     */
    bool contains(GEOSContextHandle_t handle, Geometry& other){
        boost::lock_guard<boost::mutex> lock(mutex);
        boost::lock_guard<boost::mutex> lock_other(other.mutex);
        char res = GEOSContains_r(handle, this->geom, other.geom);
        if(res == 2){
            throw std::exception();
        }
        return res;
    }


    ~Geometry() {
        if(geom != nullptr)
            GEOSGeom_destroy(geom);
    }

    Geometry & operator=(const Geometry & other){
        if(this->geom != nullptr)
            GEOSGeom_destroy(geom);

        if(other.geom != nullptr)
            this->geom = GEOSGeom_clone(other.geom);
        else
            this->geom = nullptr;
        return *this;
    }

    Geometry(const Geometry & other) {
        if(other.geom != nullptr)
            this->geom = GEOSGeom_clone(other.geom);
        else
            this->geom = nullptr;
    }


};

struct Commune {
    std::string name;
    float population;
    std::string insee;
    Geometry geom;

    Commune() : population(0) {}
};

struct Departement {
    std::vector<Commune> communes;

    std::string name;
    std::string code;
    Geometry geom;
};

static void notice(const char *fmt, ...) {
    std::fprintf( stdout, "NOTICE: ");

    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(stdout, fmt, ap);
    va_end(ap);

    std::fprintf(stdout, "\n");
}



struct Data {
    int nb_threads;
    std::vector<Departement> departements;
    std::map<std::string, std::string> insee_key;
    std::map<std::string, std::string> key_url;
    log4cplus::Logger logger;
    GEOSContextHandle_t geos_handle;

    bool load_departements(const std::string & file_name){
        GEOSWKTReader * wkt_reader = GEOSWKTReader_create_r(geos_handle);
        CsvReader reader_dep(file_name, ',');

        std::vector<std::string> line = reader_dep.next();

        // WKT,ID_GEOFLA,CODE_COMM,INSEE_COM,NOM_COMM,STATUT,X_CHF_LIEU,Y_CHF_LIEU,X_CENTROID,Y_CENTROID,Z_MOYEN,SUPERFICIE,POPULATION,CODE_CANT,CODE_ARR,CODE_DEPT,NOM_DEPT,CODE_REG,NOM_REGION
        if(line.size() != 12){
            LOG4CPLUS_ERROR(logger, "Pas le bon nombre de colonnes pour les départements");
            return false;
        }

        if(line[0] != "WKT" || line[2] != "CODE_DEPT" || line[3] != "NOM_DEPT") {
            LOG4CPLUS_ERROR(logger, "Les colonnes ne correspondent pas pour les départements");
            return false;
        }

        line = reader_dep.next();
        while(!reader_dep.eof()) {
            Departement dep;
            dep.code = line[2];
            dep.name = line[3];
            dep.geom.geom = GEOSWKTReader_read_r(geos_handle, wkt_reader, line[0].c_str());
            departements.push_back(std::move(dep));
            line = reader_dep.next();
        }
        GEOSWKTReader_destroy_r(geos_handle, wkt_reader);
        return true;
    }

    bool load_communes(const std::string & file_name){
        GEOSWKTReader * wkt_reader = GEOSWKTReader_create_r(geos_handle);
        CsvReader reader(file_name, ',');
        std::vector<std::string> line = reader.next();
        if(line.size() != 19){
            return false;
        }

        line = reader.next();
        while(!reader.eof()) {
            Commune c;
            c.name = line[4];
            c.population = boost::lexical_cast<float>(boost::trim_copy(line[12])) * 1000;
            c.insee = line[3];
            c.geom.geom = GEOSWKTReader_read_r(geos_handle, wkt_reader, line[0].c_str());
            std::string code_dep = line[15];
            for(Departement & dep : departements){
                if(dep.code == code_dep){
                    dep.communes.push_back(std::move(c));
                    break;
                }
            }
            line = reader.next();
        }
        GEOSWKTReader_destroy_r(geos_handle, wkt_reader);
        return true;
    }

    bool load_insee_key(const std::string & file_name){
        CsvReader reader(file_name, ',');
        std::vector<std::string> line = reader.next(); // en-tête qu'on ignore
        line = reader.next();
        while(!reader.eof()){
            if(line.size() < 2){
                LOG4CPLUS_ERROR(logger, "Pas le bon nombre de colonnes pour les correspondances insee clef");
                return false;
            }
            insee_key[line[0]] = line[1];
            line = reader.next();
        }
        return true;
    }

    bool load_key_url(const std::string & file_name){
        CsvReader reader(file_name, ',');
        std::vector<std::string> line = reader.next();// en-tête qu'on ignore
        line = reader.next();
        while(!reader.eof()){
            if(line.size() != 2){
                LOG4CPLUS_ERROR(logger, "Pas le bon nombre de colonnes pour les correspondances clef url");
                return false;
            }
            key_url[line[0]] = line[1];
            line = reader.next();
        }
        return true;
    }

    Data() : nb_threads(8){
        if(Configuration::is_instanciated()){
            geos_handle = initGEOS_r(NULL, NULL);
            init_logger();
            bool ok = true;

            logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
            LOG4CPLUS_DEBUG(logger, "Chargement de l'application");
            Configuration * conf = Configuration::get();
            std::string conf_file = conf->get_string("path") + conf->get_string("application") + ".ini";
            LOG4CPLUS_DEBUG(logger, "On tente de charger le fichier de configuration pour les logs : " + conf_file);
            init_logger(conf_file);
            LOG4CPLUS_DEBUG(logger, "On tente de charger le fichier de configuration général : " + conf_file);
            conf->load_ini(conf_file);

            nb_threads = conf->get_as<int>("GENERAL", "nb_threads", 1);
            LOG4CPLUS_TRACE(logger, std::string("nb_threads =  ") + boost::lexical_cast<std::string>(nb_threads));

            if(curl_global_init(CURL_GLOBAL_NOTHING) != 0){
                LOG4CPLUS_FATAL(logger, "erreurs lors de l'initialisation de CURL");
                exit(1);
            }

            LOG4CPLUS_TRACE(logger, "Initialisation de GEOS");
            initGEOS(notice, notice);

            std::string departements_filename = conf->get_as<std::string>("GENERAL", "departements", "DEPARTEMENT.csv");
            LOG4CPLUS_TRACE(logger, "On tente de charger le fichier des départements : " + departements_filename);
            ok = ok && load_departements(departements_filename);

            std::string communes_filename = conf->get_as<std::string>("GENERAL", "communes", "COMMUNE.csv");
            LOG4CPLUS_TRACE(logger, "On tente de charger le fichier des communes : " + communes_filename);
            ok = ok && load_communes(communes_filename);

            std::string insee_key_filename = conf->get_as<std::string>("GENERAL", "insee_key", "insee_key.csv");
            LOG4CPLUS_TRACE(logger, "Chargement du fichier de correspondance insee/clef : " + insee_key_filename);
            ok = ok && load_insee_key(insee_key_filename);

            std::string key_url_filename = conf->get_as<std::string>("GENERAL", "key_url", "key_url.csv");
            LOG4CPLUS_TRACE(logger, "Chargement du fichier de correspondance clef/url : " + key_url_filename);
            ok = ok && load_key_url(key_url_filename);

            if(!ok){
                LOG4CPLUS_FATAL(logger, "Il y a eu un problème de chargement des données. J'abandonne");
                exit(1);
            }

            LOG4CPLUS_DEBUG(logger, "Initialisation OK");
        }

    }

    ~Data(){
        finishGEOS_r(geos_handle);
    }
};

size_t curl_callback( char *ptr, size_t size, size_t nmemb, void *userdata){
    std::stringstream * ss = static_cast<std::stringstream*>(userdata);
    ss->write(ptr, size * nmemb);
    return size * nmemb;
}

struct Worker : public BaseWorker<Data> {
    log4cplus::Logger logger;

    GEOSContextHandle_t geos_handle;

    std::string find(std::vector<Departement> & departements, double lon, double lat) {
        GEOSCoordSequence *s = GEOSCoordSeq_create_r(geos_handle, 1, 2);
        GEOSCoordSeq_setX_r(geos_handle, s, 0, lon);
        GEOSCoordSeq_setY_r(geos_handle, s, 0, lat);
        Geometry p;
        p.geom = GEOSGeom_createPoint_r(geos_handle, s);
        for(auto &d : departements) {
            if(d.geom.contains(geos_handle, p)) {
                for(auto &c : d.communes){
                    if(c.geom.contains(geos_handle, p)) {
                        return c.insee;
                    }
                }
            }
        }
        return "";
    }


    ResponseData planner(RequestData req, Data & d) {


        ResponseData rd;
        rd.status_code = 400;
        rd.content_type = "text/plain";

        if(!req.params_are_valid){
            rd.response << "Paramètres invalides";
            return rd;
        }

        navitia::type::EntryPoint origin(boost::get<std::string>(req.parsed_params["origin"].value));
        navitia::type::EntryPoint destination(boost::get<std::string>(req.parsed_params["destination"].value));

        std::string insee_departure = find(d.departements, origin.coordinates.x, origin.coordinates.y);
        std::string insee_destination = find(d.departements, destination.coordinates.x, destination.coordinates.y);

        if(insee_departure.empty() || insee_destination.empty()){
            if(insee_departure.empty())
                rd.response << "J'ai pas trouvé de commune de début\n";
            if(insee_destination.empty())
                rd.response << "J'ai pas trouvé de commune de destination\n";
            rd.response << "N'auriez-vous pas confondu longitude avec latitude ?\n";
            return rd;
        }

        if(d.insee_key.find(insee_departure) == d.insee_key.end()){
            rd.response << "On a pas de clef associée à la commune de départ " << insee_departure;
            return rd;
        }

        if(d.insee_key.find(insee_destination) == d.insee_key.end()){
            rd.response << "On a pas de clef associée à la commune de destination " << insee_destination;
            return rd;
        }

        if(d.insee_key.at(insee_departure) != d.insee_key.at(insee_destination)){
            rd.response << "Le départ et l'arrivée ne sont pas dans la même zone : " << d.insee_key.at(insee_departure) << " et " << d.insee_key.at(insee_destination);
            return rd;
        }

        if(d.insee_key.at(insee_departure).empty()){
            rd.response << "Les communes ne sont pas couvertes";
            return rd;
        }

        if(d.key_url.find(d.insee_key.at(insee_departure)) == d.key_url.end()){
            rd.status_code = 500;
            rd.response << "Pas d'URL associée à la clef : " << d.insee_key.at(insee_departure);
            return rd;
        }

        size_t pos = req.path.find_last_of('/');
        if(pos == std::string::npos)
            pos = 0;
        std::string url = d.key_url.at(d.insee_key.at(insee_departure)) + req.path.substr(pos) + "?" + req.raw_params;


        CURL *handle = curl_easy_init();
        curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
        curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 5);
        curl_easy_setopt(handle, CURLOPT_TIMEOUT, 60);
        curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);
        curl_easy_setopt(handle, CURLOPT_WRITEDATA, static_cast<void*>(&rd.response));
        curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, curl_callback);

        if(!req.data.empty()){
            curl_easy_setopt(handle, CURLOPT_POST, 1);
            curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, req.data.size());
            curl_easy_setopt(handle, CURLOPT_POSTFIELDS, req.data.c_str());
        }

        LOG4CPLUS_TRACE(logger, "Execution de la requête : " + url);
        if(curl_easy_perform(handle) != 0){
            LOG4CPLUS_ERROR(logger, "Erreur lors de la requête curl : " + url);
            rd.status_code = 500;
        }
        curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &rd.status_code);
        char **content_type = NULL;
        if(content_type != NULL)
            rd.content_type.assign(*content_type);

        curl_easy_cleanup(handle);

        return rd;
    }


    ResponseData status(RequestData, const Data & data){
        ResponseData rd;
        rd.status_code = 200;
        rd.content_type = "application/json";

        int nb_communes = 0;
        for(const Departement & dep : data.departements){
            nb_communes += dep.communes.size();
        }

        rd.response << "{ \"status\": {\n"
                    << "    \"nb_departments\" : \"" << data.departements.size() << "\",\n"
                    << "    \"nb_communes\": \"" << nb_communes << "\",\n"
                    << "    \"nb_urls\": \"" << data.key_url.size() << "\"\n"
                    << "}}";
        return rd;
    }

    public:
    /** Constructeur par défaut
      *
      * On y enregistre toutes les api qu'on souhaite exposer
      */
    Worker(Data &) {
        //@TODO: passer des fonctions de log
        geos_handle = initGEOS_r(NULL, NULL);

        logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
        register_api("journeys",boost::bind(&Worker::planner, this, _1, _2), "Effectue un calcul d'itinéraire entre deux coordonnées");
        add_param("journeys", "origin", "EntryPoint (coordonnées) de départ", ApiParameter::STRING, true);
        add_param("journeys", "destination", "EntryPoint (coordonnées) de destination", ApiParameter::STRING, true);

        register_api("journeysarray",boost::bind(&Worker::planner, this, _1, _2), "Effectue un calcul d'itinéraire entre deux coordonnées");
        add_param("journeysarray", "origin", "EntryPoint (coordonnées) de départ", ApiParameter::STRING, true);
        add_param("journeysarray", "destination", "EntryPoint (coordonnées) de destination", ApiParameter::STRING, true);

        register_api("status",boost::bind(&Worker::status, this, _1, _2), "Informations sur les données chargées");

        add_default_api();
    }

    ~Worker(){
        finishGEOS_r(geos_handle);
    }
};

MAKE_WEBSERVICE(Data, Worker)
