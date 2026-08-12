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
| `getset` | six `glGetIntegerv()`, then six `glPixelStorei()` to put them back — the Cocoa path |
| unset | platform default (`getset` on Cocoa, `clientattrib` elsewhere) |

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
progs/demos/bitmap_bench/run_ab.sh --seconds 5 --font 9x15
progs/demos/bitmap_bench/run_ab.sh --strategy getset --hold   # eyeball it
```

## Method

Each call is timed alternately pass by pass, and the reported figure is the
**minimum** over all passes — interference from the compositor, scheduler and
background load can only add time, so the minimum is the most stable estimate
of what the call itself costs. Every pass ends in `glFinish()`.

By default nothing is presented, so the figure is pure draw cost and nothing is
clamped to the refresh rate. `--swap` presents each pass as a real frame and
`--repeats N` multiplies the work per pass, which is how to push a workload past
one refresh period. Two caveats when reading `--swap` numbers:

- The Cocoa backend paces frames through CVDisplayLink and does not honour
  `glutSwapInterval`, so on macOS `--swap` always behaves as if vsync were on.
- Under frame pacing, any figure sitting at ~16 ms/frame just means the swap was
  the limit. Only figures **above** the refresh period reflect real work, and
  only those are worth comparing.

## Results

Default workload: `8x13`, 40 lines × 80 cols = 3200 glyphs/pass, 3 s per
strategy. All figures ns/glyph.

### macOS 15 (Darwin 25.6), Cocoa backend, Apple OpenGL compatibility profile

| strategy | Character | String | save/restore |
|---|---:|---:|---:|
| `clientattrib` | 1882 / 1882 / 1902 | 428 / 427 / 431 | **1454 / 1456 / 1471** |
| `getset` | 491 / 488 / 488 | 416 / 408 / 415 | **75 / 79 / 72** |

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

### Ubuntu 24.04, X11/GLX, NVIDIA (proprietary driver, 8 GB)

| strategy | Character | String | save/restore |
|---|---:|---:|---:|
| `clientattrib` | 1214 / 1214 / 1221 | 161 / 163 / 162 | **1053 / 1052 / 1059** |
| `getset` | 1266 / 1263 / 1278 | 165 / 164 / 165 | **1101 / 1099 / 1112** |

Very low noise here (the String column, which is the same work either way,
agrees to 0.5 %). Two things stand out:

- The two strategies are **within 5 % of each other**, with `clientattrib`
  marginally ahead. There is nothing to win by switching.
- The save/restore costs ~1.05 µs against ~163 ns to actually draw the glyph, so
  on NVIDIA `glutBitmapString` is **~7.5× faster** than `glutBitmapCharacter`
  regardless of strategy. That is a much bigger effect than the patch, and it is
  an argument about which API applications should call rather than about
  freeglut's internals.

### Mesa

Pending a re-run — the Mesa box was not attached to a display when first
measured, and its numbers were too noisy to draw a conclusion from (the
save/restore delta swung between −85 ns and +266 ns across runs, i.e. the effect
was smaller than the run-to-run spread).

## Cost of changing glColor between glyphs

`color_bench` draws the same glyphs with `glutBitmapCharacter()` throughout and
varies only how often `glColor3f()` is called — the pixel-store handling is
identical in every case, so the delta is purely the colour change:

| platform | one colour | colour/line | colour/glyph | per-glyph colour cost |
|---|---:|---:|---:|---:|
| macOS / Cocoa | 491.2 | 492.8 | 494.1 | **+2.8 ns** |
| Linux / NVIDIA | 1227.1 | 1224.2 | 1233.1 | **+6.0 ns** |

A colour change between bitmap glyphs is essentially free on both — about 1 % of
the cost of the glyph, and far below the save/restore effect. Whatever Mesa
sensitivity to colour changes exists, it did not show up in this shape of
workload on the hardware measured so far; the Mesa re-run will confirm.

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

The expensive client-attribute stack is specific to Apple's OpenGL
implementation, where the fix is worth ~3.9× on `glutBitmapCharacter` and can be
the difference between hitting and missing a frame. On NVIDIA the historical
path is marginally the cheaper of the two. So the change belongs scoped to the
Cocoa backend, exactly as `src/fg_font.c` has it — not promoted to the default
everywhere.
