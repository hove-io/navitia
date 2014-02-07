#pragma once
#include "fare/fare.h"
#include "ed/data.h"

namespace ed { namespace connectors {

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
