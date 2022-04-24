cmake_minimum_required(VERSION 3.16)

project(ricochet-irc LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (FORCE_QT5)
    find_package(
        QT
        NAMES
        Qt5
        COMPONENTS Core
                Gui
                Network
                Widgets
        REQUIRED)
else ()
    find_package(
        QT
        NAMES
        Qt6
        Qt5
        COMPONENTS Core
                Gui
                Network
                Widgets
        REQUIRED)
endif ()

find_package(
    Qt${QT_VERSION_MAJOR}
    COMPONENTS Core
               Gui
               Network
               Widgets
    REQUIRED)

# Require Qt >5.15
if (${QT_VERSION_MAJOR} EQUAL 5)
    if (${QT_VERSION_MINOR} LESS 15)
        message(FATAL_ERROR "Qt >5.15 is required to build this project!")
    endif ()
endif ()

if (APPLE)
    find_package(
        Qt${QT_VERSION_MAJOR}
        COMPONENTS MacExtras
        REQUIRED)
endif ()

include(FindOpenSSL)

add_executable(ricochet-irc)

if (DEFINED ENV{RICOCHET_REFRESH_VERSION})
    add_compile_definitions(TEGO_VERSION=$ENV{RICOCHET_REFRESH_VERSION})
endif ()

target_sources(ricochet-irc PRIVATE 
    main.cpp ${RICOCHET_QML_RES} ${RICOCHET_QM_RES}
    IrcChannel.cpp
    IrcConnection.cpp
    IrcServer.cpp
    IrcUser.cpp
    RicochetIrcServer.cpp
    RicochetIrcServerTask.cpp
)
if (STATIC_QT)
    include(qmake_static)
    target_generate_static_qml_plugins(ricochet-irc)
    target_generate_static_qt_plugins(ricochet-irc)
endif ()

target_precompile_headers(ricochet-irc PRIVATE precomp.hpp)

include(lto)
include(compiler_opts)
# enables compiler specific warnings/sanitizers if requested
setup_compiler(ricochet-irc)

target_compile_features(ricochet-irc PRIVATE cxx_std_20)

target_link_libraries(ricochet-irc PUBLIC tego tego_ui)

if (NOT USE_SUBMODULE_FMT)
    find_package(fmt REQUIRED)
endif ()
target_link_libraries(ricochet-irc PRIVATE fmt::fmt-header-only)
target_link_libraries(ricochet-irc PRIVATE OpenSSL::Crypto)

target_link_libraries(
    ricochet-irc
    PRIVATE Qt${QT_VERSION_MAJOR}::Core
            Qt${QT_VERSION_MAJOR}::Widgets
            Qt${QT_VERSION_MAJOR}::Network)
if (APPLE)
    target_link_libraries(ricochet-irc PRIVATE Qt${QT_VERSION_MAJOR}::MacExtras)
endif ()

if ("${CMAKE_BUILD_TYPE}" MATCHES "Rel.*" OR "${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    target_compile_definitions(ricochet-irc PRIVATE QT_NO_DEBUG_OUTPUT QT_NO_WARNING_OUTPUT)
endif ()

# Linux / Cygwin
if (UNIX)
    # Again, not sure if this needs to be UNIX AND NOT WIN32, or if we should
    # install to /bin on Cygwin like it does now
    install(TARGETS ricochet-irc DESTINATION usr/bin)
endif ()

# Move our final binary to a bin dir inside the output dir. This makes it
# easier for integration with ricochet-build
set_target_properties(ricochet-irc
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/ricochet-irc/"
)
