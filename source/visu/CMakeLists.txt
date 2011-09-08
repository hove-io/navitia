FIND_PACKAGE(Boost 1.40.0 COMPONENTS serialization date_time regex thread iostreams REQUIRED)
FIND_PACKAGE(Qt4 REQUIRED)

link_directories (${Boost_LIBRARY_DIRS})
include_directories ( ${Boost_INCLUDE_DIRS}
                      ${CMAKE_SOURCE_DIR}
                      ${CMAKE_SOURCE_DIR}/fastlz_filter
                      ${CMAKE_SOURCE_DIR}/type
                      ${CMAKE_SOURCE_DIR}/../third_party
                      ${CMAKE_SOURCE_DIR}/../third_party/fastlz
                      ${PROTOBUF_INCLUDE_DIRS}
                      ${CMAKE_CURRENT_BINARY_DIR}/../type
                      ${CMAKE_CURRENT_BINARY_DIR}
)

include(${QT_USE_FILE})
QT4_WRAP_UI(qt_UI navisu.ui)
QT4_WRAP_CPP(qt_MOC navisu.h)
add_executable(navisu ${qt_UI} ${qt_MOC} navisu.cpp)
target_link_libraries(navisu ${QT_LIBRARIES} types ${Boost_LIBRARIES} )
