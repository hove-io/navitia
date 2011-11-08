FIND_PACKAGE(Boost 1.40.0 COMPONENTS graph serialization REQUIRED)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS}
                      ${CMAKE_SOURCE_DIR}
                      ${CMAKE_SOURCE_DIR}/../third_party
)

add_library(streetnetwork  types.h types.cpp ../utils/csv.cpp ../utils/encoding_converter.cpp ${CMAKE_SOURCE_DIR}/../third_party/fastlz/fastlz.c)
target_link_libraries(streetnetwork ${Boost_LIBRARIES})

add_executable (test_sn test.cpp types.cpp ../type/type.cpp)
target_link_libraries(test_sn ${Boost_LIBRARIES} streetnetwork proj)


