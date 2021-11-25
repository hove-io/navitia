SET(PROTO_FILES
    ${CMAKE_SOURCE_DIR}/navitia-proto/type.proto
    ${CMAKE_SOURCE_DIR}/navitia-proto/response.proto
    ${CMAKE_SOURCE_DIR}/navitia-proto/request.proto
    ${CMAKE_SOURCE_DIR}/navitia-proto/task.proto
    ${CMAKE_SOURCE_DIR}/chaos-proto/chaos.proto
    ${CMAKE_SOURCE_DIR}/chaos-proto/gtfs-realtime.proto
    ${CMAKE_SOURCE_DIR}/chaos-proto/kirin.proto
)

SET(PROTO_HDRS
    ${CMAKE_CURRENT_BINARY_DIR}/chaos.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/gtfs-realtime.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/kirin.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/request.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/response.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/task.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/type.pb.h
)

SET(PROTO_SRCS ${CMAKE_CURRENT_BINARY_DIR}/chaos.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/gtfs-realtime.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/kirin.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/request.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/response.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/task.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/type.pb.cc
)

SET(NAVITIACOMMON_DIR ${CMAKE_SOURCE_DIR}/navitiacommon/navitiacommon)
SET(NAVITIACOMMON_PROTO_PY
    ${NAVITIACOMMON_DIR}/request_pb2.py
    ${NAVITIACOMMON_DIR}/response_pb2.py
    ${NAVITIACOMMON_DIR}/stat_pb2.py
    ${NAVITIACOMMON_DIR}/task_pb2.py
    ${NAVITIACOMMON_DIR}/type_pb2.py
)

SET(MONITOR_KRAKEN_DIR ${CMAKE_SOURCE_DIR}/monitor/monitor_kraken)
SET(MONITOR_KRAKEN_PROTO_PY
    ${MONITOR_KRAKEN_DIR}/request_pb2.py
    ${MONITOR_KRAKEN_DIR}/response_pb2.py
    ${MONITOR_KRAKEN_DIR}/type_pb2.py
)

SET(JORMUNGANDR_TEST_DIR ${CMAKE_SOURCE_DIR}/jormungandr/tests)
SET(JORMUNGANDR_TEST_PY
    ${JORMUNGANDR_TEST_DIR}/chaos_pb2.py
    ${JORMUNGANDR_TEST_DIR}/gtfs_realtime_pb2.py
    ${JORMUNGANDR_TEST_DIR}/kirin_pb2.py
)

add_custom_command(OUTPUT ${PROTO_HDRS} ${PROTO_SRCS}
    COMMAND protoc ARGS ${PROTO_FILES} -I ${CMAKE_SOURCE_DIR}/navitia-proto -I ${CMAKE_SOURCE_DIR}/chaos-proto
    --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/type" VERBATIM
    DEPENDS ${PROTO_FILES})

add_custom_command (OUTPUT ${NAVITIACOMMON_PROTO_PY}
    COMMAND protoc ARGS "--python_out=${NAVITIACOMMON_DIR}"
    type.proto response.proto request.proto task.proto stat.proto
    COMMAND 2to3 --no-diffs -w ${NAVITIACOMMON_PROTO_PY}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/navitia-proto" VERBATIM
    DEPENDS ${PROTO_FILES})

#we don't want a dependancy to navitiacommon for monitor
add_custom_command (OUTPUT ${MONITOR_KRAKEN_PROTO_PY}
    COMMAND protoc ARGS "--python_out=${MONITOR_KRAKEN_DIR}"
    type.proto response.proto request.proto
    COMMAND 2to3 --no-diffs -w ${MONITOR_KRAKEN_PROTO_PY}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/navitia-proto" VERBATIM
    DEPENDS ${PROTO_FILES})

#for integration tests, we need chaos protobuf
add_custom_command (OUTPUT ${JORMUNGANDR_TEST_PY}
    COMMAND protoc ARGS "--python_out=${JORMUNGANDR_TEST_DIR}"
    chaos.proto gtfs-realtime.proto kirin.proto
    COMMAND 2to3 --no-diffs -w ${JORMUNGANDR_TEST_PY}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/chaos-proto" VERBATIM
    DEPENDS ${PROTO_FILES})

add_custom_target(protobuf_files ALL DEPENDS ${PROTO_HDRS} ${PROTO_SRCS}
                  ${NAVITIACOMMON_PROTO_PY} ${MONITOR_KRAKEN_PROTO_PY} ${JORMUNGANDR_TEST_PY})
set_source_files_properties(${PROTO_HDRS} ${PROTO_SRCS} PROPERTIES GENERATED TRUE)

#
# disable warnings for protobuf generated cpp files
#
if(CMAKE_COMPILER_IS_GNUCXX)
    set(PROTO_CMAKE_CXX_FLAGS "${CMAKE_GNUCXX_COMMON_FLAGS} -w")
endif(CMAKE_COMPILER_IS_GNUCXX)
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(PROTO_CMAKE_CXX_FLAGS "${CMAKE_CLANG_COMMON_FLAGS} -Wno-everything")
endif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
SET_SOURCE_FILES_PROPERTIES(${PROTO_SRCS} PROPERTIES COMPILE_FLAGS ${PROTO_CMAKE_CXX_FLAGS})

add_library(pb_lib ${PROTO_SRCS})
target_link_libraries(pb_lib ${PROTOBUF_LIBRARY})

add_library(pb_converter pb_converter.cpp)
target_link_libraries(pb_converter thermometer vptranslator pthread pb_lib)

add_library(types message.cpp datetime.cpp geographical_coord.cpp timezone_manager.cpp
    validity_pattern.cpp type_utils.cpp stop_point.cpp entrance_exit.cpp connection.cpp calendar.cpp stop_area.cpp network.cpp
    contributor.cpp dataset.cpp company.cpp commercial_mode.cpp physical_mode.cpp line.cpp route.cpp
    vehicle_journey.cpp meta_vehicle_journey.cpp stop_time.cpp type_interfaces.cpp comment_container.cpp
    odt_properties.cpp comment.cpp static_data.cpp entry_point.cpp)
target_link_libraries(types ptreferential utils pb_lib protobuf)
add_dependencies(types protobuf_files)

SET(DATA_SRC
    data.cpp
    data_exceptions.cpp
    "${CMAKE_SOURCE_DIR}/third_party/lz4/lz4.c"
    pt_data.cpp
    headsign_handler.cpp
)


add_library(data ${DATA_SRC})
target_link_libraries(data
    fill_disruption_from_database
    routing
    fare
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
)

# Add tests
if(NOT SKIP_TESTS)
    add_subdirectory(tests)
endif(NOT SKIP_TESTS)
