SET(vptranslator_src
    vptranslator.h
    vptranslator.cpp)
add_library(vptranslator ${vptranslator_src})
target_link_libraries(vptranslator types)

# Add tests
if(NOT SKIP_TESTS)
    add_subdirectory(tests)
endif(NOT SKIP_TESTS)
