cmake_minimum_required(VERSION 2.8.7)
PROJECT(alldeb-installer)

set(CMAKE_AUTOMOC ON)
set(terjemah alldeb_en.ts alldeb_id.ts)

FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
 
QT4_WRAP_UI(UISrcs dialog.ui about.ui)
#QT4_WRAP_CPP(MOCSrcs dialog.h about.h)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

#ADD_LIBRARY(terjemah ${sources} ${UISrcs})

#qt4_create_translation(qm_files alldeb_en.ts alldeb_id.ts)
qt4_add_translation(qm_files ${terjemah})
#    ${terjemah_SRCS} )

ADD_EXECUTABLE(alldeb-installer main.cpp dialog.cpp about.cpp ${MOCSrcs} ${UISrcs} ${qm_files})
TARGET_LINK_LIBRARIES(alldeb-installer ${QT_LIBRARIES})

install(TARGETS alldeb-installer RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(FILES ${qm_files} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/alldeb/installer/lang)
install(FILES alldeb-installer.desktop DESTINATION /usr/share/applications)
install(FILES alldeb.xml DESTINATION /usr/share/mime/application)
