# freeglut: OSMesa headless backend + standalone glutshapes (geometry+fonts) library

## Context

freeglut creates its GL **context** through the platform's native window-system path
(GLX/X11, WGL/Windows, NSOpenGL/Cocoa, EGL/GLES). There is no way to get a GL context
with **no window system at all**. Two needs follow from that gap:

1. **Headless rendering (OSMesa).** The motivating downstream consumer (gl-repl) needs
   a legacy/compatibility desktop-GL context that supports `glRenderMode(GL_FEEDBACK)`
   and freeglut's `glutSolid*` shapes, runnable in CI with no display server, on
   Linux/macOS/Windows. OSMesa ("Off-Screen Mesa") renders into a CPU buffer you
   `malloc`, with no X11/GLX/Cocoa/EGL. (Spec that motivated this: the
   `freeglut-osmesa-backend.md` external plan in the gl-repl tree.)
2. **Reusable geometry (emscripten / gl4es).** freeglut's shape and font code
   (`glutSolidTeapot`, `glutWireSphere`, `glutStrokeString`, …) is useful on its own.
   **emscripten** already emulates the GLUT windowing API (`glutCreateWindow`/
   `glutMainLoop` via the canvas) and **gl4es** emulates legacy desktop GL on GLES —
   they don't need freeglut's window/event machinery, they just want the *shapes*.
   Today the geometry is welded to `fgState`/`fgStructure.CurrentWindow`, so it can't be
   pulled in standalone.

This plan delivers both, staged: **Phase 1** adds an `osmesa` platform backend;
**Phase 2** extracts a standalone **`libglutshapes`** (shapes **and** fonts). Both are
strictly **additive** — no existing backend is altered beyond shared, guarded insertion
points.

Verified on this machine: OSMesa **8.0.0** (Mesa 24.2.8) via Homebrew (`pkg-config
--exists osmesa` → yes; `/opt/homebrew/include/GL/osmesa.h`, `…/lib/libOSMesa.dylib`).

> **Note on templates (important correction):** OSMesa is its own **standalone
> platform**, so it is modeled on **`src/ogc/`** (a real standalone backend) — *not*
> `src/egl/`, which is a sub-component embedded inside x11/wayland/android and therefore
> under-specifies the platform contract and defines no `SFG_PlatformContext`/
> `SFG_PlatformDisplay` types. The egl `.c` files remain a useful analog only for the
> shape of the context calls (create / make-current / destroy).

---

## Phase 1 — OSMesa headless backend (`src/osmesa/`)

### Design decision

OSMesa has **no window system**, so there is nothing for a separate context layer to
pair with. Implement **one new platform** `src/osmesa/` that provides the OSMesa
context/display bits **and** stubs the entire window-manager/input surface. Gate with a
new CMake option `FREEGLUT_OSMESA` → `-DTARGET_HOST_OSMESA=1` (OS-agnostic; **not** a
`TARGET_HOST_POSIX_*`). Mutually exclusive with `FREEGLUT_GLES`/`WAYLAND`/`COCOA`.

### Context storage — `src/osmesa/fg_internal_osmesa.h` (new), templated on **ogc**

`SFG_PlatformDisplay` and `SFG_PlatformContext` are **per-platform typedef'd structs**
(not unions): `fg_internal.h:419` holds `SFG_PlatformDisplay pDisplay;` and `:451`
`SFG_PlatformContext pContext;`, and each backend's header defines those types directly
(`ogc/fg_internal_ogc.h:33-68`). So the osmesa header defines them directly and there is
**no union arm to add** anywhere:

```c
#include <GL/osmesa.h>
typedef struct tagSFG_PlatformDisplay SFG_PlatformDisplay;
struct tagSFG_PlatformDisplay { int dummy; };          /* OSMesa has no "display" */
typedef struct tagSFG_PlatformContext SFG_PlatformContext;
struct tagSFG_PlatformContext { void *Buffer; GLsizei Width, Height; };  /* the fb */
typedef struct tagSFG_PlatformWindowState SFG_PlatformWindowState;
struct tagSFG_PlatformWindowState { int dummy; };
typedef struct tagSFG_PlatformJoystick SFG_PlatformJoystick;
struct tagSFG_PlatformJoystick { int dummy; };
typedef void*         SFG_WindowHandleType;            /* unused */
typedef OSMesaContext SFG_WindowContextType;           /* the GL context handle */
typedef int           SFG_WindowColormapType;          /* dummy */
/* + FREEGLUT_MENU_FONT/PEN_* macros and _JS_MAX_AXES / MAX_NUM_JOYSTICKS, as ogc does */
```

