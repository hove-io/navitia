SET(PROTO_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/type.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/response.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/request.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/task.proto
    ${CMAKE_CURRENT_SOURCE_DIR}/realtime.proto
    ${CMAKE_SOURCE_DIR}/chaos-proto/chaos.proto
    ${CMAKE_SOURCE_DIR}/chaos-proto/gtfs-realtime.proto
)

SET(PROTO_HDRS
    ${CMAKE_CURRENT_BINARY_DIR}/chaos.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/gtfs-realtime.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/realtime.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/request.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/response.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/task.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/type.pb.h
)

SET(PROTO_SRCS ${CMAKE_CURRENT_BINARY_DIR}/chaos.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/gtfs-realtime.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/realtime.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/request.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/response.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/task.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/type.pb.cc
)

add_custom_command(OUTPUT ${PROTO_HDRS} ${PROTO_SRCS}
    COMMAND protoc ARGS ${PROTO_FILES} -I ${CMAKE_CURRENT_SOURCE_DIR} -I ${CMAKE_SOURCE_DIR}/chaos-proto
    --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/type" VERBATIM
    DEPENDS ${PROTO_FILES})

add_custom_target(protobuf_files DEPENDS ${PROTO_HDRS} ${PROTO_SRCS})
set_source_files_properties(${PROTO_HDRS} ${PROTO_SRCS} PROPERTIES GENERATED TRUE)


add_library(pb_lib ${PROTO_SRCS} pb_converter.cpp)
target_link_libraries(pb_lib pthread ${PROTOBUF_LIBRARY})
add_custom_command (TARGET pb_lib
    PRE_BUILD
    COMMAND protoc ARGS "--python_out=${CMAKE_SOURCE_DIR}/navitiacommon/navitiacommon" type.proto
    response.proto request.proto task.proto realtime.proto stat.proto
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/type" VERBATIM)

add_custom_command (TARGET pb_lib
    PRE_BUILD
    COMMAND protoc ARGS "--python_out=${CMAKE_SOURCE_DIR}/stat_persistor/stat_persistor" type.proto
    task.proto realtime.proto stat.proto
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/type" VERBATIM)

add_custom_command (TARGET pb_lib
    PRE_BUILD
    COMMAND protoc ARGS "--python_out=${CMAKE_SOURCE_DIR}/sindri/sindri" type.proto
    task.proto realtime.proto
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/type" VERBATIM)

add_custom_command (TARGET pb_lib
    PRE_BUILD
    COMMAND protoc ARGS "--python_out=${CMAKE_SOURCE_DIR}/connectors/connectors" type.proto
    task.proto realtime.proto
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/type" VERBATIM)

#we don't want a dependancy to navitiacommon for monitor
add_custom_command (TARGET pb_lib
    PRE_BUILD
    COMMAND protoc ARGS "--python_out=${CMAKE_SOURCE_DIR}/monitor/monitor_kraken" type.proto
    response.proto request.proto
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/type" VERBATIM)

add_library(types type.cpp message.cpp datetime.cpp geographical_coord.cpp)
target_link_libraries(types utils pb_lib)
add_dependencies(types protobuf_files)

SET(DATA_SRC
    data.cpp
    "${CMAKE_SOURCE_DIR}/third_party/lz4/lz4.c"
    pt_data.cpp
)

add_library(data ${DATA_SRC})
target_link_libraries(data types fill_disruption_from_database)

SET(BOOST_LIBS ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    ${Boost_SYSTEM_LIBRARY} ${Boost_SERIALIZATION_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY} ${Boost_REGEX_LIBRARY} ${Boost_THREAD_LIBRARY})

add_executable(main_destination_test tests/main_destination_test.cpp)
target_link_libraries(main_destination_test ed data types routing fare georef autocomplete utils ${BOOST_LIBS} log4cplus pb_lib protobuf)
ADD_BOOST_TEST(main_destination_test)

add_executable(fill_pb_placemark_test tests/fill_pb_placemark_test.cpp)
target_link_libraries(fill_pb_placemark_test ed data types routing fare georef autocomplete utils ${BOOST_LIBS} log4cplus pb_lib thermometer protobuf)
ADD_BOOST_TEST(fill_pb_placemark_test)

add_executable(associated_calendar_test tests/associated_calendar_test.cpp)
target_link_libraries(associated_calendar_test ed data types routing fare georef autocomplete utils ${BOOST_LIBS} log4cplus pb_lib protobuf)
ADD_BOOST_TEST(associated_calendar_test)

add_executable(type_test tests/test.cpp)
target_link_libraries(type_test types data utils ${BOOST_LIBS} log4cplus)
add_dependencies(type_test protobuf_files)
ADD_BOOST_TEST(type_test)

add_executable(datetime_test tests/datetime.cpp)
target_link_libraries(datetime_test types ${BOOST_LIBS} log4cplus)
ADD_BOOST_TEST(datetime_test)


add_executable(accessible_test tests/accessible.cpp)
target_link_libraries(accessible_test ${BOOST_LIBS} log4cplus)
add_dependencies(accessible_test protobuf_files)
ADD_BOOST_TEST(accessible_test)
