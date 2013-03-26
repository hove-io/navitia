#pragma once

#include "data.h"
#include <boost/date_time/gregorian_calendar.hpp>

/** Ce connecteur permet de faciliter la construction d'un réseau en code
 *
 *  Cela est particulièrement (voire exclusivement) utile pour les tests unitaires
 */

namespace navimake {

struct builder;

/// Structure retournée à la construction d'un VehicleJourney
struct VJ {
    builder & b;
    types::VehicleJourney * vj;

    /// Construit un nouveau vehicle journey
    VJ(builder & b, const std::string &line_name, const std::string &validity_pattern, const std::string & block_id, bool is_adapted = true, const std::string& uri="");
    VJ(builder & b, const std::string &network_name, const std::string &line_name, const std::string &validity_pattern, const std::string & block_id, bool is_adapted = true, const std::string& uri="");

    /// Ajout un nouveau stopTime
    /// Lorsque le depart n'est pas specifié, on suppose que c'est le même qu'à l'arrivée
    /// Si le stopPoint n'est pas connu, on le crée avec un stopArea ayant le même nom
    VJ& operator()(const std::string &stopPoint, int arrivee, int depart = -1, uint32_t local_traffic_zone = std::numeric_limits<uint32_t>::max(),
                   bool drop_off_allowed = true, bool pick_up_allowed = true);

    VJ& operator()(const std::string &stopPoint, const std::string& arrivee, const std::string& depart,
            uint32_t local_traffic_zone = std::numeric_limits<uint32_t>::max(), bool drop_off_allowed = true, bool pick_up_allowed = true);

    /// Transforme les horaires en horaire entre start_time et end_time, toutes les headways secs
    VJ & frequency(uint32_t start_time, uint32_t end_time, uint32_t headway_secs);

};

struct SA {
    builder & b;
    types::StopArea * sa;

    /// Construit un nouveau stopArea
    SA(builder & b, const std::string & sa_name, double x, double y, bool is_adapted = true);

    /// Construit un stopPoint appartenant au stopArea courant
    SA & operator()(const std::string & sp_name, double x = 0, double y = 0, bool is_adapted = true);
};


struct builder{
    std::map<std::string, types::Line *> lines;
    std::map<std::string, types::ValidityPattern *> vps;
    std::map<std::string, types::StopArea *> sas;
    std::map<std::string, types::StopPoint *> sps;
    std::map<std::string, types::Network *> nts;
    boost::gregorian::date begin;

    Data data;
    //navitia::streetnetwork::StreetNetwork street_network;
    navitia::georef::GeoRef street_network;


    /// Il faut préciser là date de début des différents validity patterns
    builder(const std::string & date) : begin(boost::gregorian::date_from_iso_string(date)) {}

    /// Crée un vehicle journey
    VJ vj(const std::string &line_name, const std::string &validity_pattern = "11111111", const std::string & block_id="", const bool wheelchair_boarding = true, const std::string& uri="");
    VJ vj(const std::string &network_name, const std::string &line_name, const std::string &validity_pattern = "11111111", const std::string & block_id="", const bool wheelchair_boarding = true, const std::string& uri="");

    /// Crée un nouveau stop area
    SA sa(const std::string & name, double x = 0, double y = 0, const bool is_adapted = true);
    SA sa(const std::string & name, navitia::type::GeographicalCoord geo, bool is_adapted = true ){return sa(name,geo.lon(), geo.lat(), is_adapted);}

    /// Crée une connexion
    void connection(const std::string & name1, const std::string & name2, float length);
    void build(navitia::type::PT_Data & pt_data);
    void generate_dummy_basis();
};

}
