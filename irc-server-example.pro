QT += core
QT -= gui

CONFIG += c++11

TARGET = irc-server-example
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app
QT = core network

SOURCES += src/irc-server-example.cpp \
    src/irc/IrcChannel.cpp \
    src/irc/IrcConnection.cpp \
    src/irc/IrcServer.cpp \
    src/irc/IrcUser.cpp \
    src/utils/QRegularExpressionValidator.cpp \
    src/irc/ExampleIrcServer.cpp

HEADERS += \
    src/irc/irc.h \
    src/irc/IrcChannel.h \
    src/irc/IrcConnection.h \
    src/irc/IrcServer.h \
    src/irc/IrcUser.h \
    src/utils/QRegularExpressionValidator.h \
    src/irc/ExampleIrcServer.h \
    src/irc/IrcConstants.h
