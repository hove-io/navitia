#pragma once

#include "type/data.h"

#include <memory>

namespace navitia {

class MaintenanceWorker {
    private:
        navitia::type::Data & data;
        log4cplus::Logger logger;

    public:
        MaintenanceWorker(navitia::type::Data & data) : data(data), logger(log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("background"))){}

        bool load_and_switch();

        void load();

        void operator()();
};

}
