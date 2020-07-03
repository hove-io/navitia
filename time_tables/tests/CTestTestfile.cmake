# CMake generated Testfile for 
# Source directory: /home/runner/work/navitia/navitia/source/time_tables/tests
# Build directory: /home/runner/work/navitia/navitia/time_tables/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(thermometer_test "/thermometer_test" "--logger=XML,all,results_thermometer_test.xml" "--report_level=no" "")
set_tests_properties(thermometer_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/time_tables/tests/CMakeLists.txt;9;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/time_tables/tests/CMakeLists.txt;0;")
add_test(departure_boards_test "/departure_boards_test" "--logger=XML,all,results_departure_boards_test.xml" "--report_level=no" "")
set_tests_properties(departure_boards_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/time_tables/tests/CMakeLists.txt;19;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/time_tables/tests/CMakeLists.txt;0;")
add_test(route_schedules_test "/route_schedules_test" "--logger=XML,all,results_route_schedules_test.xml" "--report_level=no" "")
set_tests_properties(route_schedules_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/time_tables/tests/CMakeLists.txt;23;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/time_tables/tests/CMakeLists.txt;0;")
add_test(passages_test "/passages_test" "--logger=XML,all,results_passages_test.xml" "--report_level=no" "")
set_tests_properties(passages_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/time_tables/tests/CMakeLists.txt;28;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/time_tables/tests/CMakeLists.txt;0;")
