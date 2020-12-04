#-------------------------------------------------
#
# Project created by QtCreator 2015-01-22T08:20:52
#
#-------------------------------------------------

QT       += core gui widgets
CONFIG   += c++14
lessThan(QT_MAJOR_VERSION, 5): error("requires Qt 5")

TARGET = QJsonModel
TEMPLATE = app

include(QJsonModel.pri)

SOURCES += \
    main.cpp \
