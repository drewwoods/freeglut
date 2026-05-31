# freeglut: OSMesa headless backend + standalone glutshapes (geometry+fonts) library

## Context

freeglut creates its GL **context** through the platform's native window-system path
(GLX/X11, WGL/Windows, NSOpenGL/Cocoa, EGL/GLES). There is no way to get a GL
context with **no window system at all**. Two needs follow from that gap:

1. **Headless rendering (OSMesa).** The motivating downstream consumer (gl-repl)
   needs a legacy/compatibility desktop-GL context that supports
   `glRenderMode(GL_FEEDBACK)` and freeglut's `glutSolid*` shapes, runnable in CI
   with no display server, on Linux/macOS/Windows. OSMesa ("Off-Screen Mesa")
   renders into a CPU buffer you `malloc` with no X11/GLX/Cocoa/EGL. The external
   spec for this lives at
   `~/src/code/openGL/.../plans/external/freeglut-osmesa-backend.md`.

2. **Reusable geometry (emscripten / gl4es).** freeglut's shape and font code
   (`glutSolidTeapot`, `glutWireSphere`, `glutStrokeString`, …) is genuinely useful
   on its own. Environments like **emscripten** already emulate the GLUT windowing
   API (`glutCreateWindow`/`glutMainLoop` via the browser canvas) and **gl4es**
   emulates legacy desktop GL on GLES — they don't need freeglut's window/event
   machinery, they just want the *shapes*. Today the geometry is welded to
   `fgState`/`fgStructure.CurrentWindow`, so it can't be pulled in standalone.

This plan delivers both, staged: **Phase 1** adds an `osmesa` platform backend;
**Phase 2** extracts a standalone **`libglutshapes`** (shapes **and** fonts) that an
emscripten/gl4es consumer can link without freeglut's windowing. Both are strictly
**additive** — no existing backend is touched beyond shared, guarded insertion points.

Verified on this machine: OSMesa **8.0.0** (Mesa 24.2.8) is installed via Homebrew
(`pkg-config --exists osmesa` → yes; `/opt/homebrew/include/GL/osmesa.h`,
`/opt/homebrew/lib/libOSMesa.dylib`).

---

## Phase 1 — OSMesa headless backend (`src/osmesa/`)

### Design decision

OSMesa has **no window system**, so there is nothing for a separate context layer to
pair with. Implement **one new platform** `src/osmesa/` that provides the OSMesa
context/display bits **and** stubs the entire window-manager/input surface — rather
than "null window backend + context provider" (two dirs). Gate with a new CMake
option `FREEGLUT_OSMESA` → `-DTARGET_HOST_OSMESA=1` (OS-agnostic; **not** a
`TARGET_HOST_POSIX_*`). Mutually exclusive with `FREEGLUT_GLES`/`WAYLAND`/`COCOA`.

The two reusable templates:
- **Context provider:** `src/egl/` (`fg_window_egl.c`, `fg_init_egl.c`,
  `fg_state_egl.c`, `fg_structure_egl.c`, `fg_display_egl.c`, `fg_internal_egl.h`).
  OSMesa's API is tinier than EGL's, so this is a direct, mechanical analog.
- **Stub shape:** `src/ogc/` — the leanest existing backend (~1150 lines, all stubs
  are `fgWarning("%s() : not implemented", __func__)` + sane return).

### Context storage — `src/osmesa/fg_internal_osmesa.h` (new)

