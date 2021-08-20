lessThan(QT_MAJOR_VERSION,5)|lessThan(QT_MINOR_VERSION,11) {
    error("Qt 5.11 or greater is required. You can build your own, or get the SDK at https://qt-project.org/downloads")
}

QMAKE_INCLUDES = $${PWD}/../qmake_includes

include($${QMAKE_INCLUDES}/artifacts.pri)
include($${QMAKE_INCLUDES}/compiler_flags.pri)
include($${QMAKE_INCLUDES}/linker_flags.pri)

TARGET = ricochet-irc
TEMPLATE = app

QT += core gui network quick widgets

isEmpty(RICOCHET_REFRESH_VERSION) {
    VERSION = devbuild
} else {
    VERSION = $${RICOCHET_REFRESH_VERSION}
}

DEFINES += "TEGO_VERSION=$${VERSION}"

# Use CONFIG+=no-hardened to disable compiler hardening options
!CONFIG(no-hardened) {
    CONFIG += hardened
    include($${QMAKE_INCLUDES}/hardened.pri)
}

macx {
    CONFIG += bundle force_debug_plist
    QT += macextras

    # Qt 5.4 introduces a bug that breaks QMAKE_INFO_PLIST when qmake has a relative path.
    # Work around by copying Info.plist directly.
    QMAKE_INFO_PLIST = Info.plist

    icons.files = icons/ricochet_refresh.icns
    icons.path = Contents/Resources/
    QMAKE_BUNDLE_DATA += icons
}

# Create a pdb for release builds as well, to enable debugging
win32-msvc2008|win32-msvc2010 {
    QMAKE_CXXFLAGS_RELEASE += /Zi
    QMAKE_LFLAGS_RELEASE += /DEBUG /OPT:REF,ICF
}


# Exclude unneeded plugins from static builds
QTPLUGIN.playlistformats = -
QTPLUGIN.imageformats = -
QTPLUGIN.printsupport = -
QTPLUGIN.mediaservice = -
# Include Linux input plugins, which are missing by default, to provide complex input support. See issue #60.
unix:!macx:QTPLUGIN.platforminputcontexts = composeplatforminputcontextplugin ibusplatforminputcontextplugin

DEFINES += QT_NO_CAST_TO_ASCII

# QML
RESOURCES +=\
    $${PWD}/../libtego_ui/ui/qml/qml.qrc 

win32:RC_ICONS = icons/ricochet_refresh.ico
OTHER_FILES += $${PWD}/../libtego_ui/ui/qml/*
lupdate_only {
    SOURCES += $${PWD}/../libtego_ui/ui/qml/*.qml
    SOURCES += $${PWD}/../libtego_ui/ui/*.cpp
    SOURCES += $${PWD}/../libtego_ui/ui/*.h
    SOURCES += $${PWD}/../libtego_ui/shims/*.cpp
    SOURCES += $${PWD}/../libtego_ui/shims/*.h
    SOURCES += $${PWD}/../libtego_ui/utils/*.cpp
    SOURCES += $${PWD}/../libtego_ui/utils/*.h
}


# Only build translations when creating the primary makefile.
!build_pass: {
    contains(QMAKE_HOST.os,Windows):QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease.exe
    else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
    for (translation, TRANSLATIONS) {
        system($$QMAKE_LRELEASE translation/$${translation}.ts -qm translation/$${translation}.qm)
    }
}

# RESOURCES += translation/embedded.qrc

CONFIG += precompile_header
PRECOMPILED_HEADER = precomp.hpp

SOURCES += main.cpp \
    IrcChannel.cpp \
    IrcConnection.cpp \
    IrcServer.cpp \
    IrcUser.cpp \
    RicochetIrcServer.cpp \
    RicochetIrcServerTask.cpp

HEADERS += \
    $${PWD}/../libtego_ui/core/ContactIDValidator.h \
    IrcChannel.h \
    IrcConnection.h \
    IrcServer.h \
    IrcUser.h \
    RicochetIrcServer.h \
    IrcConstants.h \
    RicochetIrcServerTask.h

include($${PWD}/../libtego_ui/libtego_ui.pri)
include($${PWD}/../libtego/libtego.pri)