The GL context handle lives in `window->Window.Context` (`OSMesaContext`); the
framebuffer + size live in `window->Window.pContext` (accessed directly as
`pContext.Buffer`, `pContext.Width/Height` — **no `.osmesa` member**).

The **only** edit to `src/fg_internal.h` is adding, in the platform-include block
(`:194-215`):

```c
#if TARGET_HOST_OSMESA
#include "osmesa/fg_internal_osmesa.h"
#endif
```

### Files to create — model the **whole** stub/main surface on `src/ogc/`

| Path | Required entry points (model: ogc unless noted) |
|---|---|
| `src/osmesa/fg_internal_osmesa.h` | types above |
| `src/osmesa/fg_init_osmesa.c` | `fgPlatformInitialize` (init time base, set `fgDisplay.ScreenWidth/Height` to a default virtual size, **no display open**), `fgPlatformCloseDisplay`, `fgPlatformDestroyContext`→`OSMesaDestroyContext(MContext)`, `fgPlatformDeinitialiseInputDevices` |
| `src/osmesa/fg_window_osmesa.c` | `fghCreateNewContextOSMesa` (map `fgState.DisplayMode`→`OSMesaCreateContextExt`), `fgPlatformOpenWindow` (create ctx, `malloc(w*h*4)`, `OSMesaMakeCurrent`, **`glDrawBuffer/glReadBuffer(GL_FRONT)`**, set `window->State.Visible=GL_TRUE`), `fgPlatformCloseWindow` (free buffer, destroy ctx), `fgPlatformSetWindow` (re-`OSMesaMakeCurrent`), title setters (store/ignore) |
| `src/osmesa/fg_display_osmesa.c` | `fgPlatformGlutSwapBuffers` (no-op; optional `glFinish`), `fgPlatformInitSwapCtl`/`fgPlatformSwapInterval` (no-ops), `fgPlatformExtSupported` (parse `GL_EXTENSIONS`) |
| `src/osmesa/fg_state_osmesa.c` | `fgPlatformGlutGet` (answer `GLUT_WINDOW_WIDTH/HEIGHT`, `GLUT_WINDOW_RGBA`, `GLUT_WINDOW_DOUBLEBUFFER`=0, depth/stencil/accum bits from the stored config), `fgPlatformGlutGetModeValues`, **`fgPlatformGlutDeviceGet`** (called unconditionally at `fg_state.c:264` — ogc stubs it) |
| `src/osmesa/fg_main_osmesa.c` | `fgPlatformProcessSingleEvent` (no-op), `fgPlatformMainLoopPreliminaryWork` (no-op), `fgPlatformSleepForEvents` (`nanosleep` ~1 ms, capped by the passed `msec`), `fgPlatformSystemTime` (`clock_gettime(CLOCK_MONOTONIC)`; macOS 10.12+), `fgPlatformInitWork`, `fgPlatformPosResZordWork` (warn-only), `fgPlatformVisibilityWork` (`INVOKE_WCB(WindowStatus, GLUT_FULLY_RETAINED)`), **`fgPlatformSetColor`/`fgPlatformGetColor`/`fgPlatformCopyColormap`** (called unconditionally at `fg_misc.c:169-181`; ogc stubs them in `fg_main_ogc.c:274-288`) |
| `src/osmesa/fg_structure_osmesa.c` | `fgPlatformCreateWindow` (zero `pContext`) |
| `src/osmesa/fg_ext_osmesa.c` | `fgPlatformGetProcAddress`→`OSMesaGetProcAddress`, `fgPlatformGetGLUTProcAddress` |
| `src/osmesa/fg_cursor_osmesa.c`, `fg_gamemode_osmesa.c`, `fg_input_devices_osmesa.c`, `fg_joystick_osmesa.c` | stubs: `fgWarning("%s() : not implemented", __func__)` + sane return |

Cross-checked against the core callers, the full unconditional contract is covered here;
do **not** derive completeness from egl (it omits device-get and the colormap trio).

### Display-mode mapping & context contract

