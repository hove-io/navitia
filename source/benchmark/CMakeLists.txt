FIND_PACKAGE(WS_Commons)
find_package(Protobuf REQUIRED)


MAKE_WS(WS_Benchmark ${CMAKE_SOURCE_DIR}/gateway_mod/interface.h ${CMAKE_SOURCE_DIR}/gateway_mod/interface.cpp ${CMAKE_SOURCE_DIR}/WS_commons/threadpool.h ${CMAKE_SOURCE_DIR}/WS_commons/data_structures.h ${CMAKE_SOURCE_DIR}/WS_commons/fcgi.h ${CMAKE_SOURCE_DIR}/WS_commons/isapi.h ${CMAKE_SOURCE_DIR}/WS_commons/baseworker.h benchmark_ws.cpp LIBS pthread ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} types network)
MAKE_WS(Benchmark_raptor_ws ${CMAKE_SOURCE_DIR}/gateway_mod/interface.h ${CMAKE_SOURCE_DIR}/gateway_mod/interface.cpp ${CMAKE_SOURCE_DIR}/WS_commons/threadpool.h ${CMAKE_SOURCE_DIR}/WS_commons/data_structures.h ${CMAKE_SOURCE_DIR}/WS_commons/fcgi.h ${CMAKE_SOURCE_DIR}/WS_commons/isapi.h ${CMAKE_SOURCE_DIR}/WS_commons/baseworker.h benchmark_raptor_ws.cpp LIBS pthread ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} types raptor)

add_executable (gen_csv generateur_csv.cpp)

target_link_libraries(gen_csv ${Boost_LIBRARIES} utils connectors types)
