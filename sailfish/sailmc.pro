# SailMC - qmake project for the Sailfish IDE / Sailfish SDK.
#
# Builds the same code as the CMake build (-DLauncher_SAILFISH=ON) into a single binary:
# the MultiMC launcher core, its libraries and the Silica frontend. The file list lives in
# sources.pri, generated from the CMake build by qmake/generate-pri.py.
#
# Open this file in the Sailfish IDE, or build an RPM with: mb2 -t <target> build  (in this folder)

TARGET = sailmc

CONFIG += sailfishapp c++11

# Target architecture, named like the RPM architectures (and native/prebuilt/). QT_ARCH comes from the
# target's Qt, so it is also right when Qt Creator evaluates the project on the host (e.g. Windows,
# where there is no uname)
equals(QT_ARCH, arm64): ARCH = aarch64
else: equals(QT_ARCH, i386): ARCH = i486
else: equals(QT_ARCH, arm): ARCH = armv7hl
else: ARCH = $$QT_ARCH

# Keep generated files out of the source folder, one folder per architecture: mb2 builds in place,
# and switching targets must not reuse objects or the binary of the other architecture
DESTDIR = .build/$${ARCH}
OBJECTS_DIR = .build/$${ARCH}/obj
MOC_DIR = .build/$${ARCH}/moc
RCC_DIR = .build/$${ARCH}/rcc
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
# the MultiMC core triggers these by the hundred with the SDK's -Wall -Wextra; they are harmless
# (unused parameters of virtual functions, implicit copy operators, commented switch fallthroughs)
QMAKE_CXXFLAGS += -Wno-deprecated-declarations -Wno-unused-parameter -Wno-deprecated-copy -Wno-implicit-fallthrough
QMAKE_CFLAGS += -std=gnu11
LIBS += -lz

include(sources.pri)

# Files the CMake build generates (configure_file / generate_export_header)
INCLUDEPATH += $$PWD/qmake
SOURCES += $$PWD/qmake/BuildConfig.cpp

# Object files all go to OBJECTS_DIR by file name. object_parallel_to_source can't be used: with a
# shadow build on Windows (Sailfish IDE) it makes paths like obj/C:/Users/..., so the one clash
# (launcher/Version.cpp and launcher/meta/Version.cpp) is compiled through a wrapper instead
SOURCES -= $$PWD/../launcher/meta/Version.cpp
SOURCES += $$PWD/qmake/meta_Version.cpp
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
exists($$PWD/native/prebuilt/$${ARCH}) {
    message("Bundling game natives for $${ARCH}")
    natives.files = $$PWD/native/prebuilt/$${ARCH}/*
    natives.path = /usr/share/$${TARGET}/renderers
    INSTALLS += natives
} else {
    message("No game natives for $${ARCH}, run native/build-natives.sh to bundle them")
}
