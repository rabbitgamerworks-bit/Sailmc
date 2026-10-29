#!/bin/bash
# Builds the native libraries Minecraft needs on Sailfish OS, inside the Sailfish SDK build engine:
#
#   libGL.so.1           GL4ES - OpenGL 2.1 on top of the device's OpenGL ES 2
#   libglfw.so           GLFW 3.3 for Wayland, patched for GLES contexts and touch input
#   liblwjgl*.so         LWJGL 3.2.2 natives (Minecraft 1.13 - 1.16), "32" suffix on 32-bit targets
#   libopenal.so         OpenAL Soft with the PulseAudio backend
#
# usage: build-natives.sh <sb2 target> [output folder]
#   e.g. build-natives.sh SailfishOS-5.1.0.11-aarch64
# The result is also copied to sailfish/native/prebuilt/<arch>/, from where the RPM build installs it
# to /usr/share/sailmc/renderers/ (gl4es/ for libGL, common/ for the rest).
set -e

TARGET="$1"
[ -n "$TARGET" ] || { echo "usage: $0 <sb2 target> [output folder]"; exit 1; }
HERE="$(cd "$(dirname "$0")" && pwd)"
WORK="${WORK:-$HOME/sailmc-natives}"
OUT="${2:-$WORK/out-$TARGET}"
JOBS="${JOBS:-$(nproc)}"
SB2="sb2 -t $TARGET"

GL4ES_REV="${GL4ES_REV:-master}"
GLFW_TAG=3.3.10
LWJGL_TAG=3.2.2
DYNCALL_VERSION=1.1          # the dyncall API LWJGL 3.2.x was written against
OPENAL_TAG=1.23.1

mkdir -p "$WORK/src" "$OUT/gl4es" "$OUT/common"
cd "$WORK/src"

fetch() {  # fetch <dir> <git url> <ref>
    [ -d "$1" ] || git clone -q --depth 1 -b "$3" "$2" "$1"
}
fetch gl4es https://github.com/ptitSeb/gl4es.git "$GL4ES_REV"
fetch glfw https://github.com/glfw/glfw.git "$GLFW_TAG"
fetch lwjgl3 https://github.com/LWJGL/lwjgl3.git "$LWJGL_TAG"
fetch openal-soft https://github.com/kcat/openal-soft.git "$OPENAL_TAG"
if [ ! -d dyncall-$DYNCALL_VERSION ]; then
    curl -sSfL https://www.dyncall.org/r$DYNCALL_VERSION/dyncall-$DYNCALL_VERSION.tar.gz | tar xz
fi
if ! grep -q GLFW_FORCE_GLES glfw/src/wl_window.c; then
    patch -d glfw -p1 < "$HERE/glfw-$GLFW_TAG-sailfish.patch"
fi

ARCH="$($SB2 uname -m)"
case "$ARCH" in
    x86_64|aarch64) SUFFIX="" ;;
    *) SUFFIX="32" ;;
esac
echo "== target $TARGET ($ARCH), LWJGL library suffix '$SUFFIX'"

done_already() {  # skip parts that were built before, FORCE=1 rebuilds everything
    [ -z "$FORCE" ] && [ -e "$1" ] && echo "   (already built: $1)"
}

cmake_build() {  # cmake_build <src> <build> <cmake args...>
    local src="$1" build="$2"; shift 2
    mkdir -p "$build"
    (cd "$build" && $SB2 cmake "$src" -DCMAKE_BUILD_TYPE=Release "$@" >/dev/null && $SB2 make -j"$JOBS" >/dev/null)
}

# ---- GL4ES: no X11, the context is created by GLFW through EGL
echo "== gl4es"
done_already "$OUT/gl4es/libGL.so.1" || cmake_build "$WORK/src/gl4es" "$WORK/build-$TARGET/gl4es" -DNOX11=ON -DGLX_STUBS=ON -DEGL_WRAPPER=OFF
[ -e "$OUT/gl4es/libGL.so.1" ] || cp "$WORK/src/gl4es/lib/libGL.so.1" "$OUT/gl4es/"

# ---- GLFW for Wayland
echo "== glfw"
done_already "$OUT/common/libglfw$SUFFIX.so" || cmake_build "$WORK/src/glfw" "$WORK/build-$TARGET/glfw" -DGLFW_USE_WAYLAND=ON -DBUILD_SHARED_LIBS=ON \
    -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF
cp -L "$WORK/build-$TARGET/glfw/src/libglfw.so.3" "$OUT/common/libglfw$SUFFIX.so"

# ---- OpenAL Soft
echo "== openal-soft"
done_already "$OUT/common/libopenal$SUFFIX.so" || cmake_build "$WORK/src/openal-soft" "$WORK/build-$TARGET/openal" -DALSOFT_UTILS=OFF -DALSOFT_EXAMPLES=OFF \
    -DALSOFT_TESTS=OFF -DALSOFT_BACKEND_PULSEAUDIO=ON -DALSOFT_BACKEND_ALSA=ON -DALSOFT_BACKEND_SNDIO=OFF \
    -DALSOFT_BACKEND_JACK=OFF -DALSOFT_BACKEND_PIPEWIRE=OFF -DALSOFT_REQUIRE_PULSEAUDIO=ON
