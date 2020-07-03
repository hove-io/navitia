# CMake generated Testfile for 
# Source directory: /home/runner/work/navitia/navitia/source/routing/tests
# Build directory: /home/runner/work/navitia/navitia/routing/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(raptor_test "/raptor_test" "--logger=XML,all,results_raptor_test.xml" "--report_level=no" "")
set_tests_properties(raptor_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;10;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;0;")
add_test(reverse_raptor_test "/reverse_raptor_test" "--logger=XML,all,results_reverse_raptor_test.xml" "--report_level=no" "")
set_tests_properties(reverse_raptor_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;14;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;0;")
add_test(frequency_raptor_test "/frequency_raptor_test" "--logger=XML,all,results_frequency_raptor_test.xml" "--report_level=no" "")
set_tests_properties(frequency_raptor_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;18;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;0;")
add_test(routing_api_test "/routing_api_test" "--logger=XML,all,results_routing_api_test.xml" "--report_level=no" "")
set_tests_properties(routing_api_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;22;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;0;")
add_test(next_stop_time_test "/next_stop_time_test" "--logger=XML,all,results_next_stop_time_test.xml" "--report_level=no" "")
set_tests_properties(next_stop_time_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;26;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;0;")
add_test(disruptions_test "/disruptions_test" "--logger=XML,all,results_disruptions_test.xml" "--report_level=no" "")
set_tests_properties(disruptions_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;30;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;0;")
add_test(co2_emission_test "/co2_emission_test" "--logger=XML,all,results_co2_emission_test.xml" "--report_level=no" "")
set_tests_properties(co2_emission_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;34;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;0;")
add_test(journey_pattern_container_test "/journey_pattern_container_test" "--logger=XML,all,results_journey_pattern_container_test.xml" "--report_level=no" "")
set_tests_properties(journey_pattern_container_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;38;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;0;")
add_test(get_stop_times_test "/get_stop_times_test" "--logger=XML,all,results_get_stop_times_test.xml" "--report_level=no" "")
set_tests_properties(get_stop_times_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;42;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;0;")
add_test(isochrone_test "/isochrone_test" "--logger=XML,all,results_isochrone_test.xml" "--report_level=no" "")
set_tests_properties(isochrone_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;46;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;0;")
add_test(heat_map_test "/heat_map_test" "--logger=XML,all,results_heat_map_test.xml" "--report_level=no" "")
set_tests_properties(heat_map_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;50;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;0;")
add_test(journey_test "/journey_test" "--logger=XML,all,results_journey_test.xml" "--report_level=no" "")
set_tests_properties(journey_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;54;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/routing/tests/CMakeLists.txt;0;")
