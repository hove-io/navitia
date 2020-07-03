# CMake generated Testfile for 
# Source directory: /home/runner/work/navitia/navitia/source/georef/tests
# Build directory: /home/runner/work/navitia/navitia/georef/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(georef_test "/georef_test" "--logger=XML,all,results_georef_test.xml" "--report_level=no" "")
set_tests_properties(georef_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/georef/tests/CMakeLists.txt;11;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/georef/tests/CMakeLists.txt;0;")
add_test(street_network_test "/street_network_test" "--logger=XML,all,results_street_network_test.xml" "--report_level=no" "")
set_tests_properties(street_network_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/georef/tests/CMakeLists.txt;15;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/georef/tests/CMakeLists.txt;0;")
add_test(path_finder_test "/path_finder_test" "--logger=XML,all,results_path_finder_test.xml" "--report_level=no" "")
set_tests_properties(path_finder_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/georef/tests/CMakeLists.txt;19;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/georef/tests/CMakeLists.txt;0;")
