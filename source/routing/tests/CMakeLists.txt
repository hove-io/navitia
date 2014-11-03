#We use the BOOST_LIBS define is the parent: routing
SET(BOOST_LIBS ${BOOST_LIBS} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
add_executable(raptor_test raptor_test.cpp)
target_link_libraries(raptor_test ${Boost_LIBRARIES} ed data fare routing georef
    autocomplete utils ${BOOST_LIBS} log4cplus pb_lib protobuf)
ADD_BOOST_TEST(raptor_test)

add_executable(reverse_raptor_test reverse_raptor_test.cpp)
target_link_libraries(reverse_raptor_test ed data fare routing georef autocomplete
    utils ${BOOST_LIBS} log4cplus pb_lib protobuf)
ADD_BOOST_TEST(reverse_raptor_test)

add_executable(routing_api_test routing_api_test.cpp)
target_link_libraries(routing_api_test ed data types routing fare pb_lib thermometer georef
    autocomplete utils  ${BOOST_LIBS} log4cplus pthread protobuf)
ADD_BOOST_TEST(routing_api_test)

add_executable(best_stoptime_test best_stoptime.cpp)
target_link_libraries(best_stoptime_test ed connectors data fare types routing
    autocomplete pb_lib thermometer autocomplete georef utils ${BOOST_LIBS} log4cplus protobuf)
ADD_BOOST_TEST(best_stoptime_test)

add_executable(raptor_adapted_test raptor_adapted_test.cpp)
target_link_libraries(raptor_adapted_test adapted ed data types routing fare pb_lib thermometer georef
    autocomplete utils ${BOOST_LIBS} log4cplus pthread protobuf)
ADD_BOOST_TEST(raptor_adapted_test)

add_executable(raptor_odt_test raptor_odt_test.cpp)
target_link_libraries(raptor_odt_test ed data types routing fare pb_lib thermometer georef
    autocomplete utils ${BOOST_LIBS} log4cplus pthread protobuf)
ADD_BOOST_TEST(raptor_odt_test)

add_executable(disruptions_test disruptions_test.cpp)
target_link_libraries(disruptions_test ed data types routing fare pb_lib thermometer georef
  autocomplete utils workers
  ${BOOST_LIBS} log4cplus pthread protobuf)
ADD_BOOST_TEST(disruptions_test)