cp -L "$WORK/build-$TARGET/openal/libopenal.so.1" "$OUT/common/libopenal$SUFFIX.so"
# 32-bit targets need libatomic for OpenAL; it is part of the compiler runtime and not on the device
if $SB2 readelf -d "$OUT/common/libopenal$SUFFIX.so" | grep -q libatomic; then
    cp -L "$($SB2 gcc -print-file-name=libatomic.so.1)" "$OUT/common/"
fi

# ---- dyncall (static, linked into liblwjgl)
echo "== dyncall"
DC="$WORK/build-$TARGET/dyncall"
done_already "$DC/dyncall/libdyncall_s.a" || (rm -rf "$DC" && cp -r "$WORK/src/dyncall-$DYNCALL_VERSION" "$DC" && cd "$DC" && $SB2 bash -c "CFLAGS='-O2 -fPIC' ASFLAGS='-fPIC' ./configure >/dev/null && make -j$JOBS >/dev/null")

# ---- LWJGL natives, like config/linux/build.xml of LWJGL
echo "== lwjgl"
JNI="$WORK/jni"
mkdir -p "$JNI/linux"
JDK_INC="${JDK_INCLUDE:-$(dirname "$(readlink -f "$(command -v javac 2>/dev/null || echo /nonexistent)")")/../include}"
if [ -f "$JDK_INC/jni.h" ]; then
    cp "$JDK_INC/jni.h" "$JNI/" && cp "$JDK_INC/linux/jni_md.h" "$JNI/linux/"
fi
[ -f "$JNI/jni.h" ] || { echo "jni.h missing: copy jni.h and linux/jni_md.h of any JDK to $JNI"; exit 1; }

# LinuxLWJGL.h includes X11 headers that nothing in the built sources uses; Sailfish OS has no X11
mkdir -p "$WORK/x11-stubs/X11"
touch "$WORK/x11-stubs/X11/X.h" "$WORK/x11-stubs/X11/Xlib.h"

L="$WORK/src/lwjgl3/modules/lwjgl"
LB="$WORK/build-$TARGET/lwjgl"
mkdir -p "$LB"
case "$ARCH" in
    i?86) ARCHFLAGS="-msse -msse2 -mfpmath=sse -DLWJGL_x86" ;;
    x86_64) ARCHFLAGS="-DLWJGL_x64" ;;
    aarch64) ARCHFLAGS="-DLWJGL_arm64" ;;
    *) ARCHFLAGS="-DLWJGL_arm32" ;;
esac
CFLAGS="-std=c11 -O2 -fPIC -pthread -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -D_GNU_SOURCE -DNDEBUG -DLWJGL_LINUX $ARCHFLAGS \
    -I$JNI -I$JNI/linux -I$L/core/src/main/c -I$L/core/src/main/c/linux -I$WORK/x11-stubs"
LDFLAGS="-shared -z noexecstack -pthread -Wl,--no-undefined -Wl,--version-script,$WORK/src/lwjgl3/config/linux/version.script"

lwjgl_lib() {  # lwjgl_lib <output name> <compiler flags> <static libs> -- <sources...>
    local name="$1" flags="$2" libs="$3"; shift 4
    $SB2 bash -c "gcc $CFLAGS $flags $* $libs $LDFLAGS -o $LB/$name -lm -ldl"
    cp "$LB/$name" "$OUT/common/$name"
    echo "   $name"
}
lwjgl_lib "liblwjgl$SUFFIX.so" "-I$L/core/src/main/c/dyncall" \
    "$DC/dyncall/libdyncall_s.a $DC/dyncallback/libdyncallback_s.a $DC/dynload/libdynload_s.a" -- \
    $L/core/src/main/c/*.c $L/core/src/generated/c/*.c $L/core/src/generated/c/linux/*.c
lwjgl_lib "liblwjgl_opengl$SUFFIX.so" "-I$L/opengl/src/main/c" "" -- $L/opengl/src/generated/c/*.c
lwjgl_lib "liblwjgl_stb$SUFFIX.so" "-isystem $L/stb/src/main/c" "" -- $L/stb/src/generated/c/*.c
lwjgl_lib "liblwjgl_tinyfd$SUFFIX.so" "-I$L/tinyfd/src/main/c" "" -- $L/tinyfd/src/main/c/*.c $L/tinyfd/src/generated/c/*.c

# picked up by the RPM build (sailfish/CMakeLists.txt) and installed to /usr/share/sailmc/renderers
PREBUILT="$HERE/prebuilt/$ARCH"
mkdir -p "$PREBUILT"
cp -r "$OUT"/gl4es "$OUT"/common "$PREBUILT/"

echo "== done: $OUT (copied to $PREBUILT)"
ls -la "$OUT"/*
