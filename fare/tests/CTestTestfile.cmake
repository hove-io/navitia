# CMake generated Testfile for 
# Source directory: /home/runner/work/navitia/navitia/source/fare/tests
# Build directory: /home/runner/work/navitia/navitia/fare/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(fare_test "/fare_test" "--logger=XML,all,results_fare_test.xml" "--report_level=no" "")
set_tests_properties(fare_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/fare/tests/CMakeLists.txt;3;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/fare/tests/CMakeLists.txt;0;")
add_test(fare_integration_test "/fare_integration_test" "--logger=XML,all,results_fare_integration_test.xml" "--report_level=no" "")
set_tests_properties(fare_integration_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/fare/tests/CMakeLists.txt;7;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/fare/tests/CMakeLists.txt;0;")
