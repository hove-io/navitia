# CMake generated Testfile for 
# Source directory: /home/runner/work/navitia/navitia/source/ed/tests
# Build directory: /home/runner/work/navitia/navitia/ed/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(jpp_shape_test "/jpp_shape_test" "--logger=XML,all,results_jpp_shape_test.xml" "--report_level=no" "")
set_tests_properties(jpp_shape_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;10;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;0;")
add_test(gtfs_parser_test "/gtfs_parser_test" "--logger=XML,all,results_gtfs_parser_test.xml" "--report_level=no" "")
set_tests_properties(gtfs_parser_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;14;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;0;")
add_test(fusio_parser_test "/fusio_parser_test" "--logger=XML,all,results_fusio_parser_test.xml" "--report_level=no" "")
set_tests_properties(fusio_parser_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;18;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;0;")
add_test(osm_tags_reader_test "/osm_tags_reader_test" "--logger=XML,all,results_osm_tags_reader_test.xml" "--report_level=no" "")
set_tests_properties(osm_tags_reader_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;22;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;0;")
add_test(associated_calendar_test "/associated_calendar_test" "--logger=XML,all,results_associated_calendar_test.xml" "--report_level=no" "")
set_tests_properties(associated_calendar_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;26;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;0;")
add_test(fare_parser_test "/fare_parser_test" "--logger=XML,all,results_fare_parser_test.xml" "--report_level=no" "")
set_tests_properties(fare_parser_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;30;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;0;")
add_test(conv_coord_test "/conv_coord_test" "--logger=XML,all,results_conv_coord_test.xml" "--report_level=no" "")
set_tests_properties(conv_coord_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;34;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;0;")
add_test(shift_stop_times_test "/shift_stop_times_test" "--logger=XML,all,results_shift_stop_times_test.xml" "--report_level=no" "")
set_tests_properties(shift_stop_times_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;38;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;0;")
add_test(osm2ed_test "/osm2ed_test" "--logger=XML,all,results_osm2ed_test.xml" "--report_level=no" "")
set_tests_properties(osm2ed_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;42;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;0;")
add_test(poi2ed_test "/poi2ed_test" "--logger=XML,all,results_poi2ed_test.xml" "--report_level=no" "/home/runner/work/navitia/navitia/source/../fixtures/ed/poi/poi")
set_tests_properties(poi2ed_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;46;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;0;")
add_test(ed2nav_test "/ed2nav_test" "--logger=XML,all,results_ed2nav_test.xml" "--report_level=no" "")
set_tests_properties(ed2nav_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;53;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;0;")
add_test(route_main_destination_test "/route_main_destination_test" "--logger=XML,all,results_route_main_destination_test.xml" "--report_level=no" "")
set_tests_properties(route_main_destination_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;57;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;0;")
add_test(speed_parser_test "/speed_parser_test" "--logger=XML,all,results_speed_parser_test.xml" "--report_level=no" "")
set_tests_properties(speed_parser_test PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/cmake_modules/UseBoost.cmake;40;add_test;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;62;ADD_BOOST_TEST;/home/runner/work/navitia/navitia/source/ed/tests/CMakeLists.txt;0;")