Mirror `src/egl/fg_internal_egl.h` (which defines the window/context typedefs
directly, since it's its own platform):

```c
#include <GL/osmesa.h>
typedef void*          SFG_WindowHandleType;   /* unused; dummy */
typedef OSMesaContext  SFG_WindowContextType;  /* the GL context handle */
typedef int            SFG_WindowColormapType; /* dummy */
struct tagSFG_PlatformContextOSMesa {          /* -> SFG_PlatformContext.osmesa */
    void   *Buffer;                            /* malloc'd w*h*4 RGBA framebuffer */
    GLsizei Width, Height;
};
struct tagSFG_PlatformDisplayOSMesa { int dummy; };  /* OSMesa has no "display" */
```

In `src/fg_internal.h`, add the additive arm next to the existing platform includes
(`fg_internal.h:194-215`) and to the `SFG_PlatformDisplay`/`SFG_PlatformContext`
unions:

```c
#if TARGET_HOST_OSMESA
#include "osmesa/fg_internal_osmesa.h"
#endif
```

`SFG_Context` (`fg_internal.h:443-462`) already holds `SFG_WindowContextType Context`
and `SFG_PlatformContext pContext` plus `attribute_v_coord/normal/texture` — no change
needed there beyond the typedef resolving to `OSMesaContext`.

### Files to create (mirror egl context bits + ogc stubs)

| Path | Role | Template |
|---|---|---|
| `src/osmesa/fg_internal_osmesa.h` | typedefs + context/display structs | `egl/fg_internal_egl.h` |
| `src/osmesa/fg_init_osmesa.c` | `fgPlatformInitialize` (time base, no display), `fgPlatformCloseDisplay`, `fgPlatformDestroyContext` (`OSMesaDestroyContext`), `fgPlatformDeinitialiseInputDevices` | `egl/fg_init_egl.c` + `x11/fg_init_x11.c` |
| `src/osmesa/fg_window_osmesa.c` | `fghCreateNewContextOSMesa`, `fgPlatformOpenWindow` (create ctx, `malloc(w*h*4)`, `OSMesaMakeCurrent`, synthesize first reshape/visibility), `fgPlatformCloseWindow` (`free` buffer, destroy ctx), `fgPlatformSetWindow` (re-`OSMesaMakeCurrent`), title setters store/ignore | `egl/fg_window_egl.c` |
| `src/osmesa/fg_display_osmesa.c` | `fgPlatformGlutSwapBuffers` (no-op; optional `glFinish`), swap-ctl no-ops, `fgPlatformExtSupported` (parse `GL_EXTENSIONS`) | `egl/fg_display_egl.c` |
| `src/osmesa/fg_state_osmesa.c` | `fgPlatformGlutGet` (answer `GLUT_WINDOW_WIDTH/HEIGHT`, `GLUT_WINDOW_RGBA`, `GLUT_WINDOW_DOUBLEBUFFER`, depth/stencil/accum bits from stored config), `fgPlatformGlutGetModeValues` | `egl/fg_state_egl.c` |
| `src/osmesa/fg_main_osmesa.c` | null loop: `fgPlatformProcessSingleEvent` (no-op), `fgPlatformMainLoopPreliminaryWork`, `fgPlatformSleepForEvents` (brief `nanosleep`), `fgPlatformSystemTime` (`clock_gettime(CLOCK_MONOTONIC)`), `fgPlatformInitWork`, `fgPlatformPosResZordWork`, `fgPlatformVisibilityWork` | `android/fg_main_android.c` |
| `src/osmesa/fg_structure_osmesa.c` | `fgPlatformCreateWindow` (init `pContext.osmesa` defaults) | `egl/fg_structure_egl.c` |
| `src/osmesa/fg_ext_osmesa.c` | `fgPlatformGetProcAddress`→`OSMesaGetProcAddress`, `fgPlatformGetGLUTProcAddress` | `ogc/fg_ext_ogc.c` |
| `src/osmesa/fg_cursor_osmesa.c`, `fg_gamemode_osmesa.c`, `fg_input_devices_osmesa.c`, `fg_joystick_osmesa.c` | stubs (warn + sane return) | `ogc/fg_*_ogc.c` |

(Spaceball/dial live in `fg_input_devices_osmesa.c` as in OGC.)

### Display-mode mapping (`glutInitDisplayMode` → OSMesa)

`fghCreateNewContextOSMesa` reads `fgState.DisplayMode` →
`OSMesaCreateContextExt(OSMESA_RGBA, depthBits, stencilBits, accumBits, share)`:
`GLUT_DEPTH`→24, `GLUT_STENCIL`→8, `GLUT_ACCUM`→16 (best-effort; llvmpipe usually
lacks accum — document, gl-repl passes `--noaccum`), `GLUT_DOUBLE`→single buffer
(swap = no-op), `GLUT_MULTISAMPLE`/`GLUT_INDEX` ignored/forced-RGBA (document).

### The subtle part — first-frame callback synthesis

A real WM delivers reshape/visibility as events; the null backend must **post them
directly** or `display` never fires. The generic flow (verified):
`fgOpenWindow()` sets `window->State.WorkMask |= GLUT_INIT_WORK` (`fg_window.c:~130`)
→ `fgProcessWork()` (`fg_main.c:365`) runs `fgPlatformInitWork`, invokes the
`InitContext` WCB, asserts a Display callback exists, then on `GLUT_DISPLAY_WORK`
calls `fghRedrawWindow` **only if `window->State.Visible`**. So `fgPlatformInitWork`
(model `android/fg_main_android.c:491`) must: post `fghOnPositionNotify(window,0,0,…)`,
post `fghOnReshapeNotify(window,w,h,…)` (fires reshape + sets `GLUT_DISPLAY_WORK`),
set `window->State.Visible = GL_TRUE`, and `INVOKE_WCB(WindowStatus, GLUT_FULLY_RETAINED)`.
`glutPostRedisplay` already sets `GLUT_DISPLAY_WORK` (`fg_display.c:44`), so the
existing timer/redisplay work-list drives subsequent frames; `fgPlatformSleepForEvents`
just needs to not busy-spin.

### CMake wiring (`CMakeLists.txt`)

1. **Option** near `:87`: `OPTION(FREEGLUT_OSMESA "Headless off-screen Mesa backend (no window system)" OFF)`.
2. **Platform-selection branch — placed FIRST** in the chain (the chain currently
   starts `IF(WIN32)` at `:160`; OSMesa is OS-agnostic and must override the native
   platform on any OS, so make it `IF(FREEGLUT_OSMESA) … ELSEIF(WIN32) …`). List the
   `src/osmesa/*.c/.h` sources there.
3. **Exclude from native dep blocks:** add `OR FREEGLUT_OSMESA` to the `NOT(...)`
   guards of the UNIX/X11 dependency block (`:342`) and skip the desktop-GL/GLES
   find/link (`:375-441`) when OSMesa is on.
4. **Find + link OSMesa:** `pkg_check_modules(OSMESA osmesa)` (fallback
   `find_library(OSMESA_LIBRARY OSMesa)`); append to `LIBS`. Confirmed working via
   `pkg-config osmesa` on this Mac.
5. **Define + lib name:** near `:651` (mirror the `FREEGLUT_COCOA`
   `-DTARGET_HOST_MACOS_COCOA=1` block) add `IF(FREEGLUT_OSMESA)
   ADD_DEFINITIONS(-DTARGET_HOST_OSMESA=1)` and set a distinct `LIBNAME`
   (`glut_osmesa`) so a headless build coexists with a normal freeglut. Demos still
   link the CMake target name `freeglut`, so `progs/` build unchanged.

### Phase 1 verification

- **Self-test demo** (new minimal prog, model `progs/demos/timer/timer.c`):
  `glutInit` → `glutInitDisplayMode(GLUT_RGBA|GLUT_DEPTH)` →
  `glutInitWindowSize(64,64)` → `glutCreateWindow` → in `display`, clear to a known
  color + draw a triangle, then `OSMesaGetColorBuffer` and assert the center pixel.
- **Solid-capture test:** `glutSolidTeapot(1)` under `glRenderMode(GL_FEEDBACK)` →
  assert non-zero `GL_POLYGON_TOKEN` count (the property gl-repl depends on).
- Build `cmake -DFREEGLUT_OSMESA=ON -DFREEGLUT_GLES=OFF ..` and run with no
  `DISPLAY`/no Cocoa session. Cross-check on Linux (`apt install libosmesa6-dev`).

---

## Phase 2 — Standalone `libglutshapes` (shapes + fonts)

### Goal

Compile freeglut's geometry **and** font sources into a self-contained library that
provides the `glutSolid*`/`glutWire*` shapes, teapot/teacup/teaspoon, and
stroke/bitmap text — with **no** dependency on freeglut's windowing/event/context
code. An emscripten (or gl4es) consumer that already supplies GLUT windowing + a GL
context links `libglutshapes` for the shapes. (Per decision: this plan makes it
**buildable standalone and documents the emscripten path only** — the actual
emcc/WebGL glue lives downstream, the same way gl-repl is downstream of Phase 1.)

### Coupling to remove (verified, and it is small)

The sources `#include "fg_internal.h"` and reach into exactly:
- `fgState.HasOpenGL20`, `fgState.Initialised`, `fgState.MajorVersion`,
  `fgState.StrokeFontDrawJoinDots`
- `fgStructure.CurrentWindow` → `Window.attribute_v_{coord,normal,texture}`,
  `State.VisualizeNormals` (only in `fghDrawGeometryWire/Solid`, `fg_geometry.c:135,183`
  and the `glutSetVertexAttrib*` setters, `fg_gl2.c:41-54`)
- `FREEGLUT_EXIT_IF_NOT_INITIALISED` (every public entry point)
- `fgError` / `fgWarning`
- `glutGetProcAddress` (only `fg_gl2.c` `LOADFUNC`, and only on the non-GLES2 path —
  on `GL_ES_VERSION_2_0` it is `#define`d away)

### Decoupling approach — compat shim header with **symbol-renaming `#define`s**

Per decision: use a compatibility shim (no rewrite of the geometry algorithms), **and
`#define`-rename every shimmed symbol** to a private `fgshapes_*` name. Renaming is the
key correctness move: emscripten and gl4es **implement the real GLUT/GL windowing
symbols themselves** (notably `glutGetProcAddress`), so if the standalone TUs referenced
the un-prefixed names they would either collide at link time or silently bind to the
host's implementation. Renaming makes the library reference *only its own* support
symbols and never the host's.

New `src/standalone/fg_glutshapes_shim.h` (or `src/fg_internal_standalone.h`), supplying
under `-DFREEGLUT_GEOMETRY_STANDALONE`:

```c
#include <GL/freeglut.h>                 /* GLUT API decls + GL types only */

/* rename shimmed support symbols so we never bind to a host GLUT/GL-emu */
#define fgState             fgshapes_State
#define fgStructure         fgshapes_Structure
#define fgError             fgshapes_Error
#define fgWarning           fgshapes_Warning
#define glutGetProcAddress  fgshapes_GetProcAddress
#define FREEGLUT_EXIT_IF_NOT_INITIALISED(s)  ((void)0)

/* minimal stand-ins for the structs the geometry/font code dereferences */
typedef struct { GLint attribute_v_coord, attribute_v_normal, attribute_v_texture; } SFG_Context;
typedef struct { GLboolean VisualizeNormals, Visible; } SFG_WindowState;
typedef struct { SFG_Context Window; SFG_WindowState State; } SFG_Window;
typedef struct { GLboolean Initialised; int HasOpenGL20, MajorVersion; int StrokeFontDrawJoinDots; } SFG_State;
typedef struct { SFG_Window *CurrentWindow; } SFG_Structure;
extern SFG_State     fgshapes_State;
extern SFG_Structure fgshapes_Structure;          /* .CurrentWindow → a single static dummy window */
void  fgshapes_Error(const char *fmt, ...);        /* fprintf(stderr); no exit */
void  fgshapes_Warning(const char *fmt, ...);
void *fgshapes_GetProcAddress(const char *name);   /* weak default; consumer-overridable */
```

Plus one tiny new TU `src/standalone/fg_glutshapes_shim.c` defining
`fgshapes_State` (default `{Initialised=1, HasOpenGL20=1, MajorVersion=2}` so the GL2
attrib path is live out-of-the-box), the static dummy window wired into
`fgshapes_Structure.CurrentWindow` (so `glutSetVertexAttrib*` work), and the
`fgshapes_Error/Warning/GetProcAddress` bodies, and a small public init helper
(e.g. `glutShapesSetGLVersion(int major)`).

### The only edit to shared geometry/font code

At the top of each of `fg_geometry.c`, `fg_teapot.c`, `fg_gl2.c`, `fg_gl2.h`,
`fg_font.c`, `fg_font_data.c`, `fg_stroke_roman.c`, `fg_stroke_mono_roman.c`, replace
the lone `#include "fg_internal.h"` with the guarded form:

```c
#ifdef FREEGLUT_GEOMETRY_STANDALONE
#  include "fg_glutshapes_shim.h"
#else
#  include "fg_internal.h"
#endif
```

That is the entire change to existing source — additive and invisible to every normal
build. (Public entry points `glutSolidTeapot`, `glutStrokeString`,
`glutSetVertexAttrib*`, `fgInitGL2` keep their names — that's the exported product;
emscripten does not define the shapes. Provide an optional `-DFGSHAPES_PREFIX_PUBLIC`
escape hatch in the shim that renames the public entry points too, for a host that
*does* stub them.)

### CMake target

Add `OPTION(FREEGLUT_BUILD_GLUTSHAPES "Build standalone geometry+font library" OFF)`
and, when set, an independent `add_library(glutshapes …)` target that compiles **only**
`fg_geometry.c fg_teapot.c fg_gl2.c fg_font.c fg_font_data.c fg_stroke_roman.c
fg_stroke_mono_roman.c src/standalone/fg_glutshapes_shim.c` with
`-DFREEGLUT_GEOMETRY_STANDALONE`, includes `include/` for `<GL/freeglut.h>`, and links
**no** window-system libs. This target is orthogonal to the platform-selection chain
(it never pulls a backend), so it builds on any host.

### GLES2 / fixed-function caveat (document, don't block)

The GL1.1 geometry path (`fghDrawGeometrySolid11`, client vertex arrays) and the fonts
(`glBitmap`, immediate-mode `glBegin/glVertex2f` in `fg_font.c`) are **legacy
fixed-function** — they work on desktop GL, on the Phase-1 OSMesa context, and on
**gl4es** (which emulates exactly these on GLES), but **not** on raw WebGL/GLES2. On a
pure GLES2/WebGL target only the GL2.0 vertex-attrib geometry path is valid, and the
consumer must bind a shader + call `glutSetVertexAttribCoord3/Normal/TexCoord2`. This is
why gl4es is the natural emscripten companion. State this in the README; it does not
limit the OSMesa use case at all.

---

## Critical files

- **New (Phase 1):** `src/osmesa/*` (10 files above).
- **New (Phase 2):** `src/standalone/fg_glutshapes_shim.h`, `src/standalone/fg_glutshapes_shim.c`.
- **Edited, additive:**
  - `src/fg_internal.h` — `#if TARGET_HOST_OSMESA` include + union arms (`:194-215`, `:443-462`).
  - `CMakeLists.txt` — `FREEGLUT_OSMESA` option/branch/find/define/libname; `FREEGLUT_BUILD_GLUTSHAPES` option + `glutshapes` target.
  - `fg_geometry.c`, `fg_teapot.c`, `fg_gl2.c`, `fg_gl2.h`, `fg_font.c`, `fg_font_data.c`, `fg_stroke_roman.c`, `fg_stroke_mono_roman.c` — the one guarded-include swap each.
  - `README` — both new options + deps + the GLES2/gl4es caveat.
- **Reused templates (do not modify):** `src/egl/*` (context provider), `src/ogc/*` (stubs), `src/android/fg_main_android.c` (event-poor loop), `progs/demos/timer/` (self-test shape).

## Verification

- **Phase 1:** the two demos above (triangle pixel-readback; teapot `GL_FEEDBACK`
  token count) built with `-DFREEGLUT_OSMESA=ON`, run headless on macOS and Linux.
- **Phase 2:** build the `glutshapes` target standalone (`-DFREEGLUT_BUILD_GLUTSHAPES=ON`);
  confirm it links with **no** X11/EGL/Cocoa/OSMesa libs and exports the `glut*` shape/
  font symbols (`nm` check) while exporting its support symbols only under the
  `fgshapes_*` names. A small desktop test program (link `glutshapes` + a real GL
  context, e.g. the Phase-1 OSMesa context) draws `glutSolidSphere` and
  `glutStrokeString` and reads back a non-blank framebuffer — proving the geometry runs
  with zero freeglut windowing linked.

## Out of scope / risks

- **Menus & overlays** under OSMesa: freeglut menus are sub-windows — leave
  unsupported (assert/no-op).
- **Accum/MSAA** in swrast: best-effort/ignored, documented.
- **`GL_FEEDBACK` on Mesa:** legacy GL; works on standard distro/Homebrew Mesa — pin
  the tested Mesa version in CI.
- **emscripten build itself:** out of scope by request — only documented as the
  downstream consumer of `libglutshapes` (+ gl4es for the legacy-GL paths).
- **Upstreaming:** keep every edit strictly additive/guarded so the change is a clean
  candidate for a freeglut PR (new `FREEGLUT_OSMESA` + `FREEGLUT_BUILD_GLUTSHAPES`
  options in its CI matrix).
