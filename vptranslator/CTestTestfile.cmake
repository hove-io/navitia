# CMake generated Testfile for 
# Source directory: /home/runner/work/navitia/navitia/source/vptranslator
# Build directory: /home/runner/work/navitia/navitia/vptranslator
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(vptranslator_test "/vptranslator_test" "--logger=XML,all,results_vptranslator_test.xml" "--report_level=no" "")
set_tests_properties(vptranslator_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/vptranslator/CMakeLists.txt;11;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/vptranslator/CMakeLists.txt;0;")
