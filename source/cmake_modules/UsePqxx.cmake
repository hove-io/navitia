# Pqxx
find_path(PQXX_INCLUDE_DIR pqxx/pqxx)
find_library(PQXX_LIB pqxx)

include_directories("${PQXX_INCLUDE_DIR}")
