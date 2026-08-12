# bitmap_bench — evidence for the Cocoa bitmap-font pixel-store change

`glutBitmapCharacter()` saves and restores the six `GL_UNPACK_*` pixel-store
values around **every glyph**; `glutBitmapString()` does it **once per call**.
Drawing identical text both ways therefore isolates the per-glyph price of
whichever save/restore strategy freeglut uses:

```
per-glyph save/restore cost  =  ns/glyph(Character) - ns/glyph(String)
```

`src/fg_font.c` picks the strategy at run time from
`FREEGLUT_BITMAP_PIXEL_STORE`:

| value | strategy |
|---|---|
| `clientattrib` | `glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT)` / `glPopClientAttrib()` — the historical path |
| `getset` | six `glGetIntegerv()`, then six `glPixelStorei()` to put them back — the Apple path |
| unset | platform default (`getset` on Apple, `clientattrib` elsewhere) |

It is read once and cached, since re-reading per glyph would cost more than the
difference being measured. A process therefore exercises exactly one strategy,
so `bitmap_bench` re-runs itself once per strategy and prints the comparison.

## Programs

| | |
|---|---|
| `bitmap_bench` | Character vs String, both strategies, optionally behind a buffer swap |
| `color_bench` | cost of changing `glColor` between bitmap glyphs |
| `pixel_store_check` | correctness: both strategies must restore what they found |
| `run_ab.sh` | build with the right backend, run the check, run the benchmark |

```sh
progs/demos/bitmap_bench/run_ab.sh                     # build + check + A/B
progs/demos/bitmap_bench/run_ab.sh --micro             # cleanest strategy A/B
progs/demos/bitmap_bench/run_ab.sh --seconds 5 --font 9x15
progs/demos/bitmap_bench/run_ab.sh --strategy getset --hold   # eyeball it
```

On a machine that scales CPU or GPU frequency, prefer `--micro` — see Method.

## Method

Each call is timed alternately pass by pass, and the reported figure is the
**minimum** over all passes — interference from the compositor, scheduler and
background load can only add time, so the minimum is the most stable estimate
of what the call itself costs. Every pass ends in `glFinish()`.

### Two measurements, and why both exist

`fg_font.c` caches its strategy choice, so the **library-level** comparison
(Character vs String) has to fork one child process per strategy. On a machine
that scales CPU/GPU frequency the two children can land on different clock
states, and no amount of averaging *inside* a child fixes a difference *between*
them. `bitmap_bench` prints a **noise check** for exactly this: `glutBitmapString`
does very nearly the same work under both strategies, so however much it differs
between the two children is roughly the measurement noise floor. Treat any run
whose noise check is comparable to the effect as worthless.

`--micro` avoids the problem entirely. The two save/restore sequences are lifted
out of `fg_font.c` verbatim — no `glBitmap`, no library involvement — so both can
be timed **in one process, interleaved**, under identical clocks. On the Mesa box
this took the spread from ±100 % down to ±1 %.

The two numbers answer different questions and do not agree, by design:

- `--micro` is the save/restore sequence **in isolation**.
- The library-level figure is that sequence **interleaved with `glBitmap`**, which
  is more expensive than the sum of its parts — the driver revalidates between
  the state change and the raster op. On Cocoa the isolated sequence costs 309 ns
  but 1479 ns per glyph in situ.

Use `--micro` to compare the two strategies. Use the library-level figure to see
what an application actually pays.

By default nothing is presented, so the figure is pure draw cost and nothing is
clamped to the refresh rate. `--swap` presents each pass as a real frame and
`--repeats N` multiplies the work per pass, which is how to push a workload past
one refresh period. Two caveats when reading `--swap` numbers:

- The Cocoa backend paces frames through CVDisplayLink and does not honour
  `glutSwapInterval`, so on macOS `--swap` always behaves as if vsync were on.
- Under frame pacing, any figure sitting at ~16 ms/frame just means the swap was
  the limit. Only figures **above** the refresh period reflect real work, and
  only those are worth comparing.

## Test machines

| host | CPU | RAM | GPU / driver |
|---|---|---|---|
| **mac** | Apple M2 (4P + 4E) | 24 GiB | Apple M2, Apple OpenGL (Cocoa backend) |
| **mac-x11** | *(same machine)* | | Apple M2, Apple OpenGL (X11/GLX via XQuartz) |
| **gracemont** | Intel N95, 4 cores | 8 GiB | Intel ADL-N, Mesa 25.2.8 (X11/GLX) |
| **zen3** | AMD Ryzen 9 5900XT, 16 cores | 16 GiB | NVIDIA RTX 5050, driver 610.43.02 (X11/GLX) |

