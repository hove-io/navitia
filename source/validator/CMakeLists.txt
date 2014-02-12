
add_executable(validator validator.cpp)
target_link_libraries(validator data routing fare types georef utils autocomplete
    ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_DATE_TIME_LIBRARY}
    ${Boost_THREAD_LIBRARY} ${Boost_REGEX_LIBRARY}
    ${Boost_SERIALIZATION_LIBRARY} ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY} log4cplus)
