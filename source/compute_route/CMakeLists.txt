FIND_PACKAGE(Boost 1.40.0 COMPONENTS serialization date_time regex thread iostreams REQUIRED)
find_package(Protobuf REQUIRED)
link_directories (${Boost_LIBRARY_DIRS})
include_directories ( ${Boost_INCLUDE_DIRS}
                      ${CMAKE_SOURCE_DIR}/type
                      ${CMAKE_SOURCE_DIR}
                      ${CMAKE_SOURCE_DIR}/../third_party
                      ${CMAKE_CURRENT_BINARY_DIR}/../type
)


SET(CPTR_SRC network.h network.cpp main.h main.cpp )


add_executable(compute_route network.cpp main.cpp)
target_link_libraries(compute_route ${Boost_LIBRARIES} ${PROTOBUF_LIBRARY} pthread readline  types)

