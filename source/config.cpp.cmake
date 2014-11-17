#include "conf.h"

namespace navitia { namespace config {

const char* fixtures_dir = "${FIXTURES_DIR}";
const char* navitia_build_type = "${CMAKE_BUILD_TYPE}";
const char* kraken_version = "${GIT_REVISION}";

}}// namespace navitia::config
