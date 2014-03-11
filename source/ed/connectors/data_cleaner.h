#pragma once
#include "ed/data.h"


namespace ed { namespace connectors {

/**
 * Structure meant to clean and consolidate data before adding them to ED
 */
struct data_cleaner {
    ed::Georef& data;
    log4cplus::Logger logger;

    data_cleaner(ed::Georef& data_to_clean) :
        data(data_to_clean),
        logger(log4cplus::Logger::getInstance("log")) {}

    void clean();
    void fusion_ways();
    void fusion_ways_list(const std::vector<ed::types::Edge*>& edges);
    void fusion_ways_by_graph(const std::vector<types::Edge*>& edges);

    size_t ways_fusionned = 0;
};

}}
