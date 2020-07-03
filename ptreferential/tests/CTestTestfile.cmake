# CMake generated Testfile for 
# Source directory: /home/runner/work/navitia/navitia/source/ptreferential/tests
# Build directory: /home/runner/work/navitia/navitia/ptreferential/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(ptref_test "/ptref_test" "--logger=XML,all,results_ptref_test.xml" "--report_level=no" "")
set_tests_properties(ptref_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/ptreferential/tests/CMakeLists.txt;8;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/ptreferential/tests/CMakeLists.txt;0;")
add_test(ptref_ng_test "/ptref_ng_test" "--logger=XML,all,results_ptref_ng_test.xml" "--report_level=no" "")
set_tests_properties(ptref_ng_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/ptreferential/tests/CMakeLists.txt;12;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/ptreferential/tests/CMakeLists.txt;0;")
add_test(ptref_odt_level_test "/ptref_odt_level_test" "--logger=XML,all,results_ptref_odt_level_test.xml" "--report_level=no" "")
set_tests_properties(ptref_odt_level_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/ptreferential/tests/CMakeLists.txt;16;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/ptreferential/tests/CMakeLists.txt;0;")
add_test(ptref_companies_test "/ptref_companies_test" "--logger=XML,all,results_ptref_companies_test.xml" "--report_level=no" "")
set_tests_properties(ptref_companies_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/ptreferential/tests/CMakeLists.txt;20;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/ptreferential/tests/CMakeLists.txt;0;")
add_test(vehicle_journey_test "/vehicle_journey_test" "--logger=XML,all,results_vehicle_journey_test.xml" "--report_level=no" "")
set_tests_properties(vehicle_journey_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/ptreferential/tests/CMakeLists.txt;24;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/ptreferential/tests/CMakeLists.txt;0;")
