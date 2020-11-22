lessThan(QT_MAJOR_VERSION,5)|lessThan(QT_MINOR_VERSION,15) {
    error("Qt 5.15 or greater is required. You can build your own, or get the SDK at https://qt-project.org/downloads")
}

QMAKE_INCLUDES = $${PWD}/../qmake_includes

include($${QMAKE_INCLUDES}/artifacts.pri)
include($${QMAKE_INCLUDES}/compiler_flags.pri)
include($${QMAKE_INCLUDES}/linker_flags.pri)

TARGET = ricochet-irc
TEMPLATE = app

QT += core gui network quick widgets

VERSION = 3.0.0a

DEFINES += "TEGO_VERSION=$${VERSION}"

# Use CONFIG+=no-hardened to disable compiler hardening options
!CONFIG(no-hardened) {
    CONFIG += hardened
    include($${QMAKE_INCLUDES}/hardened.pri)
}

# Pass DEFINES+=RICOCHET_NO_PORTABLE for a system-wide installation

CONFIG(release,debug|release):DEFINES += QT_NO_DEBUG_OUTPUT QT_NO_WARNING_OUTPUT

contains(DEFINES, RICOCHET_NO_PORTABLE) {
    unix:!macx {
        target.path = /usr/bin
        INSTALLS += target

        exists(tor) {
            message(Adding bundled Tor to installations)
            bundletor.path = /usr/lib/ricochet/tor/
            bundletor.files = tor/*
            INSTALLS += bundletor
            DEFINES += BUNDLED_TOR_PATH=\\\"/usr/lib/ricochet/tor/\\\"
        }
    }
}

LIBS += -lprotobuf

macx {
    CONFIG += bundle force_debug_plist
    QT += macextras

    # Qt 5.4 introduces a bug that breaks QMAKE_INFO_PLIST when qmake has a relative path.
    # Work around by copying Info.plist directly.
    greaterThan(QT_MAJOR_VERSION,5)|greaterThan(QT_MINOR_VERSION,4) {
        QMAKE_INFO_PLIST = Info.plist
    } else:equals(QT_MAJOR_VERSION,5):lessThan(QT_MINOR_VERSION,4) {
        QMAKE_INFO_PLIST = Info.plist
    } else {
        CONFIG += no_plist
        QMAKE_POST_LINK += cp $${_PRO_FILE_PWD_}/Info.plist $${OUT_PWD}/$${TARGET}.app/Contents/;
    }

    exists(tor) {
        # Copy the entire tor/ directory, which should contain tor/tor (the binary itself)
        QMAKE_POST_LINK += cp -R $${_PRO_FILE_PWD_}/tor $${OUT_PWD}/$${TARGET}.app/Contents/MacOS/;
    }

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

HEADERS += \
    $${PWD}/../libtego_ui/core/ContactIDValidator.h \
    IrcChannel.h \
    IrcConnection.h \
    IrcServer.h \
    IrcUser.h \
    RicochetIrcServer.h \
    IrcConstants.h

include($${PWD}/../libtego_ui/libtego_ui.pri)
include($${PWD}/../libtego/libtego.pri)
