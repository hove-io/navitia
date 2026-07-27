SET(CALENDAR_SRC
    calendar_api.cpp
    calendar.cpp
)
add_library(calendar_api ${CALENDAR_SRC})
# calendar_api.h includes the generated type/*.pb.h; make sure protoc has run.
add_dependencies(calendar_api protobuf_files)

# Add tests
if(NOT SKIP_TESTS)
    add_subdirectory(tests)
endif(NOT SKIP_TESTS)
