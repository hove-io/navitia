# Install script for directory: /home/runner/work/navitia/navitia/source/sql

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/share/navitia/ed/script/alembic.ini;/usr/share/navitia/ed/script/env.py")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/share/navitia/ed/script" TYPE FILE FILES
    "/home/runner/work/navitia/navitia/source/sql/alembic.ini"
    "/home/runner/work/navitia/navitia/source/sql/alembic/env.py"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/share/navitia/ed/alembic/versions/11b1fb5bd523_add_boarding_and_alighting_time_to_stop_.py;/usr/share/navitia/ed/alembic/versions/12660cd87568_main_destination_for_route.py;/usr/share/navitia/ed/alembic/versions/13673746db16_postal_codes_table_added.py;/usr/share/navitia/ed/alembic/versions/14346346596e_add_direction_type_column_in_route_table.py;/usr/share/navitia/ed/alembic/versions/14e13cf7a042_manage_multi_codes.py;/usr/share/navitia/ed/alembic/versions/1651f9b235f5_journey_pattern_suppression.py;/usr/share/navitia/ed/alembic/versions/1cec3dee6597_feed_info_table_added.py;/usr/share/navitia/ed/alembic/versions/1faee1d2551a_add_headsign_in_vehiclejourney.py;/usr/share/navitia/ed/alembic/versions/224621d9edde_timezone_at_metavj_level.py;/usr/share/navitia/ed/alembic/versions/23dd34bfaeaf_co2_emision_nullable.py;/usr/share/navitia/ed/alembic/versions/25b7f3ace052_type_comments.py;/usr/share/navitia/ed/alembic/versions/27fae61b6786_add_shape_table.py;/usr/share/navitia/ed/alembic/versions/291c8dbbaed1_create_line_group_and_line_group_link_.py;/usr/share/navitia/ed/alembic/versions/29fc422c56cb_add_areas_in_stop_points.py;/usr/share/navitia/ed/alembic/versions/2c510fae878d_add_calendar_to_navitia_types.py;/usr/share/navitia/ed/alembic/versions/2d86200bcb93_co2_emission_column_added.py;/usr/share/navitia/ed/alembic/versions/2e1a3cb6555b_headsign_in_stop_time.py;/usr/share/navitia/ed/alembic/versions/35c20e6103d6_create_tables.py;/usr/share/navitia/ed/alembic/versions/3b17c15627cb_meta_vj_calendar.py;/usr/share/navitia/ed/alembic/versions/3bea0b3cb116_create_object_properties_table.py;/usr/share/navitia/ed/alembic/versions/3dc7d057ec3_rename_frame_to_dataset.py;/usr/share/navitia/ed/alembic/versions/40d9139ba0ed_shape_in_line_and_route.py;/usr/share/navitia/ed/alembic/versions/43822e80c45a_change_bounding_shape_function.py;/usr/share/navitia/ed/alembic/versions/4429fe91ac94_add_poi_and_streetnetwork_source.py;/usr/share/navitia/ed/alembic/versions/45719cfc11f9_change_parameter_shape_to_multipolygon.py;/usr/share/navitia/ed/alembic/versions/46168c7abc89_change_fare_zone_id_to_string.py;/usr/share/navitia/ed/alembic/versions/47b663b9defc_remove_global_timezone.py;/usr/share/navitia/ed/alembic/versions/4a47bcf84927_force_multipolygon_in_compute_bounding_.py;/usr/share/navitia/ed/alembic/versions/4ccd66ece649_add_vj_class_and_metavj_in_vj.py;/usr/share/navitia/ed/alembic/versions/4f4ee492129f_foreign_key_datasets.py;/usr/share/navitia/ed/alembic/versions/52b017632678_add_functions.py;/usr/share/navitia/ed/alembic/versions/538bc4ea9cd1_multiple_comments.py;/usr/share/navitia/ed/alembic/versions/53c1e84535f8_add_closing_opening_time_on_line.py;/usr/share/navitia/ed/alembic/versions/56c1c7a19078_add_visible_parameter.py;/usr/share/navitia/ed/alembic/versions/5812ecbcd76d_shape_in_journey_pattern_point.py;/usr/share/navitia/ed/alembic/versions/59f4456a029_license_and_website_in_contributor.py;/usr/share/navitia/ed/alembic/versions/5a590ae95255_manage_frames.py;/usr/share/navitia/ed/alembic/versions/795308e6d7c_add_parameter_parse_pois_from_osm.py;/usr/share/navitia/ed/alembic/versions/82749d34a18_frequency_vj.py;/usr/share/navitia/ed/alembic/versions/844a9fa86ad2_add_car_speed_on_edges.py;/usr/share/navitia/ed/alembic/versions/90f4275525_add_data.py;/usr/share/navitia/ed/alembic/versions/c3da2283d4a_text_color_line.py")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/share/navitia/ed/alembic/versions" TYPE FILE FILES
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/11b1fb5bd523_add_boarding_and_alighting_time_to_stop_.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/12660cd87568_main_destination_for_route.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/13673746db16_postal_codes_table_added.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/14346346596e_add_direction_type_column_in_route_table.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/14e13cf7a042_manage_multi_codes.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/1651f9b235f5_journey_pattern_suppression.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/1cec3dee6597_feed_info_table_added.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/1faee1d2551a_add_headsign_in_vehiclejourney.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/224621d9edde_timezone_at_metavj_level.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/23dd34bfaeaf_co2_emision_nullable.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/25b7f3ace052_type_comments.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/27fae61b6786_add_shape_table.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/291c8dbbaed1_create_line_group_and_line_group_link_.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/29fc422c56cb_add_areas_in_stop_points.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/2c510fae878d_add_calendar_to_navitia_types.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/2d86200bcb93_co2_emission_column_added.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/2e1a3cb6555b_headsign_in_stop_time.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/35c20e6103d6_create_tables.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/3b17c15627cb_meta_vj_calendar.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/3bea0b3cb116_create_object_properties_table.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/3dc7d057ec3_rename_frame_to_dataset.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/40d9139ba0ed_shape_in_line_and_route.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/43822e80c45a_change_bounding_shape_function.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/4429fe91ac94_add_poi_and_streetnetwork_source.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/45719cfc11f9_change_parameter_shape_to_multipolygon.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/46168c7abc89_change_fare_zone_id_to_string.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/47b663b9defc_remove_global_timezone.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/4a47bcf84927_force_multipolygon_in_compute_bounding_.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/4ccd66ece649_add_vj_class_and_metavj_in_vj.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/4f4ee492129f_foreign_key_datasets.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/52b017632678_add_functions.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/538bc4ea9cd1_multiple_comments.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/53c1e84535f8_add_closing_opening_time_on_line.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/56c1c7a19078_add_visible_parameter.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/5812ecbcd76d_shape_in_journey_pattern_point.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/59f4456a029_license_and_website_in_contributor.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/5a590ae95255_manage_frames.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/795308e6d7c_add_parameter_parse_pois_from_osm.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/82749d34a18_frequency_vj.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/844a9fa86ad2_add_car_speed_on_edges.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/90f4275525_add_data.py"
    "/home/runner/work/navitia/navitia/source/sql/alembic/versions/c3da2283d4a_text_color_line.py"
    )
endif()

