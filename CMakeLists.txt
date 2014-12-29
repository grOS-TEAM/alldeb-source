cmake_minimum_required(VERSION 2.8.7)
PROJECT(alldeb-installer)

#include_directories(ts)
#include_directories(ui)

set(CMAKE_AUTOMOC ON)

file(GLOB TERJEMAH ts/alldeb_*.ts)

FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})

qt4_wrap_ui(UISrcs ui/dialog.ui ui/about.ui)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
qt4_add_resources(QT_res assets/icon.qrc)
qt4_add_translation(QM_files ${TERJEMAH})

ADD_EXECUTABLE(alldeb-installer src/main.cpp src/dialog.cpp src/about.cpp ${MOCSrcs} ${UISrcs} ${QT_res} ${QM_files})
TARGET_LINK_LIBRARIES(alldeb-installer ${QT_LIBRARIES})

install(TARGETS alldeb-installer RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(FILES ${QM_files} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/alldeb/installer/lang)
install(FILES alldeb-installer.desktop DESTINATION /usr/share/applications)