`fghCreateNewContextOSMesa` reads `fgState.DisplayMode` →
`OSMesaCreateContextExt(OSMESA_RGBA, depthBits, stencilBits, accumBits, share)`:
`GLUT_DEPTH`→24, `GLUT_STENCIL`→8, `GLUT_ACCUM`→16 (best-effort; llvmpipe usually lacks
accum — document, gl-repl passes `--noaccum`), `GLUT_MULTISAMPLE`/`GLUT_INDEX`
ignored/forced-RGBA. **Profile is intentionally forced to compatibility** —
`OSMesaCreateContextExt` yields a legacy context (which is exactly what makes
`glRenderMode(GL_FEEDBACK)` + fixed-function solids work); a consumer's
`glutInitContextProfile(GLUT_CORE_PROFILE)` is silently ignored. Document this as the
contract, not a surprise.

**GLUT_DOUBLE / draw buffer (must handle explicitly).** OSMesa has one physical buffer.
After `OSMesaMakeCurrent`, `fgPlatformOpenWindow` calls `glDrawBuffer(GL_FRONT);
glReadBuffer(GL_FRONT)` **unconditionally**, and the backend treats the window as
single-buffered (`glutGet(GLUT_WINDOW_DOUBLEBUFFER)`→0, swap = no-op). This is required
because the generic code (`fg_window.c:108-116`, active since OSMesa doesn't define
`EGL_VERSION_1_0`) only redirects to `GL_FRONT` when it thinks the window is
single-buffered; a consumer requesting `GLUT_DOUBLE` would otherwise leave the draw
buffer at the default and readback could be empty. Document that `GLUT_DOUBLE` is
downgraded to single-buffer.

### First-frame callback synthesis (the subtle part) — model: **ogc**, not android

A real WM delivers reshape/visibility as events; the null backend must post them or
`display` never fires. Verified generic flow: `fgOpenWindow()` sets
`window->State.WorkMask |= GLUT_INIT_WORK` (`fg_window.c:130`) → `fgProcessWork()`
(`fg_main.c:365`) runs `fgPlatformInitWork`, invokes `InitContext`, asserts a Display
callback exists, then on `GLUT_DISPLAY_WORK` calls `fghRedrawWindow` **only if
`window->State.Visible`** (`fg_main.c:407`).

Idiom (resolves the earlier doc contradiction): **`State.Visible = GL_TRUE` is set in
`fgPlatformOpenWindow`** (as every backend does, e.g. `ogc/fg_window_ogc.c`,
`x11/fg_window_x11.c:433`), and **`fgPlatformInitWork` synthesizes
position+reshape+status** (model `ogc/fg_main_ogc.c:257-262`, which calls
`fghOnReshapeNotify`). Compose the sequence directly (Android's InitWork at
`fg_main_android.c:491` is **not** a template — it omits reshape, deferring it to its
event poll):

```c
void fgPlatformInitWork(SFG_Window *window) {
    fghOnPositionNotify(window, 0, 0, GL_TRUE);
    fghOnReshapeNotify(window, window->State.Width, window->State.Height, GL_TRUE); /* GL_TRUE: force */
    INVOKE_WCB(*window, WindowStatus, (GLUT_FULLY_RETAINED));
}
```

`fghOnReshapeNotify` fires Reshape and sets `GLUT_DISPLAY_WORK` (`fg_main.c:71`), but
only when dimensions differ from `State.Width/Height` (`fg_main.c:60`) — so pass
`forceNotify = GL_TRUE` to guarantee the first frame. Use the **window's** size (not
`fgDisplay.ScreenWidth/Height` as ogc does, since OSMesa windows are independently
sized). `glutPostRedisplay` then drives subsequent frames via `GLUT_DISPLAY_WORK`
(`fg_display.c:44`); `fgPlatformSleepForEvents` just avoids busy-spin.

> **Reshape limitation:** with a warn-only `fgPlatformPosResZordWork`,
> `glutReshapeWindow` will **not** realloc the OSMesa buffer (fine for fixed-size CI).
> Optionally honor `GLUT_SIZE_WORK` by `free`+`malloc`+`OSMesaMakeCurrent`; document
> either way.

### CMake wiring (`CMakeLists.txt`)

1. **Option** near `:87`: `OPTION(FREEGLUT_OSMESA "Headless off-screen Mesa backend" OFF)`.
2. **Platform branch FIRST** in the selection chain (currently `IF(WIN32)` at `:160`):
   make it `IF(FREEGLUT_OSMESA) … ELSEIF(WIN32) …` so it overrides the native platform on
   any OS. List the `src/osmesa/*` sources there; in that branch
   `INCLUDE(FindPkgConfig)` (it is otherwise included only inside the X11/Wayland block
   at `:446`), then `pkg_check_modules(OSMESA osmesa)` (fallback `find_library(OSMESA
   OSMesa)`) and append to `LIBS`.
