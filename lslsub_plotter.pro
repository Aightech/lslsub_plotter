#-------------------------------------------------
#
# Project created by QtCreator 2019-03-08T14:30:46
#
#-------------------------------------------------
QT       += core gui datavisualization charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = lslsub_plotter
TEMPLATE = app
DESTDIR = bin
#RCC_DIR = build
#RESOURCES = build
#RES_FILE = build
#OBJECTS_DIR = build



# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
# DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0



CONFIG += c++11

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp

HEADERS += \
    include/mainwindow.h

FORMS += \
    forms/mainwindow.ui


unix:!macx: LIBS += \
                    -L./lib \
                    -Wl,-rpath,./lib \
                    -llsl64 \
                    -lboost_system\
                    -lboost_thread




INCLUDEPATH += include/lsl
DEPENDPATH += include/lsl



# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


