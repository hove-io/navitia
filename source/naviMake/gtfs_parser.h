#pragma once
#include "data.h"
#include <boost/unordered_map.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <queue>

/** Lit les fichiers au format General Transit Feed Specifications
  *
  * http://code.google.com/intl/fr/transit/spec/transit_feed_specification.html
  */
namespace navimake{ namespace connectors{
class GtfsParser {
private:
    std::string path;///< Chemin vers les fichiers
    boost::gregorian::date start;///< Premier jour où les données sont valables
 //   navimake::Data data;

    // Plusieurs maps pour savoir à quel position est quel objet identifié par son ID GTFS
    boost::unordered_map<std::string, navimake::types::StopPoint*> stop_map;
    boost::unordered_map<std::string, navimake::types::StopArea*> stop_area_map;
    boost::unordered_map<std::string, navimake::types::Route*> route_map;
    boost::unordered_map<std::string, navimake::types::Line*> line_map;
    boost::unordered_map<std::string, navimake::types::ValidityPattern*> vp_map;
    boost::unordered_map<std::string, navimake::types::VehicleJourney*> vj_map;
public:
    /// Constructeur qui prend en paramètre le chemin vers les fichiers
    GtfsParser(const std::string & path, const std::string & start_date);

    /// Constructeur d'une instance vide
    GtfsParser() {}

    /// Remplis la structure passée en paramètre
    void fill(navimake::Data& data);

    /// Parse le fichier calendar_dates.txt
    /// Contient les dates définies par jour (et non par période)
    void parse_calendar_dates(Data & data);

    /// Parse le fichier routes.txt
    /// Contient les routes
    void parse_routes(Data & data);

    /// Parse le fichier stops.txt
    /// Contient les points d'arrêt et les zones d'arrêt
    void parse_stops(Data & data);

    /// Parse le fichier stop_times.txt
    /// Contient les horaires
    void parse_stop_times(Data & data);

    /// Fonction pour faire un traitement multithread de stop_times
   // void parse_stop_times_worker(int id_c, int arrival_c, int departure_c, int stop_c, int stop_seq_c, int pickup_c, int drop_c);
    /// Pour synchroniser l'accès à la structure data
   // boost::mutex data_mutex;
    /// Pour synchroniser l'accès à la queue
   // boost::mutex queue_mutex;
    /// Est-ce qu'on a fini de parser les stop_times
   // bool parse_ended;
    /// La queue des lignes à traiter
   // std::queue<std::string> queue;
    /// La variable qui permet de synchroniser les threads
   // boost::condition_variable cond;


    /// Parse le fichier transfers.txt
    /// Contient les correspondances entre arrêts
    void parse_transfers(Data & data);

    /// Parse le fichier trips.txt
    /// Contient les VehicleJourney
    void parse_trips(Data & data);


 //   inline navimake::Data getData(){return data;}

};

/** Convertit une chaine de charactères du type 8:12:31 en secondes depuis minuit
  *
  * Retourne -1 s'il y a eu un problème
  */
int time_to_int(const std::string & time);
}}
