FIND_PACKAGE(Qt4)

include_directories ( ${CMAKE_CURRENT_SOURCE_DIR}
                      /usr/include/marble
)

IF(Qt4_FOUND)
    include(${QT_USE_FILE})
    QT4_WRAP_UI(qt_UI navisu.ui)
    QT4_WRAP_CPP(qt_MOC navisu.h OPTIONS -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED)
    add_executable(navisu ${qt_UI} ${qt_MOC} navisu.cpp)
    target_link_libraries(navisu ${QT_LIBRARIES} data marblewidget utils)
ENDIF(Qt4_FOUND)
