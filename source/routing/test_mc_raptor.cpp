#include "mcraptor.h"
#include "mcraptor.cpp"
#include "naviMake/build_helper.h"
using namespace navitia::routing;

struct FootPathCritere : public raptor::mcraptor::label_parent {

    int distodest;
    int footpathlength;

    FootPathCritere() : raptor::mcraptor::label_parent(){}


    FootPathCritere(const FootPathCritere & dt)  : raptor::mcraptor::label_parent(dt) {}

    FootPathCritere(raptor::type_retour t) {
        dt = t.dt;
        distodest = t.dist_to_dest;
        footpathlength = distodest;
    }

    bool dominated_by(const FootPathCritere &lbl) const {
        return (lbl.dt < this->dt) && (lbl.footpathlength < this->footpathlength);
    }

    void update(DateTime &v, int stid) {
        dt = v + distodest;
        dt.normalize();
        stid = stid;
    }

    void ajouterfootpath(int duration) {
        dt.hour = dt.hour + duration;
        dt.normalize();
        footpathlength += duration;
    }

};

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
};


int main(int, char **) {

    navitia::type::Data d;
//    d.load_lz4("/home/vlara/navitia/jeu/poitiers/poitiers.nav");

    navitia::streetnetwork::GraphBuilder bsn(d.street_network);

    navitia::type::GeographicalCoord A(0,0);
    navitia::type::GeographicalCoord B(10,10);
    navitia::type::GeographicalCoord C(-10,-10);
    navitia::type::GeographicalCoord Z(20,0);

    bsn("A", 0,0)("B", 10,10)("C", -10,-10)("Z", 20,0);
    bsn("A", "B")("B", "A")("A", "C")("C", "A")("C", "Z")("Z", "C");




    navimake::builder b("20120614");
    b.sa("B", B);
    b.sa("C", C);
    b.sa("Z", Z);
    b.vj("t1")("B", 8*3600 + 60*20)("Z", 8*3600 + 30*60);
    b.vj("t2")("C", 8*3600 + 60*10)("Z", 9*3600 + 30*60);
    d.pt_data = b.build();
    d.build_proximity_list();


    raptor::mcraptor::McRAPTOR<FootPathCritere, FootPathCritereVisitor> dtraptor(d, FootPathCritereVisitor(d));
    dtraptor.compute(navitia::type::GeographicalCoord(0,0), 500000, navitia::type::GeographicalCoord(10,0), 500000, 7*3600, 0);


}
