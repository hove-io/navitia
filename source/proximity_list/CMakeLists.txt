add_library(proximitylist proximity_list.cpp proximitylist_api.cpp ${CMAKE_SOURCE_DIR}/third_party/lz4/lz4hc.c)
add_dependencies(proximitylist protobuf_files)
target_link_libraries(proximitylist)
add_subdirectory(tests)
