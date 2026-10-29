#!/bin/bash
# Builds the Java helper jars of the launcher (NewLaunch.jar, JavaCheck.jar) on the host.
# The Sailfish SDK build engine has no JDK, so the RPM build installs the jars from sailfish/jars/.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/sailfish/jars"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$OUT"

# Java 8 bytecode: Minecraft 1.16 and older run on Java 8
JAVAC_FLAGS="--release 8 -Xlint:-options"

build_jar() {
    local name="$1" main="$2" srcdir="$3"
    mkdir -p "$TMP/$name"
    (cd "$srcdir" && javac $JAVAC_FLAGS -d "$TMP/$name" $(find . -name '*.java'))
    jar --create --file "$OUT/$name.jar" --main-class "$main" -C "$TMP/$name" .
    echo "built $OUT/$name.jar"
}

build_jar NewLaunch org.multimc.EntryPoint "$ROOT/libraries/launcher"
build_jar JavaCheck JavaCheck "$ROOT/libraries/javacheck"
