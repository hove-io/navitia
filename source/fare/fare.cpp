/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.
  
This file is part of Navitia,
    the software to build cool stuff with public transport.
 
Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!
  
LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
   
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.
   
You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
  
Stay tuned using
twitter @navitia 
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "fare.h"

#include <boost/foreach.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

#include "type/datetime.h"

namespace greg = boost::gregorian;

namespace navitia { namespace fare {

static Label next_label(Label label, Ticket ticket, const SectionKey& section) {
    // we save the informations about the last mod used
    label.line = section.line;
    label.mode = section.mode;
    label.network = section.network;


    if (ticket.type == Ticket::ODFare) {
        if(label.stop_area == "" || label.current_type != Ticket::ODFare){ // It's a new OD ticket
            label.stop_area = section.start_stop_area;
            label.zone = section.start_zone;
            label.nb_changes = 0;
            label.start_time = section.start_time;

            ticket.sections.push_back(section);
            label.tickets.push_back(ticket);
        } else { // We got an old ticket
            label.tickets.back().sections.push_back(section);
            label.nb_changes++;
        }

    } else {
        // empty ticket, it is juste a change
        // we have to update the number of changes and the duration with the same ticket
        if (ticket.caption == "" && ticket.value == 0) {
            label.nb_changes++;
        } else {
            // we bought a new ticket
            // we save the global cost, and we reset the number of changes and duration
            if (ticket.value.undefined)
                label.nb_undefined_sub_cost++; //we need to track the number of undefined ticket for the comparison operator
            label.cost += ticket.value;
            label.tickets.push_back(ticket);
            label.nb_changes = 0;
            label.start_time = section.start_time;
            label.stop_area = section.start_stop_area;
        }
        if (label.tickets.size() == 0) {
            throw navitia::exception("internal problem");
        }
        label.tickets.back().sections.push_back(section);
    }
    label.current_type = ticket.type;
    return label;
}

static bool valid(const State& state, const SectionKey& section){
    if((state.mode != "" && !boost::iequals(state.mode, section.mode)) ||
            (state.network != "" && !boost::iequals(state.network, section.network)) ||
            (state.line != "" && !boost::iequals(state.line, section.line)) )
        return false;
    return true;
}

static bool valid(const State& state, const Label& label){
    if((state.mode != "" && !boost::iequals(state.mode, label.mode)) ||
            (state.network != "" && !boost::iequals(state.network, label.network)) ||
            (state.line != "" && !boost::iequals(state.line, label.line))  ||
            (state.ticket != "" && !boost::iequals(state.ticket, label.tickets.back().caption)) )
        return false;
    return true;
}

std::string comp_to_string(const Comp_e comp) {
    switch (comp) {
    case Comp_e::EQ:
        return "=";
    case Comp_e::NEQ:
        return "!=";
    case Comp_e::LT:
        return "<";
    case Comp_e::GT:
        return ">";
    case Comp_e::LTE:
        return "<=";
    case Comp_e::GTE:
        return ">=";
    case Comp_e::True:
        return "True";
    default:
        throw navitia::exception("unhandled Comp case");
    }
}

results Fare::compute_fare(const routing::Path& path) const {
    results res;
    int nb_nodes = boost::num_vertices(g);

    if (nb_nodes < 2) {
        LOG4CPLUS_TRACE(logger, "no fare data loaded, cannot compute fare");
        return res;
    }
    std::vector< std::vector<Label> > labels(nb_nodes);
    // Start label
    labels[0].push_back(Label());
    size_t section_idx(0);

    for (auto item : path.items) {
        if (item.type != routing::ItemType::public_transport) {
            section_idx++;
            continue;
        }

        SectionKey section_key(item, section_idx++);

        std::vector<std::vector<Label>> new_labels(nb_nodes);
        try {
            BOOST_FOREACH(edge_t e, boost::edges(g)) {
                vertex_t u = boost::source(e,g);
                vertex_t v = boost::target(e,g);
                if (! valid(g[v], section_key))
                    continue;

                for (const Label& label: labels[u]) {
                    Ticket ticket;
                    const Transition& transition = g[e];
                    if (valid(g[u], label) && transition.valid(section_key, label)) {
                        if (transition.ticket_key != "") {
                            bool ticket_found = false; //TODO refactor this, optional is way better
                            auto it = fare_map.find(transition.ticket_key);
                            try {
                                if (it != fare_map.end()) {
                                    ticket = it->second.get_fare(section_key.date);
                                    ticket_found = true;
                                }
                            }
                            catch(no_ticket) { //the ticket_found bool is still false
                            }
                            if (! ticket_found) {
                                ticket = make_default_ticket();
                            }
                        }
                        if (transition.global_condition == Transition::GlobalCondition::exclusive)
                            throw ticket;
                        else if(transition.global_condition == Transition::GlobalCondition::with_changes) {
                            ticket.type = Ticket::ODFare;
                        }
                        Label next = next_label(label, ticket, section_key);

                        // we process the OD ticket: case where we'll not use this ticket anymore
                        if (label.current_type == Ticket::ODFare || ticket.type == Ticket::ODFare) {
                            try {
                                Ticket ticket_od;
                                ticket_od = get_od(next, section_key).get_fare(section_key.date);
                                if(label.tickets.size() > 0 && label.current_type == Ticket::ODFare)
                                    ticket_od.sections = label.tickets.back().sections;

                                ticket_od.sections.push_back(section_key);
                                Label n = next;
                                n.cost += ticket_od.value;
                                n.tickets.back() = ticket_od;
                                n.current_type = Ticket::FlatFare;

                                new_labels[0].push_back(n);
                            } catch (no_ticket) {
                                LOG4CPLUS_WARN(logger, "Unable to get the OD ticket SA=" << next.stop_area
                                               << " zone=" << next.zone
                                               << ", section start_zone=" << section_key.start_zone
                                               << ", dest_zone=" << section_key.dest_zone
                                               << " start_sa=" << section_key.start_stop_area
                                               << " dest_sa=" << section_key.dest_stop_area
                                               << " mode=" << section_key.mode);
                            }

                        } else {
                            new_labels[0].push_back(next);
                        }
                        new_labels[v].push_back(next);
                    }
                }
            }
        }
        // exclusive segment, we have to use that ticket
        catch(const Ticket& ticket) {
            LOG4CPLUS_TRACE(logger, "\texclusive section for fare");
            new_labels.clear();
            new_labels.resize(nb_nodes);
            for (Label label : labels.at(0)) {
                new_labels.at(0).push_back(next_label(label, ticket, section_key));
            }
        }
        labels = std::move(new_labels);

    }

    // We look for the cheapest label
    // if 2 label have the same cost, we take the one with the least number of tickets
    boost::optional<Label> best_label;
    for(const Label& label : labels.at(0)) {
        if(!best_label || label < (*best_label)) {
            res.tickets = label.tickets;
            res.not_found = (label.nb_undefined_sub_cost != 0);
            res.total = label.cost;
            best_label = label;
        }
    }

    return res;
}


void DateTicket::add(boost::gregorian::date begin, boost::gregorian::date end, const Ticket& ticket){
    tickets.push_back(PeriodTicket(greg::date_period(begin, end), ticket));
}

SectionKey::SectionKey(const routing::PathItem& path_item, const size_t idx) : path_item_idx(idx) {
    const navitia::type::StopPoint* first_sp = path_item.stop_points.front();
    const navitia::type::StopPoint* last_sp = path_item.stop_points.back();
    const navitia::type::VehicleJourney* vj = path_item.get_vj();

    //TODO, original uri for all
    network = vj->route->line->network->uri; //uri ?
    start_stop_area = first_sp->stop_area->uri;
    dest_stop_area = last_sp->stop_area->uri;
    line = vj->route->line->uri;
    date = path_item.departure.date();
    start_time = path_item.departure.time_of_day().total_seconds();
    dest_time = path_item.arrival.time_of_day().total_seconds();
    start_zone = first_sp->fare_zone;
    dest_zone = last_sp->fare_zone;
    mode = vj->physical_mode->uri; //CHECK
}

template<class T> bool compare(const T& a, const T& b, Comp_e comp) {
    switch(comp) {
    case Comp_e::LT: return a < b; break;
    case Comp_e::GT: return a > b; break;
    case Comp_e::GTE: return a >= b; break;
    case Comp_e::LTE: return a <= b; break;
    case Comp_e::EQ: return a == b; break;
    case Comp_e::NEQ: return a != b; break;
    default: return true;
    }
}

int SectionKey::duration_at_begin(int ticket_start_time) const {
    if (ticket_start_time < boost::lexical_cast<int>(start_time))
        return start_time - ticket_start_time;
    else // Passe-minuit
        return (start_time + 24*3600) - ticket_start_time;
}

int SectionKey::duration_at_end(int ticket_start_time) const {
    if (ticket_start_time < boost::lexical_cast<int>(dest_time))
        return dest_time - ticket_start_time;
    else // Passe-minuit
        return (dest_time + 24*3600) - ticket_start_time;
}

Ticket DateTicket::get_fare(boost::gregorian::date date) const {
    for (const auto& dticket : tickets) {
        if (dticket.validity_period.contains(date))
            return dticket.ticket;
    }

    throw no_ticket();
}

DateTicket DateTicket::operator +(const DateTicket& other) const{
    DateTicket new_ticket = *this;
    if(this->tickets.size() != other.tickets.size())
        LOG4CPLUS_ERROR(log4cplus::Logger::getInstance("log"), "Tickets n'ayant pas le même nombre de dates");

    for(size_t i = 0; i < std::min(this->tickets.size(), other.tickets.size()); ++i) {
        if(this->tickets[i].validity_period != other.tickets[i].validity_period)
            LOG4CPLUS_ERROR(log4cplus::Logger::getInstance("log"),
                            "Le ticket n° " << i << " n'a pas les même dates; " << this->tickets[i].validity_period << " versus " << other.tickets[i].validity_period);
        new_ticket.tickets[i].ticket.value = this->tickets[i].ticket.value + other.tickets[i].ticket.value;
    }
    return new_ticket;
}

bool Transition::valid(const SectionKey& section, const Label& label) const
{
    bool result = true;
    if(label.current_type == Ticket::ODFare && this->global_condition != Transition::GlobalCondition::with_changes)
        result = false;
    for(Condition cond: this->start_conditions)
    {
        if(cond.key == "zone" && boost::lexical_cast<int>(cond.value) != section.start_zone)
            result = false;
        else if(cond.key == "stoparea" && cond.value != section.start_stop_area)
            result = false;
        else if(cond.key == "duration") {
            // Dans le fichier CSV, on rentre le temps en minutes, en interne on travaille en secondes
            int duration = boost::lexical_cast<int>(cond.value) * 60;
            int ticket_duration = section.duration_at_begin(label.start_time);
            result &= compare(ticket_duration, duration, cond.comparaison);
        }
        else if(cond.key == "nb_changes") {
            int nb_changes = boost::lexical_cast<int>(cond.value);
            result &= compare(label.nb_changes, nb_changes, cond.comparaison);
        }
        else if(cond.key == "ticket" && label.tickets.size() > 0) {
            LOG4CPLUS_INFO(log4cplus::Logger::getInstance("log"), label.tickets.back().key << " " << cond.value);
            result &= compare(label.tickets.back().key, cond.value, cond.comparaison);
        }
    }
    for(Condition cond: this->end_conditions)
    {
        if(cond.key == "zone" && boost::lexical_cast<int>(cond.value) != section.dest_zone)
            result = false;
        else if(cond.key == "stoparea" && cond.value != section.dest_stop_area)
            result = false;
        else if(cond.key == "duration") {
            // Dans le fichier CSV, on rentre le temps en minutes, en interne on travaille en secondes
            int duration = boost::lexical_cast<int>(cond.value) * 60;
            int ticket_duration = section.duration_at_end(label.start_time);
            result &= compare(ticket_duration, duration, cond.comparaison);
        }
    }
    return result;
}

DateTicket Fare::get_od(const Label& label, const SectionKey& section) const {
    OD_key o_sa(OD_key::StopArea, label.stop_area);
    OD_key o_mode(OD_key::Mode, label.mode);
    OD_key o_zone(OD_key::Zone, boost::lexical_cast<std::string>(label.zone));

    OD_key d_sa(OD_key::StopArea, section.dest_stop_area);
    OD_key d_mode(OD_key::Mode, section.mode);
    OD_key d_zone(OD_key::Zone, boost::lexical_cast<std::string>(section.dest_zone));

    using OD_map = std::map<OD_key, std::vector<std::string>>;

    auto get_od_dest = [&] (const OD_map& od_map, const OD_key& sa, const OD_key& mode, const OD_key& zone) {
        auto od_t = od_map.find(sa);
        if (od_t == od_map.end())
            od_t = od_map.find(mode);
        if (od_t == od_map.end())
            od_t = od_map.find(zone);
        if (od_t == od_map.end())
            return boost::optional<OD_map::const_iterator>();
        return boost::optional<OD_map::const_iterator>(od_t);
    };

    boost::optional<OD_map::const_iterator> od;
    auto start_od_map = od_tickets.find(o_sa);
    // if we have some OD-tickets on this origin stop_area,
    // we look for a precise OD-ticket match destination also
    if (start_od_map != od_tickets.end()) {
        od = get_od_dest(start_od_map->second, d_sa, d_mode, d_zone);
    }
    // if we haven't found complete OD-ticket match we search on origin's mode
    if (! od) {
        start_od_map = od_tickets.find(o_mode);
        if (start_od_map != od_tickets.end()) {
            od = get_od_dest(start_od_map->second, d_sa, d_mode, d_zone);
        }
    }
    // if we haven't found complete OD-ticket match we search on origin's zone
    if (! od) {
        start_od_map = od_tickets.find(o_zone);
        if (start_od_map != od_tickets.end()) {
            od = get_od_dest(start_od_map->second, d_sa, d_mode, d_zone);
        }
    }
    if (! od) {
        throw no_ticket();
    }

    // We create a new ticket, sum of all atomic elements
    // A STIF OD-ticket is always the sum of multiple tickets
    DateTicket ticket;
    std::vector<std::string> vec_t = (*od)->second;
    auto it = fare_map.find(vec_t.at(0));
    if (it != fare_map.end()) {
        ticket = it->second;
    }
    for (size_t i = 1; i < vec_t.size(); ++i) {
        it = fare_map.find(vec_t.at(i));
        if (it != fare_map.end()) {
            ticket = ticket + it->second;
        } else {
            auto new_ticket = DateTicket();
            ticket = ticket + new_ticket;
        }
    }
    return ticket;
}


size_t Fare::nb_transitions() const {
    return boost::num_edges(g);
}

void Fare::add_default_ticket() {
    State begin; // Start is an empty node
    begin_v = boost::add_vertex(begin, g);

    //add a default ticket (more expensive than all the other, so it is taken only when no other choices are available)
    Transition default_transition;
    Ticket default_ticket = make_default_ticket();
    default_transition.ticket_key = default_ticket.key;
    boost::add_edge(begin_v, begin_v, default_transition, g);
    DateTicket dticket;
    dticket.add(boost::gregorian::date(boost::gregorian::neg_infin), boost::gregorian::date(boost::gregorian::pos_infin), default_ticket);
    fare_map.insert({default_ticket.key, dticket});
}

}
}
