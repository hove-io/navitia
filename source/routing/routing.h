#pragma once

#include "type/pt_data.h"
#include "type/datetime.h"

namespace navitia { namespace routing {
    using type::idx_t;

struct NotFound{};




/** Représente un horaire associé à un validity pattern
 *
 * Il s'agit donc des horaires théoriques
 */
struct ValidityPatternTime {
    idx_t vp_idx;
    int hour;

    template<class T>
    bool operator<(T other) const {
        return hour < other.hour;
    }

    ValidityPatternTime() : vp_idx(type::invalid_idx), hour(-1) {}
    ValidityPatternTime(int vp_idx, int hour) : vp_idx(vp_idx), hour(hour){}
};


enum ItemType {
    public_transport,
    walking,
    stay_in,
    guarantee,
    waiting
};

/** Étape d'un itinéraire*/
struct PathItem {
    navitia::DateTime arrival;
    navitia::DateTime departure;
    std::vector<navitia::DateTime> arrivals;
    std::vector<navitia::DateTime> departures;
    std::vector<const nt::StopTime*> stop_times; //empty if not public transport

    /**
     * if public transport, contains the list of visited stop_point
     * for transfers, contains the departure and arrival stop points
     */
    std::vector<const navitia::type::StopPoint*> stop_points;

    const navitia::type::StopPointConnection* connection;

    ItemType type;

    boost::gregorian::date date; //TODO a virer, et remplacer DateTime par des boost::date pour avoir cette date.

    PathItem(navitia::DateTime departure = navitia::DateTimeUtils::inf,
             navitia::DateTime arrival = navitia::DateTimeUtils::inf) :
        arrival(arrival), departure(departure), type(public_transport) {
    }

    //VIRER ce constructeur des qu'on a vire date
    PathItem(navitia::DateTime departure,
             navitia::DateTime arrival, const nt::Data& d) :
        arrival(arrival), departure(departure), type(public_transport) {
        date = navitia::to_posix_time(departure, d).date();
    }

    std::string print() const;

    const navitia::type::VehicleJourney* get_vj() const {
        if (stop_times.empty())
            return nullptr;
        return stop_times.front()->vehicle_journey;
    }
};

/** Un itinéraire complet */
struct Path {
    uint32_t duration;
    uint32_t nb_changes;
    boost::posix_time::ptime request_time;
    std::vector<PathItem> items;

    Path() : duration(std::numeric_limits<uint32_t>::max()), nb_changes(std::numeric_limits<uint32_t>::max()) {}

    void print() const {
        for(auto item : items)
            std::cout << item.print() << std::endl;
    }

};

bool operator==(const PathItem & a, const PathItem & b);


}}


