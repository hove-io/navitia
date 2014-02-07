#pragma once
#include "fare/fare.h"
#include "ed/data.h"

namespace ed { namespace connectors {


/// Exception levée si on utilise une clef inconnue
//struct invalid_key : std::exception{};

/// Parse un état
//navitia::fare::State parse_state(const std::string & state);

///// Parse une condition de passage
//navitia::fare::Condition parse_condition(const std::string & condition);

/// Exception levée si on n'arrive pas à parser une condition
//struct invalid_condition : std::exception {};

/// Parse une liste de conditions séparés par des &
//std::vector<navitia::fare::Condition> parse_conditions(const std::string & conditions);

struct fare_parser {
    Data& data;

    const std::string state_transition_filename;
    const std::string prices_filename;
    const std::string od_filename;

    fare_parser(Data& data_, const std::string& state, const std::string& prices, const std::string od) :
        data(data_),
        state_transition_filename(state),
        prices_filename(prices),
        od_filename(od) {}

    void load();
private:
    /// Charge les deux fichiers obligatoires
    void load_transitions();
    void load_prices();

    /// Charge les tarifs
    void load_fares();

    /// Charge le fichier d'OD
    void load_od();

    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
};

}
}
