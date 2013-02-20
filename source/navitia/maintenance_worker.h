#pragma once

#include "type/data.h"

#include <memory>


namespace navitia {

class MaintenanceWorker{
    private:
        navitia::type::Data & data;
        log4cplus::Logger logger;

    public:
        MaintenanceWorker(navitia::type::Data & data);

        bool load_and_switch();

        void load();

        void operator()();

        void sighandler(int signal);
};

}
