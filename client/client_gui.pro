QT += widgets
CONFIG += c++17

SOURCES += main.cpp client_app.cpp
HEADERS += client_app.h

INCLUDEPATH += /opt/homebrew/opt/opencv/include/opencv4
LIBS += `pkg-config --libs opencv4`
