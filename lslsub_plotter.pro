#-------------------------------------------------
#
# Project created by QtCreator 2019-03-08T14:30:46
#
#-------------------------------------------------
QT       += core gui datavisualization charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = lslsub_plotter
TEMPLATE = app

CONFIG += c++11

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp

HEADERS += \
    include/mainwindow.h

FORMS += \
    forms/mainwindow.ui

LIBS += -L./lib


unix:!macx: LIBS += \
                    -Wl,-rpath,./lib \
                    -llsl64


win32: LIBS += -lliblsl64

INCLUDEPATH += include/lsl
DEPENDPATH += include/lsl

win32:DESTDIR = bin/WIN32/Window
win32:TARGET_CUSTOM_EXT = .exe
win32:DEPLOY_COMMAND = windeployqt
win32:DEPLOY_TARGET = $$shell_quote($$shell_path($${OUT_PWD}/$${DESTDIR}/$${TARGET}$${TARGET_CUSTOM_EXT}))

#warning($${DEPLOY_COMMAND} $${DEPLOY_TARGET})

#QMAKE_PRE_LINK+= $$quote(copy $$shell_path(bin\liblsl64.dll) $$shell_path($${DESTDIR}/))

win32 {
    CONFIG(release, debug|release)
    {
        QMAKE_POST_LINK += $$quote($${DEPLOY_COMMAND} $${DEPLOY_TARGET})
        #QMAKE_POST_LINK += $$quote($$escape_expand(\n\t) copy $$shell_path(lib/liblsl64.dll) $$shell_path($${DESTDIR}/liblsl64.dll))
        QMAKE_POST_LINK += $$quote($$escape_expand(\n\t) del Makefile)
        QMAKE_POST_LINK += $$quote($$escape_expand(\n\t) del Makefile.Debug)
        QMAKE_POST_LINK += $$quote($$escape_expand(\n\t) del Makefile.Release)
        QMAKE_POST_LINK += $$quote($$escape_expand(\n\t) del .qmake.stash)
        QMAKE_POST_LINK += $$quote($$escape_expand(\n\t) del ui_mainwindow.h)
        QMAKE_POST_LINK += $$quote($$escape_expand(\n\t) rmdir /S /Q release)
        QMAKE_POST_LINK += $$quote($$escape_expand(\n\t) rmdir /S /Q debug)
    }
}


unix {
    CONFIG(release, debug|release)
    {
        QMAKE_POST_LINK += $$quote($$escape_expand(\n\t) rm Makefile)
        QMAKE_POST_LINK += $$quote($$escape_expand(\n\t) rm .qmake.stash)
        QMAKE_POST_LINK += $$quote($$escape_expand(\n\t) rm lslsub_plotter.pro.user)
        QMAKE_POST_LINK += $$quote($$escape_expand(\n\t) rm *.h)
        QMAKE_POST_LINK += $$quote($$escape_expand(\n\t) rm *.cpp)
        QMAKE_POST_LINK += $$quote($$escape_expand(\n\t) rm *.o)
        #QMAKE_POST_LINK += $$quote($$escape_expand(\n\t) $$shell_path($${OUT_PWD}/mkshcut.bat) $$shell_path($${DESTDIR}/$${TARGET}$${TARGET_CUSTOM_EXT}))
    }
}


