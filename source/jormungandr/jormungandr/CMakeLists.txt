INSTALL_PROGRAMS(/srv/jormungandr jormungandr.py)
INSTALL_FILES(/srv/jormungandr FILES find_extrem_datetimes.py instance_manager.py protobuf_to_dict.py swagger.py ${CMAKE_CURRENT_SOURCE_DIR}/type_pb2.py singleton.py universals.py ${CMAKE_CURRENT_SOURCE_DIR}/request_pb2.py ${CMAKE_CURRENT_SOURCE_DIR}/response_pb2.py error.py authentification.py app.py default_settings.py db.py renderers.py jormungandr_exceptions.py cache.py index.py instance.py)

add_subdirectory("scripts")
add_subdirectory("interfaces")

add_test(jormungandr nosetests --where=${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(jormungandr PROPERTIES ENVIRONMENT
    PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR})
