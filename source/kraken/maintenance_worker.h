#pragma once

#include "type/data.h"

#include <memory>


namespace navitia {

class MaintenanceWorker{
    private:
        navitia::type::Data** data;
        log4cplus::Logger logger;

        boost::posix_time::ptime next_rt_load;

    public:
        MaintenanceWorker(type::Data** data);

        bool load_and_switch();

        void load();

        void operator()();

        void sighandler(int signal);
};

}
