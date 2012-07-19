#include "to_proto.h"
#include "type/to_proto.h"
#include "boost/foreach.hpp"
namespace itineraires {

void to_proto(etape et, pbnavitia::Parcours_Etape *etpb, navitia::type::Data &data) {
    etpb->set_mode(et.mode);
    etpb->set_descente(data.pt_data.stop_areas.at(et.descente).name);

    BOOST_FOREACH(idx_t ligne, et.lignes) {
        etpb->add_lignes(ligne);
    }
}

void to_proto(uint32_t idpa, parcours pa, pbnavitia::Parcours *papb, navitia::type::Data &data) {
    papb->set_id(idpa);
    BOOST_FOREACH(itineraires::etape e, pa.etapes) {
        to_proto(e, papb->add_etapes(), data);
    }
}

void to_proto(std::map<uint32_t, itineraires::parcours> &map_parcours, pbnavitia::ReponseDemonstrateur *rppb, navitia::type::Data &data) {
    typedef std::map<uint32_t, itineraires::parcours> map_pa;
    BOOST_FOREACH(map_pa::value_type pa, map_parcours) {
        to_proto(pa.first, pa.second, rppb->add_parcours_liste(), data);
    }
}

void to_proto(itineraire it, navitia::type::Data &data, pbnavitia::Itineraire *itpb) {

    itpb->set_parcours(it.parcours);
    unsigned int count = 0;
    pbnavitia::StopTimeEtape *ste;
    BOOST_FOREACH(uint32_t st, it.stop_times) {
        if((count%2) == 0) {
            ste = itpb->add_stop_times();
            navitia::type::to_proto(data.pt_data.stop_times.at(st), ste->mutable_depart());
        } else {
            navitia::type::to_proto(data.pt_data.stop_times.at(st), ste->mutable_arrivee());
        }
        count ++;

    }
}
}
