#-------------------------------------------------
#
# Project created by QtCreator 2014-04-15T15:07:36
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = alldeb-installer
TEMPLATE = app


SOURCES += main.cpp\
        dialog.cpp \
    about.cpp

HEADERS  += dialog.h \
    about.h

FORMS    += dialog.ui \
    about.ui

TRANSLATIONS += alldeb_en.ts \
                alldeb_id.ts