3. **Skip the desktop-GL/GLES find/link block** (`:375-441`) when OSMesa is on, and add
   `OR FREEGLUT_OSMESA` to the `NOT(...)` guard of the UNIX/X11 dep block (`:342`).
4. **Demos need GLU** (`OPENGL_GLU_FOUND` FATAL at `:726`, and the glu.h FATAL at `:437`
   lives in the skipped block): under OSMesa **force `SET(FREEGLUT_BUILD_DEMOS OFF)`** and
   build the self-test below as its own target gated on `FREEGLUT_OSMESA` (linking the
   `freeglut` target + OSMesa) — don't try to satisfy the GLU-heavy `progs/` demos.
5. **Define + lib name:** near the COCOA define block (`:651`) add `IF(FREEGLUT_OSMESA)
   ADD_DEFINITIONS(-DTARGET_HOST_OSMESA=1)`. `LIBNAME` defaults to `freeglut` at `:600`
   (then `glut` via REPLACE_GLUT at `:603`); override to `glut_osmesa` so a headless build
   coexists with a normal freeglut.

### Phase 1 verification

A new self-test target `progs/osmesa-selftest/` (gated on `FREEGLUT_OSMESA`):
- `glutInit` → `glutInitWindowSize(64,64)` → `glutCreateWindow` → in `display`, clear to
  a known color + draw a triangle → `OSMesaGetColorBuffer` and assert the center pixel.
  **Run it both single-buffered (`GLUT_RGBA|GLUT_DEPTH`) and double-buffered
  (`…|GLUT_DOUBLE`)** to exercise the draw-buffer mapping. Account for OSMesa's default
  `OSMESA_Y_UP` (bottom-up) readback orientation in the pixel assert (or set
  `OSMesaPixelStore(OSMESA_Y_UP,0)`).
- `glutSolidTeapot(1)` under `glRenderMode(GL_FEEDBACK)` → assert a **non-zero**
  `GL_POLYGON_TOKEN` count (fail loudly) — the property gl-repl depends on.
- Build `cmake -DFREEGLUT_OSMESA=ON -DFREEGLUT_GLES=OFF ..`; run with no `DISPLAY` / no
  Cocoa session. Cross-check on Linux (`apt install libosmesa6-dev`).

---

## Phase 2 — Standalone `libglutshapes` (shapes + fonts)

### Goal

Compile freeglut's geometry **and** font sources into a self-contained library
(`glutSolid*`/`glutWire*`, teapot/teacup/teaspoon, stroke/bitmap text) with **no**
dependency on freeglut's windowing/event/context code. Per decision, this plan makes it
**buildable standalone and documents the emscripten path only** — the emcc/WebGL glue
lives downstream (as gl-repl is downstream of Phase 1).

### Decoupling approach — compat shim header with **symbol-renaming `#define`s**

Use a compatibility shim (no rewrite of geometry algorithms) and **`#define`-rename every
shimmed support symbol** to a private `fgshapes_*` name. Renaming is the key correctness
move: emscripten and gl4es **implement the real GLUT/GL support symbols themselves**
(notably `glutGetProcAddress`), so referencing the un-prefixed names would either collide
at link time or silently bind to the host. Renaming makes the library reference *only its
own* support symbols.

New `src/standalone/fg_glutshapes_shim.h`, active under `-DFREEGLUT_GEOMETRY_STANDALONE`:

