#pragma once

#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include "type/data.h"
#include "kraken/data_manager.h"

#include <memory>


namespace navitia {

class MaintenanceWorker{
    private:
        DataManager<type::Data>& data_manager;
        log4cplus::Logger logger;

        AmqpClient::Channel::ptr_t channel;
        //nom de la queue créer pour ce worker
        std::string queue_name;

        void init_rabbitmq();
        void listen_rabbitmq();

    public:
        MaintenanceWorker(DataManager<type::Data>& data_manager);

        bool load_and_switch();

        void load();

        void operator()();
};

}
