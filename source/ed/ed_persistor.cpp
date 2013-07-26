#include "ed_persistor.h"

namespace bg = boost::gregorian;

namespace ed{

void EdPersistor::persist(const ed::Data& data, const navitia::type::MetaData& meta){

    this->lotus.start_transaction();

    this->clean_db();

    this->insert_metadata(meta);

    this->insert_networks(data.networks);
    this->insert_commercial_modes(data.commercial_modes);
    this->insert_physical_modes(data.physical_modes);
    this->insert_companies(data.companies);
    this->insert_contributors(data.contributors);

    this->insert_stop_areas(data.stop_areas);
    this->insert_stop_points(data.stop_points);

    this->insert_lines(data.lines);
    this->insert_routes(data.routes);

    this->insert_journey_patterns(data.journey_patterns);

    this->insert_validity_patterns(data.validity_patterns);
    this->insert_vehicle_journeys(data.vehicle_journeys);

    this->insert_journey_pattern_point(data.journey_pattern_points);

    this->insert_stop_times(data.stops);

    //@TODO: les connections ont des doublons, en attendant que ce soit corrigé, on ne les enregistre pas
    this->insert_stop_point_connections(data.stop_point_connections);
    //this->insert_journey_pattern_point_connections(data.journey_pattern_point_connections);
    this->build_relation();
    this->lotus.commit();

}

void EdPersistor::insert_metadata(const navitia::type::MetaData& meta){
    std::string request = "insert into navitia.parameters (beginning_date, end_date) VALUES ('" + bg::to_iso_extended_string(meta.production_date.begin()) +
        "', '" + bg::to_iso_extended_string(meta.production_date.end()) + "');";
    PQclear(this->lotus.exec(request));
}

void EdPersistor::build_relation(){
    PQclear(this->lotus.exec("SELECT georef.match_stop_area_to_admin()", "", PGRES_TUPLES_OK));
    PQclear(this->lotus.exec("SELECT georef.match_stop_point_to_admin();", "", PGRES_TUPLES_OK));
}

void EdPersistor::clean_db(){
    PQclear(this->lotus.exec("TRUNCATE navitia.stop_area, navitia.line, navitia.company, navitia.physical_mode, "
                "navitia.commercial_mode, navitia.properties, navitia.validity_pattern, navitia.network, navitia.parameters, navitia.connection CASCADE"));
}

void EdPersistor::insert_networks(const std::vector<types::Network*>& networks){
    this->lotus.prepare_bulk_insert("navitia.network", {"id", "uri", "name", "comment"});
    for(types::Network* net : networks){
        std::vector<std::string> values;
        values.push_back(std::to_string(net->idx));
        values.push_back(net->uri);
        values.push_back(net->name);
        values.push_back(net->comment);
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_commercial_modes(const std::vector<types::CommercialMode*>& commercial_modes){
    this->lotus.prepare_bulk_insert("navitia.commercial_mode", {"id", "uri", "name"});
    for(types::CommercialMode* mode : commercial_modes){
        std::vector<std::string> values;
        values.push_back(std::to_string(mode->idx));
        values.push_back(mode->uri);
        values.push_back(mode->name);
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_physical_modes(const std::vector<types::PhysicalMode*>& physical_modes){
    this->lotus.prepare_bulk_insert("navitia.physical_mode", {"id", "uri", "name"});
    for(types::PhysicalMode* mode : physical_modes){
        std::vector<std::string> values;
        values.push_back(std::to_string(mode->idx));
        values.push_back(mode->uri);
        values.push_back(mode->name);
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_companies(const std::vector<types::Company*>& companies){
    this->lotus.prepare_bulk_insert("navitia.company", {"id", "uri", "name", "comment", "address_name", "address_number",
                                    "address_type_name", "phone_number","mail", "website", "fax"});
    for(types::Company* company : companies){
        std::vector<std::string> values;
        values.push_back(std::to_string(company->idx));
        values.push_back(company->uri);
        values.push_back(company->name);
        values.push_back(company->comment);
        values.push_back(company->address_name);
        values.push_back(company->address_number);
        values.push_back(company->address_type_name);
        values.push_back(company->phone_number);
        values.push_back(company->mail);
        values.push_back(company->website);
        values.push_back(company->fax);
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_contributors(const std::vector<types::Contributor*>& contributors){
    this->lotus.prepare_bulk_insert("navitia.contributor", {"id", "uri", "name"});
    for(types::Contributor* contributor : contributors){
        std::vector<std::string> values;
        values.push_back(std::to_string(contributor->idx));
        values.push_back(contributor->uri);
        values.push_back(contributor->name);
        this->lotus.insert(values);
    }
    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_stop_areas(const std::vector<types::StopArea*>& stop_areas){
    this->lotus.prepare_bulk_insert("navitia.stop_area", {"id", "uri", "name", "coord", "comment"});

    //@TODO properties!!!
    for(types::StopArea* sa : stop_areas){
        std::vector<std::string> values;
        values.push_back(std::to_string(sa->idx));
        values.push_back(sa->uri);
        values.push_back(sa->name);
        values.push_back("POINT(" + std::to_string(sa->coord.lon()) + " " + std::to_string(sa->coord.lat()) + ")");
        values.push_back(sa->comment);
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_stop_points(const std::vector<types::StopPoint*>& stop_points){
    this->lotus.prepare_bulk_insert("navitia.stop_point", {"id", "uri", "name", "coord", "comment", "fare_zone", "stop_area_id"});

    //@TODO properties!!!
    for(types::StopPoint* sp : stop_points){
        std::vector<std::string> values;
        values.push_back(std::to_string(sp->idx));
        values.push_back(sp->uri);
        values.push_back(sp->name);
        values.push_back("POINT(" + std::to_string(sp->coord.lon()) + " " + std::to_string(sp->coord.lat()) + ")");
        values.push_back(sp->comment);
        values.push_back(std::to_string(sp->fare_zone));
        if(sp->stop_area != NULL){
            values.push_back(std::to_string(sp->stop_area->idx));
        }else{
            values.push_back(lotus.null_value);
        }
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();

}
void EdPersistor::insert_lines(const std::vector<types::Line*>& lines){
    this->lotus.prepare_bulk_insert("navitia.line", {"id", "uri", "name", "comment", "color", "code", "commercial_mode_id", "network_id"});

    for(types::Line* line : lines){
        std::vector<std::string> values;
        values.push_back(std::to_string(line->idx));
        values.push_back(line->uri);
        values.push_back(line->name);
        values.push_back(line->comment);
        values.push_back(line->color);
        values.push_back(line->code);
        if(line->commercial_mode != NULL){
            values.push_back(std::to_string(line->commercial_mode->idx));
        }else{
            values.push_back(lotus.null_value);
        }

        if(line->network != NULL){
            values.push_back(std::to_string(line->network->idx));
        }else{
            values.push_back(lotus.null_value);
        }
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();

}

void EdPersistor::insert_stop_point_connections(const std::vector<types::StopPointConnection*>& connections){
    this->lotus.prepare_bulk_insert("navitia.connection", {"departure_stop_point_id", "destination_stop_point_id", "connection_type_id",
            "duration", "max_duration"});

    //@TODO properties!!
    for(types::StopPointConnection* co : connections){
        std::vector<std::string> values;
        values.push_back(std::to_string(co->departure->idx));
        values.push_back(std::to_string(co->destination->idx));
        values.push_back(std::to_string(static_cast<int>(co->connection_kind)));
        values.push_back(std::to_string(co->duration));
        values.push_back(std::to_string(co->max_duration));
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_journey_pattern_point_connections(const std::vector<types::JourneyPatternPointConnection*>& connections){
    this->lotus.prepare_bulk_insert("navitia.journey_pattern_point_connection", {"departure_journey_pattern_point_id",
            "destination_journey_pattern_point_id", "connection_kind_id", "length"});

    //@TODO properties!!
    for(types::JourneyPatternPointConnection* co : connections){
        std::vector<std::string> values;
        values.push_back(std::to_string(co->departure->idx));
        values.push_back(std::to_string(co->destination->idx));
        values.push_back(std::to_string(static_cast<int>(co->connection_kind)));
        values.push_back(std::to_string(co->length));
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_routes(const std::vector<types::Route*>& routes){
    this->lotus.prepare_bulk_insert("navitia.route", {"id", "uri", "name", "comment", "line_id"});

    for(types::Route* route : routes){
        std::vector<std::string> values;
        values.push_back(std::to_string(route->idx));
        values.push_back(route->uri);
        values.push_back(route->name);
        values.push_back(route->comment);
        if(route->line != NULL){
            values.push_back(std::to_string(route->line->idx));
        }else{
            values.push_back(lotus.null_value);
        }
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();

}

void EdPersistor::insert_journey_patterns(const std::vector<types::JourneyPattern*>& journey_patterns){
    this->lotus.prepare_bulk_insert("navitia.journey_pattern", {"id", "uri", "name", "comment", "is_frequence", "route_id"});

    for(types::JourneyPattern* jp : journey_patterns){
        std::vector<std::string> values;
        values.push_back(std::to_string(jp->idx));
        values.push_back(jp->uri);
        values.push_back(jp->name);
        values.push_back(jp->comment);
        values.push_back(std::to_string(jp->is_frequence));
        if(jp->route != NULL){
            values.push_back(std::to_string(jp->route->idx));
        }else{
            values.push_back(lotus.null_value);
        }
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_validity_patterns(const std::vector<types::ValidityPattern*>& validity_patterns){
    this->lotus.prepare_bulk_insert("navitia.validity_pattern", {"id", "days"});

    for(types::ValidityPattern* vp : validity_patterns){
        std::vector<std::string> values;
        values.push_back(std::to_string(vp->idx));
        values.push_back(vp->days.to_string());
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_stop_times(const std::vector<types::StopTime*>& stop_times){
    this->lotus.prepare_bulk_insert("navitia.stop_time", {"arrival_time", "departure_time", "local_traffic_zone", "start_time",
                                    "end_time", "headway_sec", "odt", "pick_up_allowed", "drop_off_allowed",
                                    "is_frequency", "journey_pattern_point_id", "vehicle_journey_id", "comment", "date_time_estimated"});

    for(types::StopTime* stop : stop_times){
        std::vector<std::string> values;
        values.push_back(std::to_string(stop->arrival_time));
        values.push_back(std::to_string(stop->departure_time));
        if(stop->local_traffic_zone != navitia::type::invalid_idx){
            values.push_back(std::to_string(stop->local_traffic_zone));
        }else{
            values.push_back(lotus.null_value);
        }
        values.push_back(std::to_string(stop->start_time));
        values.push_back(std::to_string(stop->end_time));
        values.push_back(std::to_string(stop->headway_secs));
        values.push_back(std::to_string(stop->ODT));
        values.push_back(std::to_string(stop->pick_up_allowed));
        values.push_back(std::to_string(stop->drop_off_allowed));        
        values.push_back(std::to_string(stop->is_frequency));

        if(stop->journey_pattern_point != NULL){
            values.push_back(std::to_string(stop->journey_pattern_point->idx));
        }else{
            values.push_back(lotus.null_value);
        }
        if(stop->vehicle_journey != NULL){
            values.push_back(std::to_string(stop->vehicle_journey->idx));
        }else{
            values.push_back(lotus.null_value);
        }
        values.push_back(stop->comment);        
        values.push_back(std::to_string(stop->date_time_estimated));
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_journey_pattern_point(const std::vector<types::JourneyPatternPoint*>& journey_pattern_points){
    this->lotus.prepare_bulk_insert("navitia.journey_pattern_point", {"id", "uri", "name", "comment", "\"order\"", "stop_point_id", "journey_pattern_id"});

    for(types::JourneyPatternPoint* jpp : journey_pattern_points){
        std::vector<std::string> values;
        values.push_back(std::to_string(jpp->idx));
        values.push_back(jpp->uri);
        values.push_back(jpp->name);
        values.push_back(jpp->comment);
        values.push_back(std::to_string(jpp->order));

        if(jpp->stop_point != NULL){
            values.push_back(std::to_string(jpp->stop_point->idx));
        }else{
            values.push_back(lotus.null_value);
        }

        if(jpp->journey_pattern != NULL){
            values.push_back(std::to_string(jpp->journey_pattern->idx));
        }else{
            values.push_back(lotus.null_value);
        }
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();
}

void EdPersistor::insert_vehicle_journeys(const std::vector<types::VehicleJourney*>& vehicle_journeys){
    this->lotus.prepare_bulk_insert("navitia.vehicle_journey", {"id", "uri", "name", "comment", "validity_pattern_id",
                                    "adapted_validity_pattern_id", "company_id", "physical_mode_id", "journey_pattern_id", "theoric_vehicle_journey_id", "odt_type_id", "odt_message"});

    for(types::VehicleJourney* vj : vehicle_journeys){
        std::vector<std::string> values;
        values.push_back(std::to_string(vj->idx));
        values.push_back(vj->uri);
        values.push_back(vj->name);
        values.push_back(vj->comment);
        if(vj->validity_pattern != NULL){
            values.push_back(std::to_string(vj->validity_pattern->idx));
        }else{
            values.push_back(lotus.null_value);
        }
        if(vj->adapted_validity_pattern != NULL){
            values.push_back(std::to_string(vj->adapted_validity_pattern->idx));
        }else{
            values.push_back(std::to_string(vj->validity_pattern->idx));
        }
        if(vj->company != NULL){
            values.push_back(std::to_string(vj->company->idx));
        }else{
            values.push_back(lotus.null_value);
        }
        if(vj->physical_mode != NULL){
            values.push_back(std::to_string(vj->physical_mode->idx));
        }else{
            values.push_back(lotus.null_value);
        }
        if(vj->journey_pattern != NULL){
            values.push_back(std::to_string(vj->journey_pattern->idx));
        }else{
            values.push_back(lotus.null_value);
        }
        if(vj->theoric_vehicle_journey != NULL){
            values.push_back(std::to_string(vj->theoric_vehicle_journey->idx));
        }else{
            //@TODO WTF??
            values.push_back(lotus.null_value);
        }
        values.push_back(std::to_string(static_cast<int>(vj->odt_type)));        
        values.push_back(vj->odt_message);
        this->lotus.insert(values);
    }

    this->lotus.finish_bulk_insert();
}

}//namespace
