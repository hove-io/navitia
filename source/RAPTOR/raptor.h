#pragma once
#include "type/type.h"
#include "type/data.h"
#include <unordered_map>

namespace raptor {

struct type_retour {
    int stid,
        temps,
        jour;
    type_retour(int stid, int temps, int jour) : stid(stid), temps(temps), jour(jour) {}
    type_retour() : stid(-1), temps(-1), jour(-1) {}

    bool operator<(type_retour r2) const { return this->jour < r2.jour ||((this->jour == r2.jour) && (this->temps < r2.temps));}
};

struct type_best {
    int jour,
        temps;
    type_best(int jour, int temps) : jour(jour), temps(temps) {}
    type_best() : jour(-1), temps(-1) {}

    bool operator<(type_best b2) const {
        if(this->jour < b2.jour)
            return true;
        else if(this->jour == b2.jour)
            return (this->temps < b2.temps);
        else return false;
        /*return this->jour < b2.jour ||((this->jour == b2.jour) && (this->temps < b2.temps));*/}
};

typedef std::pair<int, int> pair_int;
typedef std::unordered_map<int, int> map_int_int_t;
typedef std::unordered_map<int, type_retour> map_int_pint_t;
typedef std::unordered_map<int, type_best> map_best_t;

typedef std::unordered_map<unsigned int, map_int_pint_t> map_retour_t;
typedef std::pair<map_retour_t, map_best_t> pair_retour_t;
typedef std::vector<type_retour> itineraire_t;

std::pair<unsigned int, bool> earliest_trip(unsigned int route, unsigned int stop_area, map_retour_t &retour, navitia::type::Data &data);
int get_rp_order(navitia::type::Route &route, unsigned int stop_area, navitia::type::Data &data);
int get_rp_order(unsigned int route, unsigned int stop_area, navitia::type::Data &data);
int get_rp_id(navitia::type::Route &route, unsigned int stop_area, navitia::type::Data &data);
int get_rp_id(unsigned int route, unsigned int stop_area, navitia::type::Data &data);
int get_sa_rp(unsigned int order, int route, navitia::type::Data & data);


map_int_int_t make_queue(std::vector<unsigned int> stops, navitia::type::Data &data) ;

pair_retour_t RAPTOR(unsigned int depart, int arrivee, int debut, int date, navitia::type::Data &data);
void afficher_chemin(unsigned int depart, int arrivee, pair_retour_t &pair_retour, unsigned int corr, navitia::type::Data &data);
itineraire_t make_itineraire(unsigned int depart, int arrivee, pair_retour_t &pair_retour, unsigned int corr, navitia::type::Data &data);
void afficher_itineraire(unsigned int depart, int arrivee, pair_retour_t &pair_retour, unsigned int corr, navitia::type::Data &data);

struct compare_rp {
    navitia::type::RoutePoint & rp;
    navitia::type::Data &data;
    compare_rp(navitia::type::RoutePoint & rp, navitia::type::Data &data) : rp(rp), data(data) {}

    bool operator ()(unsigned int vj1, int time) {
        return (data.pt_data.stop_times.at(data.pt_data.vehicle_journeys.at(vj1).stop_time_list.at(rp.order)).departure_time %86400) < time;
    }
};


}
