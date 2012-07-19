#include "logger.h"
#include <log4cplus/configurator.h>

void init_logger(){
    log4cplus::BasicConfigurator config;
    config.configure();
}
void init_logger(const std::string & config_file){
    log4cplus::PropertyConfigurator::doConfigure(config_file);
}
