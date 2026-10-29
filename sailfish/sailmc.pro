# SailMC - qmake project for the Sailfish IDE / Sailfish SDK.
#
# Builds the same code as the CMake build (-DLauncher_SAILFISH=ON) into a single binary:
# the MultiMC launcher core, its libraries and the Silica frontend. The file list lives in
# sources.pri, generated from the CMake build by qmake/generate-pri.py.
#
# Open this file in the Sailfish IDE, or build an RPM with: mb2 -t <target> build  (in this folder)

TARGET = sailmc

CONFIG += sailfishapp c++11 object_parallel_to_source

# keep generated files out of the source folder (mb2 builds in place)
OBJECTS_DIR = .obj
MOC_DIR = .moc
RCC_DIR = .rcc
QT += core gui network xml concurrent svg qml quick

# Same settings as the CMake build of the launcher core
DEFINES += \
    LAUNCHER_SAILFISH \
    QT_NO_DEPRECATED_WARNINGS=Y \
    _GLIBCXX_USE_CXX11_ABI=0 \
    QUAZIP_STATIC \
    NBT_STATIC_DEFINE \
    CLASSPARSER_LIBRARY \
    RAINBOW_LIBRARY
QMAKE_CXXFLAGS += -Wno-deprecated-declarations
QMAKE_CFLAGS += -std=gnu11
LIBS += -lz

include(sources.pri)

# Files the CMake build generates (configure_file / generate_export_header)
INCLUDEPATH += $$PWD/qmake
SOURCES += $$PWD/qmake/BuildConfig.cpp
HEADERS += $$PWD/qmake/nbt_export.h

RESOURCES += \
    $$PWD/../launcher/resources/multimc/multimc.qrc \
    $$PWD/../launcher/resources/skins/skins.qrc \
    $$PWD/../branding/logo.qrc

# sailfishapp installs qml/, sailmc.desktop and icons/<size>/sailmc.png from this folder
SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

DISTFILES += \
    qml/sailmc.qml \
    qml/cover/*.qml \
    qml/pages/*.qml \
    qml/components/*.qml \
    sailmc.desktop \
    rpm/sailmc.spec

# Java helpers of the launcher (built by build-jars.sh)
jars.files = $$PWD/jars/NewLaunch.jar $$PWD/jars/JavaCheck.jar
jars.path = /usr/share/$${TARGET}/jars
INSTALLS += jars

# Game natives built by native/build-natives.sh, if present for this architecture
NATIVES_ARCH = $$system(uname -m)
exists($$PWD/native/prebuilt/$${NATIVES_ARCH}) {
    message("Bundling game natives for $${NATIVES_ARCH}")
    natives.files = $$PWD/native/prebuilt/$${NATIVES_ARCH}/*
    natives.path = /usr/share/$${TARGET}/renderers
    INSTALLS += natives
} else {
    message("No game natives for $${NATIVES_ARCH}, run native/build-natives.sh to bundle them")
}
