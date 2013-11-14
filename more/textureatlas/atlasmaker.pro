# -------------------------------------------------
# Project created by QtCreator 2009-03-18T08:26:06
# -------------------------------------------------
QT += gui \
    console
TARGET = textureatlasmaker
CONFIG += release

# DESTDIR = release
TEMPLATE = app
SOURCES += src/main.cpp \
    src/mainwindow.cpp \
    tinyxml/tinyxml.cpp \
    tinyxml/tinyxmlparser.cpp \
    tinyxml/tinyxmlerror.cpp \
    tinyxml/tinystr.cpp \
    src/texturemodel.cpp \
    src/texturelistwidget.cpp \
    src/workarea.cpp \
    src/arrangethread.cpp
HEADERS += src/mainwindow.h \
    tinyxml/tinyxml.h \
    tinyxml/tinystr.h \
    src/texturemodel.h \
    src/texturelistwidget.h \
    src/workarea.h \
    src/common.h \
    src/arrangethread.h
FORMS += src/mainform.ui
RESOURCES += src/texture.qrc
OBJECTS_DIR = obj
MOC_DIR = obj
UI_DIR = obj
RCC_DIR = obj/res
INCLUDEPATH += tinyxml
INCLUDEPATH += src
