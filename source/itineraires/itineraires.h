#pragma once
#include "network/network.h"
namespace itineraires {
/// Une étape est composée d'une ligne à empreinter et d'un stop area où descentre
class etape {
public:
    idx_t mode;
    idx_t descente;
    std::set<idx_t> lignes;
    etape(idx_t mode, idx_t depart, idx_t ligne) : mode(mode), descente(depart){lignes = std::set<idx_t>(); lignes.insert(lignes.begin(), ligne);}

    void ajouter_ligne(idx_t &ligne);
    bool operator==(etape const & e2) const;
    bool operator!=(etape const & e2) const;
};
typedef std::vector<etape> vector_etapes;

/// Un parcours est une liste d'étape
class parcours {
public:
    vector_etapes etapes;

    parcours() {}

    void ajouter_etape(idx_t mode, idx_t descente, idx_t ligne);
    void ajouter_ligne(idx_t mode, idx_t descente, idx_t ligne);
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

void make_itineraires(network::vertex_t v1, network::vertex_t v2, std::vector<itineraire> &itineraires, map_parcours &parcours_list, network::NW &g, navitia::type::Data &data, std::vector<network::vertex_t> &predecessors, network::map_tc_t &map_tc);

}
