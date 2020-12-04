CONFIG   += c++14
lessThan(QT_MAJOR_VERSION, 5): error("requires Qt 5")

INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/qjsonmodel.cpp

HEADERS += \
    $$PWD/qjsonmodel.h
