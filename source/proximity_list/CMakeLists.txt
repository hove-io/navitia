add_library(proximitylist proximity_list.cpp proximitylist_api.cpp)
add_dependencies(proximitylist protobuf_files)
target_link_libraries(proximitylist ${LZ4_LIBRARY})
add_subdirectory(tests)
