SET(PTREF_SRC
  ptreferential.cpp
  ptreferential_utils.cpp
  ptreferential_ng.cpp
  ptreferential_api.cpp
  ptref_graph.cpp)
add_library(ptreferential ${PTREF_SRC})
target_link_libraries(ptreferential pb_converter data)
add_subdirectory(tests)