```c
#include <GL/freeglut.h>                 /* GLUT API decls + GL types only */

#define fgState             fgshapes_State
#define fgStructure         fgshapes_Structure
#define fgError             fgshapes_Error
#define fgWarning           fgshapes_Warning
#define glutGetProcAddress  fgshapes_GetProcAddress
#define FREEGLUT_EXIT_IF_NOT_INITIALISED(s)  ((void)0)
/* the two macros fonts use (copy verbatim from fg_internal.h:983-988) */
#define freeglut_return_if_fail(e)       do { if(!(e)) return;     } while(0)
#define freeglut_return_val_if_fail(e,v) do { if(!(e)) return (v); } while(0)

/* minimal stand-ins for the structs the geometry code dereferences */
typedef struct { GLint attribute_v_coord, attribute_v_normal, attribute_v_texture; } SFG_Context;
typedef struct { GLboolean VisualizeNormals, Visible; } SFG_WindowState;
typedef struct { SFG_Context Window; SFG_WindowState State; } SFG_Window;
typedef struct { GLboolean Initialised; int HasOpenGL20, MajorVersion, StrokeFontDrawJoinDots; } SFG_State;
typedef struct { SFG_Window *CurrentWindow; } SFG_Structure;

/* the FIVE font/stroke structs (copy VERBATIM from fg_internal.h:823-863) — the font
   files won't compile without them: SFG_Font, SFG_StrokeVertex, SFG_StrokeStrip,
   SFG_StrokeChar, SFG_StrokeFont */

extern SFG_State     fgshapes_State;
extern SFG_Structure fgshapes_Structure;            /* .CurrentWindow → a static dummy window */
void  fgshapes_Error(const char *fmt, ...);          /* print + exit(1): faithful to fatal fgError */
void  fgshapes_Warning(const char *fmt, ...);        /* print to stderr; returns */
void *fgshapes_GetProcAddress(const char *name);     /* default NULL; consumer-installable */
```

Plus a small TU `src/standalone/fg_glutshapes_shim.c` defining:
- `fgshapes_State` with defaults **`{Initialised=1, HasOpenGL20=0, MajorVersion=2}`** and
  `fgshapes_Structure.CurrentWindow` → a single static dummy window (so the
  `glutSetVertexAttrib*` setters in `fg_gl2.c` work unchanged).
- `fgshapes_Error` = `vfprintf(stderr,…)` **then `exit(1)`** — the real `fgError` exits,
  and the only call sites (malloc-failure paths in `fg_geometry.c`) treat it as
  non-returning; a returning stub would run code that was never meant to execute.
- `fgshapes_Warning` = `vfprintf(stderr,…)`, returns.
- `fgshapes_GetProcAddress` default returns `NULL`, plus a setter
  `glutShapesSetProcAddressFunc(void*(*)(const char*))`. **Proc-address contract:** on the
  GLES2/emscripten build `GL_ES_VERSION_2_0` makes `fg_gl2.h` map `fgh*`→`gl*` directly,
  so the resolver is never called. On a desktop/OSMesa standalone build, if no resolver is
  installed `fgInitGL2`'s `LOADFUNC` bails early (it already guards `if(!ptr) return;`),
  leaving `HasOpenGL20=0` → the GL1.1 fixed-function path runs (works on desktop/OSMesa/
  gl4es). To use the **GL2.0 vertex-attrib path** on desktop, the consumer installs a
  resolver and calls `fgInitGL2()` (or `glutShapesSetGLVersion(2)`), then binds a shader +
  `glutSetVertexAttribCoord3/Normal/TexCoord2`. So NULL is a safe graceful-degrade, not a
  crash.

**Struct-drift safety:** the shim deliberately mirrors only the touched fields
(`attribute_v_*`, `VisualizeNormals`, plus `StrokeFontDrawJoinDots` on `fgState`). If the
geometry/font code is later changed to touch another field, the standalone build fails
with a **compile error** (the named field is simply absent) — it cannot silently read
garbage, since these are distinct named members, not an overlay of the real struct.

### The only edit to shared geometry/font code

At the top of each of `fg_geometry.c`, `fg_teapot.c`, `fg_gl2.c`, `fg_gl2.h`,
`fg_font.c`, `fg_font_data.c`, `fg_stroke_roman.c`, `fg_stroke_mono_roman.c`, replace the
lone `#include "fg_internal.h"` with:

```c
#ifdef FREEGLUT_GEOMETRY_STANDALONE
#  include "fg_glutshapes_shim.h"
#else
#  include "fg_internal.h"
#endif
```

That is the entire change to existing source — additive and invisible to every normal
build. Public entry points (`glutSolidTeapot`, `glutStrokeString`,
`glutSetVertexAttrib*`, `fgInitGL2`) keep their GLUT names (that is the exported product;
emscripten does not implement the shapes). A consumer whose host *does* stub a shape
resolves it by link order / by not also linking that host stub.

### CMake target

