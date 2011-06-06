FIND_PACKAGE(Boost 1.40.0 COMPONENTS serialization date_time regex thread iostreams REQUIRED)
find_package(Protobuf REQUIRED)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS}
                      ${CMAKE_SOURCE_DIR}/fastlz_filter
                      ${CMAKE_SOURCE_DIR}/third_party
                      ${PROTOBUF_INCLUDE_DIRS}
                      ${CMAKE_CURRENT_BINARY_DIR}
)
 
add_executable (test_gtfs test.cpp gtfs_parser.cpp gtfs_parser.h type.h type.cpp data.h data.cpp indexes.h ${CMAKE_SOURCE_DIR}/fastlz_filter/fastlz.c)
target_link_libraries(test_gtfs ${Boost_LIBRARIES})

add_executable (examples examples.cpp indexes.h)
target_link_libraries(examples ${Boost_LIBRARIES})

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS type.proto)
add_executable (ptref ptreferential.h ptreferential.cpp type.h type.cpp data.h data.cpp indexes.h indexes2.h where.h reflexion.h ${CMAKE_SOURCE_DIR}/fastlz_filter/fastlz.c ${PROTO_SRCS} ${PROTO_HDRS} ptref_cli.cpp)
target_link_libraries(ptref ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY})

add_subdirectory(tests)