The two Linux boxes share one monitor, so only one is display-attached at a
time; each was attached when its own figures were taken.

**mac-x11** is the same machine as **mac**, built with `FREEGLUT_COCOA=OFF`
against XQuartz. Note that it links homebrew Mesa's `libGL`, but the GL
implementation that actually answers is still Apple's — `bitmap_bench` prints
`GL: Apple | Apple M2 | 2.1 Metal - 90.5` on stderr for every run, which is why
it does that. It isolates *backend* from *driver*: same driver, different
freeglut backend.

## Results

### The strategy comparison, in isolation (`--micro`)

Nanoseconds per save/restore, 5 runs each, in one process with both strategies
interleaved. This is the cleanest available measurement of the patch itself:

| platform | `clientattrib` | `getset` | |
|---|---:|---:|---|
| **mac** — Apple GL, Cocoa | 308.8 – 309.0 | **86.5 – 86.7** | `getset` **3.6× cheaper** |
| **mac-x11** — Apple GL, X11 | 326.5 – 327.3 | **102.9 – 103.1** | `getset` **3.2× cheaper** |
| **gracemont** (Mesa) | **44.5 – 45.7** | 105.1 | `clientattrib` **2.3× cheaper** |
| **zen3** (NVIDIA) | **55.7** | 98.7 | `clientattrib` **1.8× cheaper** |

This is the whole argument for the patch. The two Linux drivers agree closely
with each other — `clientattrib` 45–56 ns, `getset` 99–105 ns — and **Apple is
the outlier**: its client-attribute stack costs ~309–327 ns, 5.5–7× what either
Linux driver charges, while its explicit query path is in the same range as
everyone else's.

**The cost tracks the driver, not the backend.** `mac` and `mac-x11` are the
same machine and the same Apple GL, reached through two entirely different
freeglut backends, and they give the same answer to within 6 %. Whereas the two
X11/GLX builds — `mac-x11` and `gracemont`, same backend, different drivers —
differ by 7×. So the expensive client-attribute stack is a property of Apple's
OpenGL implementation, and nothing to do with Cocoa.

All four reproduce to within ~1 % across runs; Mesa's and NVIDIA's figures were
identical to 0.1 ns in all five runs each.

### Consequence: the condition is `__APPLE__`, not the Cocoa backend

This measurement changed the patch. The original condition selected on
`TARGET_HOST_MACOS_COCOA` — a property of the *backend* — but the cost belongs
to the *driver*, so a macOS build with `FREEGLUT_COCOA=OFF` ran on Apple's GL
while defaulting to `clientattrib` and paid the full penalty:

| `mac-x11`, `glutBitmapCharacter` | ns/glyph |
|---|---:|
| old default (backend test → `clientattrib`) | 1915.0 |
| **new default** (`__APPLE__` → `getset`) | **516.6** |
| forced `getset`, for reference | 513.4 |

`src/fg_font.c` now reads:

```c
#if defined(GL_VERSION_1_1) && !defined(__APPLE__)
#  define FGH_PIXEL_STORE_DEFAULT FGH_PIXEL_STORE_CLIENT_ATTRIB
#else
#  define FGH_PIXEL_STORE_DEFAULT FGH_PIXEL_STORE_GET_SET
#endif
```

which recovers **3.7×** for X11-on-macOS builds. Verified unchanged elsewhere:
Cocoa still defaults to `getset` (491 ns/glyph) and Linux still defaults to
`clientattrib` (1318 ns, against 1400 for `getset`).

The residual risk is a macOS build running a genuinely non-Apple GL — a real
Mesa softpipe through XQuartz, say — which would take the `getset` branch and
pay ~60 ns per save/restore. That is the right way round to be wrong: the branch
it avoids costs ~1.4 µs per glyph in situ, over 20× more. Deciding at run time
from `glGetString(GL_VENDOR)` would close even that gap, at the cost of putting
a vendor-string check on a path that currently has none.

### What an application pays (library level)

Default workload: `8x13`, 40 lines × 80 cols = 3200 glyphs/pass, 3 s per
strategy. All figures ns/glyph.

