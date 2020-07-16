QMAKE_INCLUDES = $${PWD}/../qmake_includes

include($${QMAKE_INCLUDES}/artifacts.pri)
include($${QMAKE_INCLUDES}/compiler_flags.pri)

TEMPLATE = lib
TARGET = tego

CONFIG += staticlib

# setup precompiled headers
CONFIG += precompile_header
PRECOMPILED_HEADER = source/precomp.h

HEADERS =\
    include/tego/tego.h\
    include/tego/logger.hpp

SOURCES =\
    source/libtego.cpp\
    source/logger.cpp\

SOURCES +=\
    $${PWD}/../extern/fmt/src/format.cc\
    $${PWD}/../extern/fmt/src/os.cc

# external
INCLUDEPATH +=\
    include\
    $${PWD}/../extern/fmt/include