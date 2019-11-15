SET(CALENDAR_SRC
    calendar_api.cpp
    calendar.cpp
)
add_library(calendar_api ${CALENDAR_SRC})
add_subdirectory(tests)
