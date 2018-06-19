SET(CALENDAR_SRC
    calendar_api.cpp
    calendar.cpp
)
add_library(calendar_api ${CALENDAR_SRC})
add_dependencies(calendar_api protobuf_files)
add_subdirectory(tests)
