#pragma once

#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include "type/data.h"

#include <memory>


namespace navitia {

class MaintenanceWorker{
    private:
        navitia::type::Data** data;
        log4cplus::Logger logger;

        boost::posix_time::ptime next_rt_load;

        AmqpClient::Channel::ptr_t channel;
        //nom de la queue créer pour ce worker
        std::string queue_name;

        void init_rabbitmq();

    public:
        MaintenanceWorker(type::Data** data);

        bool load_and_switch();

        void load();

        void operator()();
};

}
