#include "mcraptor.h"
#include "mcraptor.cpp"
#include "naviMake/build_helper.h"
using namespace navitia::routing;





struct DateTimeCritere : public raptor::mcraptor::label_parent<DateTime> {


    DateTimeCritere() : raptor::mcraptor::label_parent<DateTime>(){}


    DateTimeCritere(const DateTimeCritere & dt)  : raptor::mcraptor::label_parent<DateTime>(dt) {}

    DateTimeCritere(raptor::type_retour t) {
        dt = t.dt;
    }

    bool dominated_by(const DateTimeCritere &lbl) const {
        return lbl.dt < this->dt;
    }

    void update(DateTime &v, int stid) {
        dt = v;
        dt.normalize();
        stid = stid;
    }

    void ajouterfootpath(int duration) {
        dt.hour = dt.hour + duration;
        dt.normalize();
    }

};

struct DateTimeCritereVisitor {
    navitia::type::Data &d;

    DateTimeCritereVisitor(navitia::type::Data &d) : d(d) {}

    bool dominated_by(const DateTimeCritere &dt1, const DateTimeCritere &dt2) {
        return dt1.dominated_by(dt2);
    }

    void update(const unsigned int order, DateTimeCritere &dtc) {
        if(dtc.vjid != -1) {
            unsigned int stid = d.pt_data.vehicle_journeys.at(dtc.vjid).stop_time_list.at(order);
            DateTime ndt(dtc.dt.date, d.pt_data.stop_times.at(stid).arrival_time);
            dtc.update(ndt, stid);
        }
    }

    void keepthebest(DateTimeCritere lbl, DateTimeCritere& lbl2) {
        if(lbl.dt < lbl2.dt)
            lbl2.dt = lbl.dt;
    }
};


int main(int, char **) {

    navitia::type::Data d;
//    d.load_lz4("/home/vlara/navitia/jeu/poitiers/poitiers.nav");

    navimake::builder b("20120614");
    b.vj("A")("stop1", 8000)("stop2", 8100)("stop3", 8200);
    b.vj("B")("stop4", 8000)("stop2", 8200)("stop5", 8300);
    d.pt_data = b.build();


    raptor::mcraptor::McRAPTOR<DateTimeCritere, DateTimeCritereVisitor> dtraptor(d, DateTimeCritereVisitor(d));
    dtraptor.compute(0, 4, 7900, 0);


}
