#pragma once

#cmakedefine HAVE_ICONV_H 1
#cmakedefine HAVE_LOGGINGMACROS_H 1

namespace navitia { namespace config {

extern const char* fixtures_dir;
extern const char* navitia_build_type;
extern const char* kraken_version;

}}// namespace navitia::config