### mac — Cocoa, Apple OpenGL compatibility profile

| strategy | Character | String | save/restore |
|---|---:|---:|---:|
| `clientattrib` | 1906 / 1919 / 1906 | 428 / 429 / 432 | **1479 / 1490 / 1474** |
| `getset` | 491 / 488 / 491 | 419 / 416 / 420 | **72 / 73 / 72** |

`glPushClientAttrib`/`glPopClientAttrib` costs ~**1.46 µs per glyph** on Apple's
implementation — roughly **20× more** than reading and writing the six values
explicitly, and more than 3× the entire cost of drawing the glyph.
`glutBitmapCharacter` gets **~3.9× faster** with the change, reproducible to
within 2 %.

**Does it survive a frame budget?** With frames actually presented
(`--swap`), at 3200 glyphs/frame both strategies fit inside a 16.7 ms budget and
the difference is invisible. Push the text past one refresh period
(`--repeats 4`, 12800 glyphs/frame) and it decides whether the frame lands:

| strategy | Character ms/frame | |
|---|---:|---|
| `clientattrib` | 31.2 / 32.6 / 31.0 | misses 60 Hz |
| `getset` | 7.6 / 13.1 / 14.0 | fits |

### mac-x11 — X11/GLX via XQuartz, Apple OpenGL

| strategy | Character | String | save/restore |
|---|---:|---:|---:|
| `clientattrib` | 1940 / 1930 / 1919 | 433 / 431 / 412 | **1507 / 1499 / 1507** |
| `getset` | 508 / 513 / 517 | 404 / 411 / 415 | **103 / 102 / 102** |

Essentially identical to the Cocoa figures on the same machine — `getset` is
**14.7× cheaper** per save/restore and `glutBitmapCharacter` is **3.8× faster**.
The backend makes no difference; the driver makes all of it. Note this is the
configuration that currently defaults to the *expensive* path.

### zen3 — X11/GLX, NVIDIA

| strategy | Character | String | save/restore |
|---|---:|---:|---:|
| `clientattrib` | 1236 / 1264 / 1234 | 164 / 168 / 165 | **1071 / 1096 / 1069** |
| `getset` | 1278 / 1274 / 1264 | 165 / 165 / 166 | **1113 / 1109 / 1099** |

Very low noise here (the String column, which is the same work either way,
agrees to 0.5 %). Two things stand out:

- The two strategies are **within 5 % of each other**, with `clientattrib`
  marginally ahead. There is nothing to win by switching.
- The save/restore costs ~1.05 µs against ~163 ns to actually draw the glyph, so
  on NVIDIA `glutBitmapString` is **~7.5× faster** than `glutBitmapCharacter`
  regardless of strategy. That is a much bigger effect than the patch, and it is
  an argument about which API applications should call rather than about
  freeglut's internals.

### gracemont — X11/GLX, Mesa

Low-noise runs only (noise check within ±3 %), 8 s each:

| strategy | Character | String | save/restore |
|---|---:|---:|---:|
| `clientattrib` | 1249 / 1288 / 1288 / 1248 | 1198 / 1212 / 1227 / 1198 | **51 / 76 / 61 / 51** |
| `getset` | 1318 / 1332 / 1320 / 1323 | 1230 / 1222 / 1202 / 1213 | **88 / 110 / 119 / 111** |

`clientattrib` is the cheaper path here — roughly 60 ns against 105 ns — which
agrees with the `--micro` figures above and is the opposite ordering to Cocoa.
Both are only 4–9 % of the ~1250 ns Mesa spends rasterising the glyph, so
nothing here is worth chasing; what matters is that switching to `getset` would
make Mesa *worse*, not better.

**On the noise.** This box is the reason `--micro` exists. It is *not* background
load (idle at 0.3), *not* thermal (52 °C against a 105 °C limit) and *not*
display sleep (DPMS timeouts are 0). It is frequency scaling: `intel_pstate` on
the `powersave` governor, with the CPU ranging 800 MHz – 3.4 GHz and the GPU
observed idling at 300 MHz of 1200. Since the two strategies are measured in
separate child processes minutes apart, they can sample different clock states —
which is how the save/restore delta came out anywhere between −206 ns and
+341 ns across runs, and why absolute glyph cost drifted between ~1250 ns and
~2000 ns between batches.

zen3 is the control: same benchmark, same methodology, but on the `performance`
governor over a much narrower range, and its numbers are stable to 0.5 %.

