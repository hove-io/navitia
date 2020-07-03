# CMake generated Testfile for 
# Source directory: /home/runner/work/navitia/navitia/source/calendar/tests
# Build directory: /home/runner/work/navitia/navitia/calendar/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(calendar_api_test "/calendar_api_test" "--logger=XML,all,results_calendar_api_test.xml" "--report_level=no" "")
set_tests_properties(calendar_api_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/calendar/tests/CMakeLists.txt;3;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/calendar/tests/CMakeLists.txt;0;")
