#include <geos_c.h>
#include "utils/csv.h"
#include <stdarg.h>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>


#include <random>
#include "utils/timer.h"

/* On obtient les communes à partir de la base GEOFLA de l'insee
 *
 * On les converti en CSV avec ogr2ogr du paquet GDAL :
 * ogr2ogr -f csv communes COMMUNE.SHP -lco GEOMETRY=AS_WKT -s_srs EPSG:2154 -t_srs EPSG:4326
 * ogr2ogr -f csv communes COMMUNE.SHP -lco GEOMETRY=AS_WKT
 *
 */

struct Geometry {
    GEOSGeometry * geom;

    Geometry() : geom(nullptr) {}

    Geometry(const Geometry & other) {
        if(other.geom != nullptr)
            this->geom = GEOSGeom_clone(other.geom);
        else
            this->geom = nullptr;
    }

    Geometry(Geometry && other) {
        this->geom = other.geom;
        other.geom = nullptr;
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

    ~Geometry() {
        if(geom != nullptr)
            GEOSGeom_destroy(geom);
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

std::string find(const std::vector<Departement> & departements, double lon, double lat) {
    GEOSCoordSequence *s = GEOSCoordSeq_create(1, 2);
    GEOSCoordSeq_setX(s, 0, lon);
    GEOSCoordSeq_setY(s, 0, lat);
    GEOSGeometry *p = GEOSGeom_createPoint(s);
    for(auto& d : departements) {
        if(GEOSContains(d.geom.geom, p)) {
            for(auto& c : d.communes){
                if(GEOSContains(c.geom.geom, p)) {
                    GEOSGeom_destroy(p);
                    return c.name;
                }
            }
        }
    }
    GEOSGeom_destroy(p);
    return "";
}


int main(int argc, char**) {
    initGEOS(notice, notice);


    CsvReader reader_dep("DEPARTEMENT.csv", ',');
    
    std::vector<std::string> line = reader_dep.next();

    // WKT,ID_GEOFLA,CODE_COMM,INSEE_COM,NOM_COMM,STATUT,X_CHF_LIEU,Y_CHF_LIEU,X_CENTROID,Y_CENTROID,Z_MOYEN,SUPERFICIE,POPULATION,CODE_CANT,CODE_ARR,CODE_DEPT,NOM_DEPT,CODE_REG,NOM_REGION
    if(line.size() != 12){
        std::cerr << "Pas le bon nombre de colonnes pour les départements" << std::endl;
        return 1;
    }

    if(line[0] != "WKT" || line[2] != "CODE_DEPT" || line[3] != "NOM_DEPT") {
        std::cerr << "Les colonnes ne correspondent pas pour les départements" << std::endl;
        return 2;
    }

    std::vector<Departement> departements;
    GEOSWKTReader * wkt_reader = GEOSWKTReader_create();
    std::cout << "Lecture des départements" << std::endl;
    line = reader_dep.next();
    while(!reader_dep.eof()) {
        Departement dep;
        dep.code = line[2];
        dep.name = line[3];
        dep.geom.geom = GEOSWKTReader_read(wkt_reader, line[0].c_str());
        departements.push_back(std::move(dep));
        line = reader_dep.next();
    }

    CsvReader reader("COMMUNE.csv", ',');
    line = reader.next();
    if(line.size() != 19){
        std::cerr << "Pas le bon nombre de colonnes pour les communes" << std::endl;
        return 3;
    }

    std::cout << "Lecture des communes" << std::endl;
    line = reader.next();
    while(!reader.eof()) {
        Commune c;
        c.name = line[4];
        c.population = boost::lexical_cast<float>(boost::trim_copy(line[12])) * 1000;
        c.insee = line[3];
        c.geom.geom = GEOSWKTReader_read(wkt_reader, line[0].c_str());
        std::string code_dep = line[15];
        for(Departement & dep : departements){
            if(dep.code == code_dep){
                dep.communes.push_back(c);
                break;
            }
        }
        line = reader.next();
    }
    
  /*  std::cout << "Tri des communes" << std::endl;
    std::sort(communes.begin(), communes.end(), [](const Commune & a, const Commune & b){return a.population > b.population;});
*/
    std::cout << "Fini!" << std::endl;

    for(auto d: departements){
        std::cout << d.name << " " << d.communes.size() << std::endl;
    }
    // Si on a pas d'arguments on lance des benchs
    if(argc == 1) {
        Timer t("Benchs retrouvailles");
        std::mt19937 rng;
        rng.seed(time(NULL));
        std::uniform_real_distribution<> dist_lon(-1,6);
        std::uniform_real_distribution<> dist_lat(44,49);
        for(int i = 0; i < 1000; ++i){
            double lon = dist_lon(rng);
            double lat = dist_lat(rng);

            std::string commune = find(departements, lon, lat);
           // std::cout << "http://www.openstreetmap.org/?zoom=16&lon=" << lon << "&lat=" << lat << " " << commune << std::endl;

        }
        Timer t2("Commune morte pour la france");
        std::string commune =  find(departements,5.408333,49.259167);
        std::cout << "http://www.openstreetmap.org/?zoom=16&lon=" << 5.408333 << "&lat=" << 49.259167 << " " << commune << std::endl;
    }
    GEOSWKTReader_destroy(wkt_reader);
    return 0;
}
