# CMake generated Testfile for 
# Source directory: /home/runner/work/navitia/navitia/source/jormungandr
# Build directory: /home/runner/work/navitia/navitia/jormungandr
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(jormungandr_serpy "py.test" "--doctest-modules" "--junit-xml=/home/runner/work/navitia/navitia/nosetest_jormungandr_serpy.xml" "--cov-report=xml:/home/runner/work/navitia/navitia/jormungandr_coverage_serpy.xml" "--cov=jormungandr" "/home/runner/work/navitia/navitia/source/jormungandr/jormungandr")
set_tests_properties(jormungandr_serpy PROPERTIES  ENVIRONMENT "PYTHONPATH=/home/runner/work/navitia/navitia/source/jormungandr:/home/runner/work/navitia/navitia/source/navitiacommon;JORMUNGANDR_CONFIG_FILE=/home/runner/work/navitia/navitia/source/jormungandr/tests/serpy_settings.py" _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/jormungandr/CMakeLists.txt;8;add_test;/home/runner/work/navitia/navitia/source/jormungandr/CMakeLists.txt;0;")
add_test(jormungandr_serpy_integration_tests "py.test" "--doctest-modules" "--junit-xml=/home/runner/work/navitia/navitia/nosetest_jormungandr_serpy_integration_tests.xml" "/home/runner/work/navitia/navitia/source/jormungandr/tests" "--cov=jormungandr" "--cov-report=xml:/home/runner/work/navitia/navitia/jormungandr_serpy_integration_coverage.xml")
set_tests_properties(jormungandr_serpy_integration_tests PROPERTIES  ENVIRONMENT "PYTHONPATH=/home/runner/work/navitia/navitia/source/jormungandr:/home/runner/work/navitia/navitia/source/navitiacommon;KRAKEN_BUILD_DIR=/home/runner/work/navitia/navitia" _BACKTRACE_TRIPLES "/home/runner/work/navitia/navitia/source/jormungandr/CMakeLists.txt;19;add_test;/home/runner/work/navitia/navitia/source/jormungandr/CMakeLists.txt;0;")
