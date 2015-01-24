#-------------------------------------------------
#
# Project created by QtCreator 2014-04-15T15:07:36
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = alldeb-installer
TEMPLATE = app

SOURCES += src/main.cpp\
        src/dialog.cpp \
    src/about.cpp

HEADERS  += src/dialog.h \
    src/about.h

FORMS    += ui/dialog.ui \
    ui/about.ui

TRANSLATIONS += ts/alldeb_en.ts \
                ts/alldeb_id.ts

RESOURCES += \
    assets/icon.qrc
