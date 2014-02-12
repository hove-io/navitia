#pragma once
#include "fare/fare.h"
#include "ed/data.h"

namespace ed { namespace connectors {

/**
 * Parser for fare files
 *
 * There are 3 fare files:
 *  - price.csv: description of all the different prices
 *  - fares.csv: description of the transition for the state machine
 *  - od_fares.csv: description of the fare by od (not mandatory)
 */
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
    void load_transitions();

    void load_prices();

    void load_fares();

    void load_od();

    log4cplus::Logger logger = log4cplus::Logger::getInstance("log");
};

}
}
