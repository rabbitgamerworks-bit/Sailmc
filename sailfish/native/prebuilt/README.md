# Prebuilt game natives

Built with `sailfish/native/build-natives.sh` in the Sailfish SDK (targets SailfishOS-5.1.0.11-aarch64
and -i486). `sailmc.pro` / the CMake build install the folder of the target architecture to
`/usr/share/sailmc/renderers`. Run the script again to rebuild them from source.

| Library | Version | Source | License |
|---|---|---|---|
| GL4ES (`gl4es/libGL.so.1`) | master | https://github.com/ptitSeb/gl4es | MIT |
| GLFW (`libglfw*.so`) | 3.3.10 + `glfw-3.3.10-sailfish.patch` | https://github.com/glfw/glfw | zlib |
| LWJGL natives (`liblwjgl*.so`) | 3.2.2, with dyncall 1.1 | https://github.com/LWJGL/lwjgl3, https://www.dyncall.org | BSD-3-Clause, ISC |
| OpenAL Soft (`libopenal*.so`) | 1.23.1 | https://github.com/kcat/openal-soft | LGPL-2.0-or-later |
| libatomic (`libatomic.so.1`) | from the SDK's GCC | https://gcc.gnu.org | GPL-3.0 with the GCC Runtime Library Exception |

The i486 libraries carry the `32` suffix LWJGL uses for 32-bit natives (e.g. `liblwjgl32.so`).
OpenAL Soft is LGPL: its source is at the link above, and it is linked dynamically, so it can be
replaced with another build.
