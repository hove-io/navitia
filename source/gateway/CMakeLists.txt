FIND_PACKAGE(WS_Commons)

include_directories (${CMAKE_SOURCE_DIR}/http_lib)
link_directories (${NAViTiA_SOURCE_DIR}/http_dll) 

#add_executable (gateway gateway.cpp gateway.h)
#target_link_libraries(gateway ${WS_LIBSS})

SET(SOURCE_FILES gateway.cpp gateway.h)
SET_WS_TYPE(gateway "ISAPI" ${SOURCE_FILES} http)