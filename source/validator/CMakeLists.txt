FIND_PACKAGE(Boost 1.40.0 COMPONENTS program_options filesystem system serialization date_time thread REQUIRED)

add_executable(validator validator.cpp)
target_link_libraries(validator data streetnetwork routing utils ${Boost_LIBRARIES})
