if(Boost_VERSION)
    # be silent if already found
    set(Boost_FIND_QUIETLY TRUE)
endif()

set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.55.0 COMPONENTS unit_test_framework thread regex
    serialization date_time filesystem system regex chrono iostreams
    program_options REQUIRED)

#boost 1.53/1.54 bugs with local datetime...
#see http://stackoverflow.com/questions/15234527/boost-1-53-local-date-time-compiler-error-with-std-c0x
add_definitions(-DBOOST_NO_CXX11_EXPLICIT_CONVERSION_OPERATORS)

link_directories(${Boost_LIBRARY_DIRS})
include_directories("${Boost_INCLUDE_DIRS}")

set(TEST_DEBUG_OUTPUT OFF CACHE BOOL "Shoot test output to stdin")

# Add args for Boost test depending on the version (log_level, etc...)
macro(BUILD_BOOST_TEST_ARGS EXE_NAME)
    if("${Boost_VERSION}" LESS 106200)
        if(TEST_DEBUG_OUTPUT)
            set(BOOST_TEST_ARGS --log_level=all)
        else()
            set(BOOST_TEST_ARGS --log_format=XML --log_sink=results_${EXE_NAME}.xml --log_level=all --report_level=no)
        endif()
    else()
        if(TEST_DEBUG_OUTPUT)
            set(BOOST_TEST_ARGS --logger=HRF,all )
        else()
            set(BOOST_TEST_ARGS --logger=XML,all,results_${EXE_NAME}.xml --report_level=no)
        endif()
    endif()
endmacro(BUILD_BOOST_TEST_ARGS)

macro(ADD_BOOST_TEST EXE_NAME)
    BUILD_BOOST_TEST_ARGS(${EXE_NAME})
    add_test(
        NAME ${EXE_NAME}
        COMMAND "${EXECUTABLE_OUTPUT_PATH}/${EXE_NAME}" ${BOOST_TEST_ARGS} "${ARGN}"
    )
endmacro(ADD_BOOST_TEST)

message("-- Found Boost version: ${Boost_VERSION}")
if ("${Boost_VERSION}" MATCHES "^[0-9]+$")
    if("${Boost_VERSION}" GREATER 106900 OR "${Boost_VERSION}" EQUAL 106900)
        add_definitions(-DBOOST_MATH_DISABLE_STD_FPCLASSIFY)
    endif()
else()
    if("${Boost_VERSION}" VERSION_GREATER_EQUAL 1.69.0)
        add_definitions(-DBOOST_MATH_DISABLE_STD_FPCLASSIFY)
    endif()
endif()
