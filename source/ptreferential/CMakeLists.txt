SET(PTREF_SRC
  ptreferential.cpp
  ptreferential_legacy.cpp
  ptreferential_ng.cpp
  ptreferential_api.cpp
  where.h
  reflexion.h
  ptref_graph.cpp)
add_library(ptreferential ${PTREF_SRC})

add_subdirectory(tests)
