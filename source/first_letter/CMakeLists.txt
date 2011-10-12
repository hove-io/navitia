FIND_PACKAGE(Boost 1.40.0 COMPONENTS unit_test_framework serialization date_time regex thread iostreams REQUIRED)
FIND_PACKAGE(WS_Commons)
find_package(Protobuf REQUIRED)
link_directories ( ${Boost_LIBRARY_DIRS} )
include_directories ( ${Boost_INCLUDE_DIRS} 
    ${CMAKE_SOURCE_DIR} 
    ${CMAKE_CURRENT_BINARY_DIR}/../type
)

#MAKE_WS(trie trie.cpp)

add_executable (first_letter_test test.cpp first_letter.h)
target_link_libraries(first_letter_test ${Boost_LIBRARIES})
ADD_TEST(test_first_letter ${EXECUTABLE_OUTPUT_PATH}/first_letter_test --report_level=no)


MAKE_WS(first_letter ${CMAKE_SOURCE_DIR}/WS_commons/threadpool.h ${CMAKE_SOURCE_DIR}/WS_commons/data_structures.h ${CMAKE_SOURCE_DIR}/WS_commons/fcgi.h ${CMAKE_SOURCE_DIR}/WS_commons/isapi.h ${CMAKE_SOURCE_DIR}/WS_commons/baseworker.h firstletter_ws.cpp LIBS pthread ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} types)
