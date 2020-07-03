# CMake generated Testfile for 
# Source directory: /home/runner/work/navitia/navitia/source/autocomplete
# Build directory: /home/runner/work/navitia/navitia/autocomplete
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(autocomplete_test "/autocomplete_test" "--logger=XML,all,results_autocomplete_test.xml" "--report_level=no" "")
set_tests_properties(autocomplete_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/autocomplete/CMakeLists.txt;10;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/autocomplete/CMakeLists.txt;0;")
