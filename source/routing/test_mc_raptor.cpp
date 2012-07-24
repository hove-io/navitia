#include "mcraptor.h"
#include "mcraptor.cpp"
#include "naviMake/build_helper.h"
using namespace navitia::routing;

struct FootPathCritere : public raptor::mcraptor::label_parent {

    int distodest;
    int footpathlength;

    FootPathCritere() : raptor::mcraptor::label_parent(), distodest(std::numeric_limits<int>::max()), footpathlength(std::numeric_limits<int>::max()){}


    FootPathCritere(const FootPathCritere & fpc)  : raptor::mcraptor::label_parent(fpc), distodest(fpc.distodest), footpathlength(fpc.footpathlength) {}

    FootPathCritere(raptor::type_retour t) {
        this->dt = t.dt;
        this->distodest = t.dist_to_dest;
        this->footpathlength = t.dist_to_dep;
        if(this->distodest  > 0)
            std::cout << " dist to dest " << this->distodest << std::endl;
    }

    bool dominated_by(const FootPathCritere &lbl) const {
        return (lbl.dt < this->dt) && (lbl.footpathlength < this->footpathlength);
    }

    void update(DateTime &v, int stid) {
        this->dt = v + distodest;
        this->dt.normalize();
        if(this->distodest > 0)
            std::cout << " foot path length : " << this->footpathlength << " " << this->distodest;
        this->footpathlength += this->distodest;
        this->stid = stid;
    }

    void ajouterfootpath(int duration) {
        dt.hour = dt.hour + duration;
        dt.normalize();
        footpathlength += duration;
    }



};
std::ostream& operator<<( std::ostream &out, FootPathCritere const& fpc ) {
    out << fpc.dt << " footpath length : " <<  fpc.footpathlength;
    return out;
}
struct FootPathCritereVisitor {
    navitia::type::Data &d;

    FootPathCritereVisitor(navitia::type::Data &d) : d(d) {}

    bool dominated_by(const FootPathCritere &dt1, const FootPathCritere &dt2) {
        return dt1.dominated_by(dt2);
    }

    void update(const unsigned int order, FootPathCritere &dtc) {
        if(dtc.vjid != -1) {
            unsigned int stid = d.pt_data.vehicle_journeys.at(dtc.vjid).stop_time_list.at(order);
            DateTime ndt(dtc.dt.date, d.pt_data.stop_times.at(stid).arrival_time);
            dtc.update(ndt, stid);
        }
    }

    void keepthebest(FootPathCritere lbl, FootPathCritere& lbl2) {
        if(lbl.dt < lbl2.dt)
            lbl2.dt = lbl.dt;
        if(lbl.footpathlength < lbl2.footpathlength)
            lbl2.footpathlength = lbl.footpathlength;
    }

    void updateDestination(const FootPathCritere destination, FootPathCritere & lbl) {

        lbl.footpathlength += destination.distodest;
    }
};


int main(int, char **) {

    navitia::type::Data d;
//    d.load_lz4("/home/vlara/navitia/jeu/poitiers/poitiers.nav");

    navitia::streetnetwork::GraphBuilder bsn(d.street_network);

    navitia::type::GeographicalCoord A(0,0);
    navitia::type::GeographicalCoord B(0.0050,0.0050);
    navitia::type::GeographicalCoord C(0.020,0.010);
    navitia::type::GeographicalCoord D(0.030,0.010, false);
    navitia::type::GeographicalCoord E(0.000100,-0.00010, false);

    navitia::type::GeographicalCoord Z(0.0040,0, false);

    bsn("A", A)("B", B)("C", C)("D", D)("E", E)("Z", Z);
    bsn("A", "B")("B","A")("A", "E")("E", "A")("C","C")("D","D")("Z","Z");


    navimake::builder b("20120614");
    b.sa("B", B);
    b.sa("C", C);
    b.sa("D", D);
    b.sa("E", E);
    b.sa("Z", Z);
    b.vj("t1")("B", 8*3600 + 60*10)("C", 8*3600 + 15*60);
    b.vj("t2")("D", 8*3600 + 60*30)("Z", 8*3600 + 45*60);
    b.vj("t3")("E", 8*3600 + 60*10)("Z", 9*3600 + 30*60);
    b.connection("C", "D", 10);

    d.pt_data = b.build();
    d.build_proximity_list();

    std::cout << "Distance A à B" << A.distance_to(B) <<std::endl;

    raptor::mcraptor::McRAPTOR<FootPathCritere, FootPathCritereVisitor> dtraptor(d, FootPathCritereVisitor(d));
    dtraptor.compute(A, 10000, Z, 1000, 7*3600, 0);
}
