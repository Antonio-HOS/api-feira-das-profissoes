QT += core gui widgets serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
TARGET = ApiFeiraDasProfissoes
TEMPLATE = app

SOURCES += \
    main.cpp \
    QtGui.cpp \
    FeiraWorker.cpp \
    Utils.cpp \
    stdafx.cpp

HEADERS += \
    QtGui.h \
    FeiraWorker.h \
    Utils.h \
    stdafx.h \
    targetver.h

FORMS += \
    QtGui.ui

RESOURCES += \
    QtGui.qrc

# Windows: porta serial
win32 {
    DEFINES += _CRT_SECURE_NO_WARNINGS
}
