#-------------------------------------------------
#
# Project created by QtCreator 2012-11-26T18:48:46
#
#-------------------------------------------------

QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

rocket_base_dir = "H:/libRocket/Install"
rocket_include_dir = "$${rocket_base_dir}/include"
rocket_lib_dir = "$${rocket_base_dir}/lib"

win32:debug {
    rocket_debug_libs = RocketCore_d.lib \
                        RocketControls_d.lib \
                        RocketDebugger_d.lib
}

win32:release {
    rocket_libs = RocketCore.lib \
                  RocketControls.lib \
                  RocketDebugger.lib
}


TARGET = RoGuE
TEMPLATE = app

CONFIG += console

SOURCES += main.cpp\
        mainwindow.cpp \
    rmlsyntaxhighlighter.cpp \
    renderwidget.cpp \
    ShellRenderInterfaceOpenGL.cpp \
    qtsysteminterface.cpp

HEADERS  += mainwindow.hpp \
    rmlsyntaxhighlighter.hpp \
    renderwidget.hpp \
    ShellRenderInterfaceOpenGL.h \
    ShellOpenGL.h \
    qtsysteminterface.hpp

FORMS    += mainwindow.ui

win32:INCLUDEPATH += $${rocket_include_dir}

CONFIG( debug, debug|release ) {
    for(lib, rocket_debug_libs) {
       win32:LIBS += $${rocket_lib_dir}/$${lib}
    }
} else {
    for(lib, rocket_libs) {
       win32:LIBS += $${rocket_lib_dir}/$${lib}
    }
}

message($$LIBS)

win32:RC_FILE += \
    Rogue.rc