`OPTION(FREEGLUT_BUILD_GLUTSHAPES "Build standalone geometry+font library" OFF)` and, when
set, an independent `add_library(glutshapes …)` compiling **only** `fg_geometry.c
fg_teapot.c fg_gl2.c fg_font.c fg_font_data.c fg_stroke_roman.c fg_stroke_mono_roman.c
src/standalone/fg_glutshapes_shim.c` with `-DFREEGLUT_GEOMETRY_STANDALONE`, including
`include/` for `<GL/freeglut.h>`, linking **no** window-system libs. Orthogonal to the
platform-selection chain (it pulls no backend), so it builds on any host.

### GLES2 / fixed-function caveat (document, don't block)

The GL1.1 geometry path and the fonts (`glBitmap`, immediate-mode `glBegin/glVertex2f` in
`fg_font.c`) are **legacy fixed-function** — they work on desktop GL, the Phase-1 OSMesa
context, and **gl4es** (which emulates exactly these on GLES), but **not** raw WebGL/GLES2.
On a pure GLES2/WebGL target only the GL2.0 vertex-attrib geometry path is valid (bind a
shader + set attrib indices); fonts there need gl4es. This is why gl4es is the natural
emscripten companion for the legacy paths. State it in the README; it does not constrain
the OSMesa use case.

---

## Critical files

- **New (Phase 1):** `src/osmesa/{fg_internal_osmesa.h, fg_init_osmesa.c, fg_window_osmesa.c, fg_display_osmesa.c, fg_state_osmesa.c, fg_main_osmesa.c, fg_structure_osmesa.c, fg_ext_osmesa.c, fg_cursor_osmesa.c, fg_gamemode_osmesa.c, fg_input_devices_osmesa.c, fg_joystick_osmesa.c}`; `progs/osmesa-selftest/`.
- **New (Phase 2):** `src/standalone/fg_glutshapes_shim.h`, `src/standalone/fg_glutshapes_shim.c`.
- **Edited, additive:**
  - `src/fg_internal.h` — **only** the `#if TARGET_HOST_OSMESA` include at `:194-215` (no union/struct arm changes; the types come from the included header).
  - `CMakeLists.txt` — `FREEGLUT_OSMESA` option/first-branch/find/define/libname/demos-off; `FREEGLUT_BUILD_GLUTSHAPES` option + `glutshapes` target.
  - `fg_geometry.c`, `fg_teapot.c`, `fg_gl2.c`, `fg_gl2.h`, `fg_font.c`, `fg_font_data.c`, `fg_stroke_roman.c`, `fg_stroke_mono_roman.c` — the one guarded-include swap each.
  - `README` — both options, deps, and the GLES2/gl4es caveat.
- **Reused templates (do not modify):** `src/ogc/*` (standalone-platform + stub model, incl. self-contained `fg_main_ogc.c`), `src/egl/*` (context-call analog only), `progs/demos/timer/` (self-test shape).

## Verification

- **Phase 1:** the self-test target above — single- **and** double-buffered triangle
  pixel-readback (orientation-aware), plus the teapot `GL_FEEDBACK` token assert — built
  `-DFREEGLUT_OSMESA=ON`, run headless on macOS and Linux.
- **Phase 2:** build the `glutshapes` target (`-DFREEGLUT_BUILD_GLUTSHAPES=ON`); confirm
  via `nm` it links with **no** X11/EGL/Cocoa/OSMesa libs, exports the `glut*` shape/font
  symbols, and exports its support symbols only under `fgshapes_*`. A small desktop test
  links `glutshapes` + a real GL context (e.g. the Phase-1 OSMesa context), draws
  `glutSolidSphere` and `glutStrokeString`, and reads back a non-blank framebuffer —
  proving the geometry runs with zero freeglut windowing linked.

## Out of scope / risks

- **Menus & overlays** under OSMesa: freeglut menus are sub-windows — leave unsupported
  (assert/no-op).
- **Accum/MSAA** in swrast: best-effort/ignored, documented.
- **`GL_FEEDBACK` on Mesa:** legacy GL; works on standard distro/Homebrew Mesa — pin the
  tested Mesa version in CI.
- **emscripten build itself:** out of scope by request — documented only as the
  downstream consumer of `libglutshapes` (+ gl4es for the legacy-GL paths).
- **Upstreaming:** every edit is strictly additive/guarded, so the change is a clean
  candidate for a freeglut PR (new `FREEGLUT_OSMESA` + `FREEGLUT_BUILD_GLUTSHAPES` options
  in its CI matrix).