Pinning with `taskset` made things worse rather than better — the N95 is all
Gracemont E-cores, so pinning just puts the benchmark and the GL driver thread
on the same core. Without root to change the governor, the fix is the in-process
`--micro` comparison, which removes the confound instead of averaging over it.

## Cost of changing glColor between glyphs

`color_bench` draws the same glyphs with `glutBitmapCharacter()` throughout and
varies only how often `glColor3f()` is called — the pixel-store handling is
identical in every case, so the delta is purely the colour change:

| platform | one colour | colour/line | colour/glyph | per-glyph colour cost |
|---|---:|---:|---:|---:|
| **mac** (Apple GL, Cocoa) | 497.2 | 496.3 | 496.9 | **−0.3 ns** |
| **mac-x11** (Apple GL, X11) | 1903 – 1935 | 1905 – 1948 | 1900 – 1953 | **−2.8 … +17.8 ns** |
| **zen3** (NVIDIA) | 1235 – 1278 | 1238 – 1278 | 1248 – 1287 | **+9.1 … +13.1 ns** |
| **gracemont** (Mesa) | 1260 – 1328 | 1260 – 1331 | 1270 – 1342 | **+8.6 … +23.3 ns** |

5 runs each on the Linux boxes, 8 s per run.

A colour change between bitmap glyphs is essentially free everywhere measured —
about 1 % of the cost of the glyph, and well below the save/restore effect.

(The **mac-x11** absolute figures are ~4× the **mac** ones because that build
defaults to `clientattrib`, per the section above — the colour *delta* is
unaffected, which is the point.)

Note this benchmark does not suffer the frequency-scaling problem described
above: all four cases run **in one process, interleaved**, so they always share a
clock state. The Mesa spread across runs comes from the absolute glyph cost
moving (1260–1342 ns), while the colour *delta* stays in a narrow band.

The Mesa sensitivity to colour changes that prompted this test did not reproduce
in this shape of workload — bitmap glyphs via `glutBitmapCharacter` with
`glColor3f` between them, on Mesa 25.2 / Intel N95. If the original problem
involved a different call (`glColor` inside `glBegin`/`glEnd`, vertex arrays, or
a different Mesa driver), that is a separate case this program does not cover.

## Correctness, not just speed

Both strategies must leave the unpack state exactly as they found it.
`pixel_store_check` sets the six values to non-defaults, draws with each
bitmap-font entry point, and reads them back:

```sh
for s in clientattrib getset; do
    FREEGLUT_BITMAP_PIXEL_STORE=$s ./bin/pixel_store_check || echo "$s FAILED"
done
```

Both paths pass on macOS/Cocoa and on Linux (Mesa and NVIDIA).

## Conclusion

Measured in isolation, the save/restore costs **309 ns (clientattrib) vs 87 ns
(getset) on Apple's GL**, **45 ns vs 105 ns on Mesa**, and **56 ns vs 99 ns on
NVIDIA** — the ordering reverses between Apple and the rest. Apple's
client-attribute stack is the outlier: 5.5–7× more expensive than either Linux
driver's, while its explicit query path is in the same range as everyone's.

Testing the same Apple GL through the X11 backend gives the same answer
(327 vs 103 ns), which pins the cost to the **driver**, not the backend.

At the library level that is worth **~3.9× on `glutBitmapCharacter`** on Cocoa,
and enough to decide whether a text-heavy frame lands inside a 60 Hz budget. On
Mesa and NVIDIA the historical path is the cheaper of the two, by margins too
small to matter (4–9 % of glyph cost).

So the change must not be promoted to the default everywhere, where it would be
a small regression. Scoping it to the *Cocoa backend* was also wrong, in the
other direction — too narrow, since the same driver reached through X11 is
equally slow. The condition is now `__APPLE__`, which is what the measurement
supports: it follows the driver rather than the windowing backend, and picks up
a 3.7× win for `FREEGLUT_COCOA=OFF` builds on macOS that the original patch
missed.

Two findings that are not about this patch but showed up alongside it:

- The save/restore dominates the glyph on some drivers (~1.05 µs vs ~163 ns to
  draw on NVIDIA), making `glutBitmapString` **~7.5× faster** than
  `glutBitmapCharacter` there regardless of strategy. Which entry point an
  application calls matters more than which strategy freeglut uses.
- Changing `glColor` between bitmap glyphs is free everywhere measured.
