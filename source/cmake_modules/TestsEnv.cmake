#
# Set up tests env
#

# add custom target to build docker tests not to have a strong docker dependendy
# obviously to run this target you should have docker installed (and the right to run it)
add_custom_target(docker_test)

# Fixture directory for unit testing
set(FIXTURES_DIR "${CMAKE_SOURCE_DIR}/../fixtures")

# Decomment for enabling streetnetwork debuging with quantum
#add_definitions(-D_DEBUG_DIJKSTRA_QUANTUM_)

# Active CTest
enable_testing()
