add_executable(dumpsn dumpsn.cpp)
target_link_libraries(dumpsn
  routing  boost_program_options data fare routing georef utils autocomplete time_tables
  ${BOOST_LIBS} log4cplus pb_lib protobuf)
