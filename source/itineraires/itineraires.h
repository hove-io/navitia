#pragma once
#include "network/network.h"
namespace itineraires {
/// Une étape est composée d'une ligne à empreinter et d'un stop area où descentre
class etape {
public:
    idx_t ligne;
    idx_t descente;
    etape(idx_t ligne, idx_t depart) : ligne(ligne), descente(depart){}

    bool operator==(etape e2);
    bool operator!=(etape e2);

};


/// Un parcours est une liste d'étape
class parcours {
public:
    std::list<etape> etapes;

    parcours() {
        etapes = std::list<etape>();
    }

    void ajouter_etape(idx_t ligne, idx_t descente);
    bool operator==(parcours i2);
    bool operator!=(parcours i2);



};


/// Un itinéraire est composé d'un parcours ainsi que d'une liste de stop times correspondant au moment où l'on descend
class itineraire {
public :
    uint32_t parcours;
    std::list<idx_t> stop_times;

    itineraire(uint32_t parcours, std::list<idx_t> stop_times) :parcours(parcours), stop_times(stop_times){}

};

class sort_itineraire {
private:
    navitia::type::Data &data;
public:
    sort_itineraire (navitia::type::Data &data) : data(data) { }
    bool operator() (const itineraire& i1, const itineraire& i2) const {
        if(i1.stop_times.size() == 0 || i2.stop_times.size() == 0)
            return false;
        else
            return  ((data.pt_data.stop_times.at(*(i1.stop_times.begin())).departure_time == data.pt_data.stop_times.at(*(i2.stop_times.begin())).departure_time)
                     &(data.pt_data.stop_times.at(*(--i1.stop_times.end())).departure_time < data.pt_data.stop_times.at(*(--i2.stop_times.end())).departure_time))
                    || data.pt_data.stop_times.at(*(i1.stop_times.begin())).departure_time < data.pt_data.stop_times.at(*(i2.stop_times.begin())).departure_time;

    }
};

typedef std::map<uint32_t, parcours> map_parcours;

void make_itineraires(network::vertex_t v1, network::vertex_t v2, std::vector<itineraire> &itineraires, map_parcours &parcours_list, network::NW &g, navitia::type::Data &data, std::vector<network::vertex_t> &predecessors);

}
