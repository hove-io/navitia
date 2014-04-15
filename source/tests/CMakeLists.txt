#We use the BOOST_LIBS define is the parent
SET(BOOST_LIBS ${BOOST_LIBS} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

SET(MOCK_KRAKEN_UTILS_SRC_TEST
    mock_kraken.h
    mock_kraken.cpp
)
add_library(mock_kraken_utils ${MOCK_KRAKEN_UTILS_SRC_TEST})
# every tests will produce an executable used in jormungandr integration tests

add_executable(main_routing_test main_routing_test.cpp)
target_link_libraries(main_routing_test mock_kraken_utils utils ${Boost_LIBRARIES})
