#pragma once
#include <log4cplus/logger.h>

/** Crée une configuration par défaut pour le logger */
void init_logger();

/** Configure le logger à partir du fichier de configuration */
void init_logger(const std::string & config_file);
