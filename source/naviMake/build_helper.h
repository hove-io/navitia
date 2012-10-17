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
    VJ(builder & b, const std::string &line_name, const std::string &validity_pattern, const std::string & block_id);

    /// Ajout un nouveau stopTime
    /// Lorsque le depart n'est pas specifié, on suppose que c'est le même qu'à l'arrivée
    /// Si le stopPoint n'est pas connu, on le crée avec un stopArea ayant le même nom
    VJ& operator()(const std::string &stopPoint, int arrivee, int depart = -1);
};

struct SA {
    builder & b;
    types::StopArea * sa;

    /// Construit un nouveau stopArea
    SA(builder & b, const std::string & sa_name, double x, double y);

    /// Construit un stopPoint appartenant au stopArea courant
    SA & operator()(const std::string & sp_name, double x = 0, double y = 0);
};



struct builder{
    std::map<std::string, types::Line *> lines;
    std::map<std::string, types::ValidityPattern *> vps;
    std::map<std::string, types::StopArea *> sas;
    std::map<std::string, types::StopPoint *> sps;
    boost::gregorian::date begin;

    Data data;
    //navitia::streetnetwork::StreetNetwork street_network;
    navitia::georef::GeoRef street_network;


    /// Il faut préciser là date de début des différents validity patterns
    builder(const std::string & date) : begin(boost::gregorian::date_from_iso_string(date)) {}

    /// Crée un vehicle journey
    VJ vj(const std::string &line_name, const std::string &validity_pattern = "11111111", const std::string & block_id="");

    /// Crée un nouveau stop area
    SA sa(const std::string & name, double x = 0, double y = 0);
    SA sa(const std::string & name, navitia::type::GeographicalCoord geo){return sa(name,geo.x, geo.y);}

    /// Crée une connexion
    void connection(const std::string & name1, const std::string & name2, float length);
    void build(navitia::type::PT_Data & pt_data);
};

}
