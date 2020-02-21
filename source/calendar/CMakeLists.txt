SET(CALENDAR_SRC
    calendar_api.cpp
    calendar.cpp
)
add_library(calendar_api ${CALENDAR_SRC})

# Add tests
if(NOT SKIP_TESTS)
    add_subdirectory(tests)
endif(NOT SKIP_TESTS)
