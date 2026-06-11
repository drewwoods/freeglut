# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

A fork of freeglut (the Free OpenGL Utility Toolkit, a GLUT-compatible C library) focused on developing the **macOS Cocoa backend** (`src/cocoa/`), which is experimental upstream. The X11 build is kept working alongside it as the reference implementation for comparing behavior.

## Build

Two preconfigured CMake build trees exist at the repo root:

- `build/` — Cocoa backend (`FREEGLUT_COCOA=ON`). This is the primary development target.
- `build-x11/` — X11/GLX backend via XQuartz, using homebrew mesa (`OPENGL_gl_LIBRARY=/opt/homebrew/lib/libGL.dylib`). Used to compare against reference X11 behavior on macOS.
- `build-mingw/` — Windows (mswin backend) cross-compiled with homebrew `mingw-w64`, used for compile checking only (toolchain file: `CMAKE_SYSTEM_NAME=Windows`, `CMAKE_C_COMPILER=x86_64-w64-mingw32-gcc`, `CMAKE_RC_COMPILER=x86_64-w64-mingw32-windres`). Produces `bin/*.exe`; run `bin/dstrprobe.exe` under wine or on a real Windows box for runtime testing.

```sh
cmake --build build -j            # build everything (lib + demos)
cmake --build build --target dstrprobe   # build a single demo
```

To configure from scratch (ARM mac paths):

```sh
FW=$(xcrun --show-sdk-path)/System/Library/Frameworks/OpenGL.framework
cmake -B build -DFREEGLUT_COCOA=ON -DOPENGL_gl_LIBRARY=$FW -DOPENGL_glu_LIBRARY=$FW .
cmake -B build-x11 -DOPENGL_gl_LIBRARY=/opt/homebrew/lib/libGL.dylib .
```

The Cocoa build MUST link Apple's OpenGL.framework, not homebrew mesa. If
CMake's FindOpenGL picks up `/opt/homebrew/lib/libGL.dylib` (it does when mesa
is installed), every `glGetIntegerv`/`glGetString` in the library and demos
binds to mesa's GLX dispatcher, which has no notion of the current
NSOpenGL/CGL context and silently returns 0/NULL. Check with
`otool -L build/lib/libglut.3.dylib | grep -i mesa` — there must be no hit.

Both shared and static libs are built; each demo is built twice (`foo` linked shared, `foo_static` linked static), output in `<builddir>/bin/`. Demos run in-tree without `make install`.

Two Linux X11 boxes are reachable over ssh for testing the X11 backend on real drivers (`DISPLAY=:0`, build in-tree with a fresh `cmake -B <dir>` — don't reuse a build dir synced from the mac):

- `gracemont.local` — Intel ADL-N on Mesa. Mesa marks accum-capable fbconfigs with the slow caveat (`glxinfo -l`), so e.g. `"acca slow=0"` is impossible here.
- `zen3.local` — NVIDIA RTX (proprietary driver). Exposes accum 16×4 and 4 aux buffers on every fbconfig, nothing slow-marked.

## Testing

There is no unit test suite; the programs under `progs/demos/` serve as manual tests. The exception is **`dstrprobe`**, which has automated self-test modes covering `glutInitDisplayString()` semantics:

```sh
./build/bin/dstrprobe --self-test            # full output
./build/bin/dstrprobe --self-test-summary    # pass/fail summary
./build/bin/dstrprobe "rgb double depth>=16" # probe one display string
```

Run the same binary from `build-x11/bin/` to check the X11 path (needs XQuartz / a `DISPLAY`). `dstrprobe` can also be built against Apple's GLUT framework (`USE_GLUT` define) to compare against Apple GLUT behavior.

## Architecture

freeglut uses compile-time platform selection, not runtime dispatch:

- **Common code** lives in `src/fg_*.c` and calls `fgPlatformXxx()` functions (e.g. `fgPlatformOpenWindow`, `fgPlatformGetProcAddress`). Each backend directory — `src/x11/`, `src/mswin/`, `src/cocoa/`, `src/wayland/`, `src/android/`, `src/blackberry/`, `src/egl/`, `src/ogc/` — provides its own implementations; CMake compiles exactly one backend into the library.
- **`src/fg_internal.h`** is the central private header. It defines the `TARGET_HOST_*` macros (e.g. `TARGET_HOST_MACOS_COCOA`, `TARGET_HOST_POSIX_X11`) and includes the matching `fg_internal_<platform>.h`, which supplies platform typedefs (`SFG_WindowHandleType`, `SFG_WindowContextType`) and `SFG_Platform*` structs that are embedded inside the common `SFG_Display`/`SFG_Window`/`SFG_Context`/`SFG_WindowState` structs.
- **Global singletons**: `fgState` (init-time settings, including `DisplayMode` and the display-string criteria), `fgDisplay` (screen info + embedded platform display), `fgStructure` (window/menu hierarchy).
- The Cocoa backend is Objective-C (`.m` files); the X11 backend splits windowing (`fg_*_x11.c`) from GLX-specific context/pixel-format code (`fg_*_x11_glx.c`). The EGL code in `src/egl/` is shared by the Android, BlackBerry, and Wayland backends.

### Display-string matching (current focus)

`src/fg_display_string.c` implements the shared `glutInitDisplayString()` criteria model: parsing into `FGCriterion` (comparator + value) per `FGCapability`, hard filtering, and left-to-right lexicographic ranking of candidate pixel formats. Backends supply a per-capability value array for each candidate format and reuse this logic so matching semantics stay identical across platforms. The Cocoa side of this is `src/cocoa/fg_pixel_format_cocoa.{h,m}`; `dstrprobe --self-test` is the regression test for this whole area.

## Style

`.clang-format` configs exist at the repo root and in `src/cocoa/` (4-space indent, aligned consecutive assignments/declarations). The legacy codebase style puts spaces inside parens: `if ( condition )`, `foo( arg1, arg2 )` — match it when editing existing files.

## GLUT compatibility constraint

freeglut aims for source and binary compatibility with original GLUT and a stable API/ABI. New features must not change existing exported behavior; comparing against Apple's GLUT and the X11 backend is the standard way to validate Cocoa behavior.
