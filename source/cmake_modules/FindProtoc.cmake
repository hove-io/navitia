if(PROTOC_EXECUTABLE)
    # be silent if already found
    set(PROTOC_FIND_QUIETLY TRUE)
endif()

find_program(PROTOC_EXECUTABLE
    NAMES	protoc
    PATHS	/usr/bin /usr/local/bin
    DOC		"protobuf compiler")

if(PROTOC_EXECUTABLE)
    set(PROTOC_FOUND TRUE)
    if(NOT PROTOC_FIND_QUIETLY)
        message(STATUS "Found protoc: ${PROTOC_EXECUTABLE}")
    endif()
else()
    message(FATAL_ERROR "Could not find protoc executable")
endif()
