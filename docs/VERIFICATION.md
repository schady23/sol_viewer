# Cleanup and release-candidate verification — 2026-10-01

- Release build: all 19 CPU/data/input suites passed, including new strict CLI
  parsing and Earth interaction regressions.
- Clang AddressSanitizer, UndefinedBehaviorSanitizer and LeakSanitizer: all five
  focused suites passed (`cli`, `earth_view`, `earth`, `guide`, `navigation`).
  Leak checks ran outside the process-tracing sandbox.
- Installed Linux candidate: 48 offscreen OpenGL render cases and eight invalid
  launches passed from an empty working directory. Every included world and both
  belts rendered; screenshots for Earth guides/topography, the Earth system,
  Moon notes and the solar overview were visually inspected.
- Earth cloud images at 0 and 60 seconds match exactly, 15 seconds differs, and
  topography images remain identical across cloud phases.
- The optional quick OpenGL suite also passed through CTest: 20/20 total suites.
- All packaged assets and terrain matched the checkout's SHA-256 hashes. The
  staged executable resolves SDL from its own `lib/` directory.

Reproduction, limitations and the remaining Steam release work are in
[release preparation](RELEASE.md). Offscreen results are functional checks, not
native GPU benchmarks. Windows and Valve runtime SDK builds remain unverified.

## Historical framework verification — 2026-09-27

Release build with warnings enabled on Linux / Radeon 680M. Four CTest suites
pass: navigation, terrain, environment and orbit. The data-only configuration also
builds and passes all four without requiring SDL or an OpenGL context. Windows
source/build support is retained but has not been compiled or run on Windows here.

New CPU checks cover all 19 body IDs and parent chains, named lookup, repeated
open/up transitions, retained parent selections/cameras, all globe return paths,
Mars dispatch, globe/ring fit in landscape and portrait, overlap picking, labels,
HUD interception and UI fitting across sizes/scales. Existing terrain, weather
and satellite tests remain intact.

Native checks on this machine:

- Solar overview, Jupiter system and Europa globe render with **zero Mars terrain
  allocations**. Native overview ran about 117 FPS in a short 90-frame check.
- Entering Mars loads exactly **33,177,600 triangles**; the default centre readout
  remains 12 N, 253 E, +5283.25 m. Smoke checks validate terrain picking and that
  weather leaves its depth untouched.
- An app-local SDL event driver completed Solar -> Jupiter -> Europa -> Jupiter ->
  Solar -> Mars system -> Phobos -> Mars system -> Mars -> Mars system -> Deimos ->
  Mars system -> Mars. It used list clicks, double-click, Enter, breadcrumbs,
  Alt+Left and the Mars Up button. Only one Mars preparation/upload occurred.
- The same tour edited a satellite to **400 x 2400 km / 98.5 degrees**, paused it,
  selected nadir and 300x time, changed UI to 125%, and returned with these values
  and mean anomaly zero preserved. It exercised Tab, fullscreen exit and window
  resizing. The 16.56-second run averaged about 118 FPS including loading.
- Explicit missing MOLA path: overview still works without loading it; Mars shows
  a labeled unavailable-data view. Retry logs a second error; Up returns through
  Mars system to Solar without closing the application or allocating terrain.
- Native Mars flight at 100 x 20,000 km / 90 degrees, paused, rendered and passed
  its picking checks at full terrain detail.
- UI was inspected at 200% fitted scale, in fullscreen and a 1242 x 1508 portrait
  window. A portrait framing issue found during verification was corrected.

Performance numbers are short local observations, not cross-platform guarantees.
GPU upload is still a main-thread operation and can briefly pause presentation.

## Reproduce basic checks

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
ctest --test-dir build --output-on-failure
./build/solar_system_sim --fullscreen --frames 90 --screenshot build/overview.ppm
./build/solar_system_sim --fullscreen --start system --body jupiter --frames 60 --screenshot build/jupiter.ppm
./build/solar_system_sim --fullscreen --start globe --body europa --frames 60
./build/solar_system_sim --fullscreen --start mars --pause-weather --frames 90 --screenshot build/mars.ppm
./build/solar_system_sim --start solar --data build/nonexistent.mola --frames 60
./build/solar_system_sim --start mars --data build/nonexistent.mola --frames 60
```

`--frames` counts settled frames for the initial Mars load, so a terrain screenshot
is not accidentally captured before the worker finishes. Loading errors count as
settled and are recoverable. The benchmark log distinguishes cached and submitted
terrain triangle counts. The temporary native SDL driver and batch checker used
here are in `build/ui_driver.cpp` and `build/check_native.py`; they send input only
inside their own application process, not through the desktop.

## Manual framework checklist

1. Open Jupiter from the overview, rotate/zoom its system, open Europa, then use
   Up twice. Confirm cameras and selections return as expected.
2. Open Mars system, enter the existing Mars globe, select Olympus Mons, and toggle
   relief/weather. Back out and re-enter; confirm its camera and settings persist.
3. Enter Mars satellite mode, edit and pause an orbit, visit a moon, and return.
   Check that the same orbit, phase and view resume. `O` still restores the atlas.
4. Open Sun, Mercury, Saturn and a remaining moon placeholder. Check labels, ring framing,
   reference-globe notices and the absence of fabricated elevation readouts.
5. Change UI scale, resize, toggle fullscreen and hide/show the HUD. Check body
   clicks, list clicks and labels; a drag must not accidentally open a body.
6. Launch with a bad Mars data path. Verify the overview works, error/retry behavior
   is explicit, and both breadcrumbs and Up still work.

---

The following sections are historical verification of the inherited Mars
subsystem. Their launch commands explicitly select Mars in the new application.

# Milestone 1 verification

## Native-grid detail and satellite orbit — 2026-09-26

- The requested **4× finer linear spacing** is now the default: 0.0625°, 16,588,802
  vertices and 33,177,600 triangles. A temporary full-mesh check verified dimensions
  and the four native Olympus vertex heights (20002, 20046, 20072, 20088 m) against
  previously independently decoded IMG values, at the PDS pixel centres.
- Release builds pass with warnings enabled. All three CTest targets pass, including
  the data-only build with `MARS_BUILD_APP=OFF`. The real-data reference tests pass.
- Terrain tests now check draw-batch partitioning, angular/height/bounding-sphere
  containment and front/back culling in addition to closed seam/pole topology.
- Orbit tests cover apsides, circular and high-e cases, speed ordering, energy,
  angular momentum, Mars rotation, polar/retrograde motion, pause/disabled state,
  invalid parameters, lens/aspect/attitude depth unprojection and scaled UI layout.
- Native full-resolution renders were inspected for the whole globe, 100 km low
  orbit, 20,000 km apoapsis, circular polar nadir view, a 300 km pass over Valles,
  closest polar atlas view and Olympus at closest zoom with 10× relief. GL error and
  weather-depth-preservation smoke checks passed.
- Culling-on and `--no-cull` screenshots are **byte-for-byte identical** for low
  polar-inclination orbit at 100 km, the closest north-pole atlas view, and the closest
  10× Olympus view. The near-pole comparison includes longitude 359.99°.
- An app-local SDL event driver typed all three orbital values, rejected a 99,999 km
  periapsis, cancelled that edit without exiting fullscreen, used the step buttons,
  changed speed/view/UI scale, paused/resumed, exited/re-entered flight, and returned
  from fullscreen to a portrait window. Final state matched the actions: 400×2400 km,
  98.5°, 300× time, paused, UI 125%. No system input was injected outside this app.
- A separate look-around/wheel check pointed above the horizon. The UI correctly
  showed `SUBSATELLITE`, the unchanged 100 km orbit and a 43° lens FOV.
- Seven invalid CLI cases (unordered/out-of-range apsides, non-finite/out-of-range
  inclination, fractional detail level and unsupported time scale) were rejected
  before creating a window or loading terrain.
- On this Radeon 680M at 2560×1600, short 240-frame runs were about **115 FPS** for
  both the full-detail globe and animated orbit. The last draw contained 14.9 million
  of 33.2 million triangles in the globe view and 0.56 million in low orbit. These
  are short device-specific checks, not a general performance guarantee. GPU terrain
  buffers still occupy 633 MiB; culling reduces draw work, not memory residency.
- Windows compilation/runtime and mixed-DPI monitor transitions remain unverified.

Reproduce the final previews:

```sh
./build/solar_system_sim --start mars --fullscreen --frames 240 --screenshot build/native16-globe.ppm
./build/solar_system_sim --start mars --fullscreen --orbit --periapsis 300 --apoapsis 300 --inclination 90 --orbit-phase 340 --lon 301.41 --orbit-paused --pause-weather --frames 120 --screenshot build/orbit-valles.ppm
```

For a culling comparison, freeze weather and orbital time, hide the UI, then run
identical arguments twice with/without `--no-cull`; compare the resulting PPMs.
Allow at least 30 frames for an asynchronous fullscreen transition to settle.

Manual acceptance additions:

1. Use `O` to enter/exit flight. Check that the atlas camera and chosen relief return.
2. Type periapsis/apoapsis/inclination; try an invalid ordering and cancel with Escape.
3. Compare a circular orbit with an eccentric orbit; altitude is constant only for
   the circle, while speed decreases toward apoapsis for the ellipse.
4. Set 0°, 90°, 180° inclination; inspect equatorial, polar and retrograde travel.
5. Pause with Space, change time scale with S, switch views with V, then drag and
   zoom the lens. These look controls must not change orbital elements.
6. Change UI scale and fullscreen state, then edit numbers again; input must stay
   aligned and must not also start a camera drag.
7. Compare `--detail 16`, `--detail 8` and `--detail 4` at the same landmark. The
   surface becomes more detailed, but the same geographic MOLA readout stays fixed.

## Ice, weather and industrial UI — 2026-09-26

- Release build succeeded with warnings enabled. Both CTest targets pass. Existing
  real-MOLA range and independent landmark reference tests also pass.
- A fresh `MARS_BUILD_APP=OFF` build and both CPU tests pass without SDL/OpenGL.
- New CPU checks cover the automatic storm lifecycle, frame-rate-independent seeded
  scheduling, pause, preview, clean-mode suppression/restoration, and control fitting
  across 800×500, 1242×740, 1242×1508, 2560×1600 and 3840×2160 at all six UI scales.
- Native Linux GL runs exercised a regional storm, north-cap overview, the closest
  south-pole view, longitude 359.99° at closest zoom, enlarged/clamped UI, 75% UI,
  clean topography and 1× relief. Saved frames were inspected. The first draft's
  detached cloud rim was corrected with a fade inside the terrain silhouette.
  Diffuse ice lighting reduces the existing fine polar shading artifacts; the
  underlying coarse/polar mesh has not acquired new detail.
- Bounded GL smoke checks verify no GL errors, centre unprojection and unchanged
  off-centre picking before/after weather. Olympus remains +20037.8 m at its centre.
- A temporary app-local SDL event driver exercised F11 entry, UI-scale button clicks,
  environment menu expansion, ice/cloud toggles, pause, preview while paused, clean
  topography on/off, scaled destination/relief clicks, Tab twice, Escape back to a
  1242×740 window, and the bracket scale shortcut. Final state matched the sequence:
  UI 125%, ice/clouds off, dust enabled, paused, clean mode off, Valles at 1× relief.
  The captured pointer readout was aligned. Events were sent only to this app.
- Short 2560×1600 globe runs with weather measured approximately 104–114 FPS on the
  Radeon 680M; startup-heavy 30-frame close-up runs measured 83–95 FPS. These are
  smoke measurements, not sustained or cross-device performance guarantees.
- `--ui-scale nan` is rejected before loading data. Windows compilation/runtime and
  mixed-DPI monitor transitions remain unverified on this Linux machine.

New reproducible visual cases:

```sh
./build/solar_system_sim --start mars --fullscreen --environment-open --preview-storm --pause-weather --frames 120 --screenshot build/weather.ppm
./build/solar_system_sim --start mars --fullscreen --lat 70 --lon 20 --distance 2.4 --frames 60 --screenshot build/ice.ppm
./build/solar_system_sim --start mars --fullscreen --lat -90 --distance 1.085 --ui-scale 2 --environment-open --frames 30
./build/solar_system_sim --start mars --fullscreen --landmark 1 --relief 1 --topography --frames 30
```

Manual additions: adjust scale and click controls after resizing; toggle each layer;
use `P` to freeze motion and `D` to preview; use `T` to inspect height colours and
restore selected layers. Leave weather running through a quiet interval to see an
automatic regional storm (it can spawn on the far hemisphere). `Tab` hides the UI,
whereas `T` suppresses environmental overlays.

## Full-window update — 2026-09-26

- Rebuilt successfully with system CMake; CTest passes, including added checks
  for near-plane clearance at the closest 10× view and depth unprojection across
  zoom/aspect ratios.
- Native 2560×1600 borderless fullscreen, the finer 2,070,720-triangle mesh,
  translucent overlays, the 1.085-radius closest zoom, 8-radius farthest zoom,
  hidden HUD, coarse mesh option and close polar rendering were exercised.
  The fullscreen smoke run was about 111 FPS on this machine; short runs are
  not performance guarantees.
- A temporary SDL event driver in `build/` exercised F11, two Tab presses,
  destination and relief-button clicks after resizing, and Escape. It checked
  the real fullscreen flag on entry and exit. The final view was Valles Marineris
  at 1× relief, confirming both resized button hitboxes. A centre pointer event
  after returning to a portrait-shaped window produced an aligned `POINTER`
  readout in the saved frame. No desktop keyboard/mouse events were injected
  outside the test application's own event queue.
- Close-up facet normal calculations use local affine coordinates, and light
  band transitions are softened to avoid magnifying floating-point noise.
  Scientific samples and source files remain unchanged.

Start with `./build/solar_system_sim --start mars --fullscreen`, use F11 to switch modes, and Tab to
hide/show the overlay. Escape leaves fullscreen first; a second Escape quits.

## Original baseline — 2026-09-25

Verified on 2026-09-25 on Linux, GCC 16.2.1, SDL3 3.4.16, CMake 3.31.8,
AMD Radeon 680M / Mesa radeonsi. The system did not have CMake installed, so
verification used the official CMake 3.31.8 binary unpacked in `/tmp`; the
project itself has no CMake binary dependency at runtime.

## Initial implementation checks

- Built the converter first with the system C++ compiler and converted the actual
  downloaded 16 ppd source. Built the native app next and ran its real GL renderer.
- Configured and built all CMake targets in Release with warnings enabled.
- Separately configured and built with `MARS_BUILD_APP=OFF`, confirming the data
  tool and tests need neither SDL nor OpenGL.
- CTest passed. Tests cover label format/direction/scale/sentinel rejection,
  signed endian decoding, exact pixel centres, bilinear interpolation, longitude
  wrapping, common polar heights, coordinate round trips, outward nondegenerate
  triangles, every mesh edge used twice, spherical topology, round-trip storage,
  checksum corruption and truncation rejection.
- Tested the real converted grid: extrema −8177/+21171 m and independently
  decoded landmark reference samples match.
- Ran bounded native render checks for the whole globe, Olympus Mons, Valles
  Marineris, both exact poles at the closest zoom, and longitude 0° at closest
  zoom. Centre-depth unprojection and GL error checks passed in each case.
- Visually inspected saved frames for the globe, Olympus, Valles, seam and poles.
  No open mesh boundary appeared. Polar sampling/long thin triangles can show
  radial facets at extreme zoom, especially at 10×; this is a coarse atlas.
- Rebuilt after the final HUD/picking changes and checked the globe, Valles,
  and Hellas at 1×. All passed; final 120-frame runs were about 117–118 FPS.
- Checked missing-data startup and converter overwrite refusal. Both return
  errors without substituting terrain or overwriting the prepared grid. The
  download script verifies/skips the already downloaded source files correctly.
- Dynamic-link inspection shows SDL3 plus standard C/C++/math runtimes; there is
  no third-party renderer, UI, font, image, geospatial, or GL-loader library.

Reference samples were independently evaluated using Python's `struct.unpack('>h')`
directly on the original IMG, using the PDS pixel-centre equations. At Olympus
(18.65°N, 226.20°E), the surrounding samples in NW/NE/SW/SE order are
`20002, 20046, 20072, 20088`, giving **20037.84 m**. At Hellas
(−42.43°, 70.50°E), they are `−6019, −6072, −6052, −6016`, giving
**−6041.13 m**. Valles at (−14.01°, 301.41°E) is **−4710.116 m**;
Elysium Planitia at (2.98°, 154.74°E) is **−2725.66 m**.

Initial 90-frame landmark runs were about **117 FPS** with vsync on a 120 Hz
display. These are short device-specific smoke runs, not a general performance
guarantee. Use a longer benchmark on your target machine:

```sh
./build/solar_system_sim --start mars --frames 1200 --benchmark
```

## Reproduce

After the README's build/data steps:

```sh
ctest --test-dir build --output-on-failure
./build/terrain_tests docs/megt90n000eb.lbl data/mars16.mola
./build/solar_system_sim --start mars --frames 120 --screenshot build/globe.ppm
./build/solar_system_sim --start mars --landmark 1 --frames 120 --screenshot build/olympus.ppm
./build/solar_system_sim --start mars --landmark 2 --frames 120 --screenshot build/valles.ppm
./build/solar_system_sim --start mars --landmark 3 --relief 1 --frames 120 --screenshot build/hellas-1x.ppm
./build/solar_system_sim --start mars --lat 90 --lon 0 --distance 1.085 --frames 30 --screenshot build/north.ppm
./build/solar_system_sim --start mars --lat -90 --lon 359.99 --distance 1.085 --frames 30 --screenshot build/south.ppm
./build/solar_system_sim --start mars --lat 0 --lon 0 --distance 1.085 --frames 30 --screenshot build/seam.ppm
```

Windows equivalents are under `build/Release/` with `.exe`; CTest needs
`-C Release`. Windows compilation/runtime, mixed-DPI monitor changes, and a full
interactive input pass have not been performed in this environment. Use the
manual checklist in the README on each target machine.

## Manual acceptance checklist

1. Orbit with drag and arrows. Release the mouse outside the view and switch
   focus; orbit must stop. `Home` returns to the initial view.
2. Scroll and use both keyboard zoom controls; test maximum/minimum distances.
   The camera must never enter the exaggerated terrain.
3. Click each destination card and use keys 1–4. Names, coordinates and the
   active card should agree. Olympus is elevated; Valles is recessed; Hellas is low.
4. Hover at several locations and off the globe. The `POINTER`/`CENTRE` labels
   must correctly explain which location is measured.
5. At a fixed centre, switch 10×/1×. The outline/relief changes; geographic
   coordinates and elevation must not. Pointer coordinates may change because
   the terrain under that pixel moves.
6. Inspect ±90° and sweep across 0°/360°. No holes, split edges, or sudden
   longitude-camera jumps. Exact pole longitude is only a camera orientation.
7. Resize and move between monitors. Pixel-snapped UI edges should remain sharp;
   terrain should fill the drawable without letterboxing. Hitboxes and depth picking must track the viewport.
8. Toggle F11 in both directions and use Escape to leave fullscreen. Toggle Tab
   twice; the overlay and markers disappear and return. Click destination/relief
   buttons after each resize; clicks must not also orbit the globe.
9. Minimize/restore and close. Try a missing or truncated `.mola`; expect a clear
   terminal error with no invented substitute data.

## Detailed Mercury — 2026-09-27

Built with the existing Linux C++17/SDL3/OpenGL stack, without additional runtime
libraries. Release build completed without warnings. The SDL-free `build-data`
configuration also builds `mercury_convert` and its tests. Windows was not run.

- All **five CTest suites** pass: Mercury, navigation, terrain, environment, orbit.
- The complete 530,841,600-byte PDS V02 image and detached label match their pinned
  SHA-256 hashes. Conversion found **zero missing samples**. The saved checked DEM
  loaded successfully; the download script also verified the existing source files.
- New tests exercise metadata rejection (format/scale/datum/coordinates/product),
  signed half-metre sampling, wrapping, pole convergence, missing interpolation,
  closed mesh edges/winding, omitted missing triangles, clipping clearance, bad
  header/length/checksum and Mercury navigation routing.
- Independent Python `struct.unpack('<h')` reads of the four raw neighbours at each
  landmark reproduce the C++ bilinear heights: Caloris −441.591 m, Rembrandt
  −2047.700 m, Rachmaninoff −5276.3486 m and Beethoven −1060.5806 m. These became
  optional real-data regression assertions in `mercury_tests`.
- Existing real MOLA tests still pass, including reference samples, source range,
  closed mesh, checksum and truncation checks.

Native OpenGL checks on Radeon 680M, 2560×1600 fullscreen, plus a 1242×1508 tiled
window, passed centre-depth unprojection and GL error checks. Reviewed screenshots
cover the global elevation palette, Rachmaninoff at 1×/10×, longitude 0° at the
minimum camera distance, both poles and 200% fitted UI. Polar facet lighting was
softened after visual review to suppress thin-triangle shading spokes; terrain
geometry and source samples are unchanged. Both poles were rendered/reviewed again
afterward. Olympus Mons was also rendered and reviewed after the shared renderer
changes. [Finished Mercury preview](mercury-preview.png).

At the Rachmaninoff centre the readout stays **−5276.35 m** at both relief factors.
A hidden-HUD Rembrandt close-up at 4 ppd produced byte-identical images with
culling enabled and disabled (11,200 versus 2,073,600 submitted triangles).
Default detail keeps 33,177,600 triangles in each separate terrain allocation.

An app-local SDL event tour (no desktop input) completed:

1. Enter detailed Mercury; select Rachmaninoff, 1× relief, elevation colors, 125% UI.
2. Click Up, click the Solar breadcrumb, select Mars system and enter Mars.
3. Enable/pause Mars satellite mode and switch to nadir.
4. Return via Mars system and Solar to Mercury system and its globe.
5. Confirm Mercury camera 27.79° N / 57.58° E / 1.12 radii, 1× relief and elevation
   colors are preserved. Both default terrain meshes remain allocated. Mercury
   loaded only once; Mars retained its own paused orbit state.

The 21-second tour averaged about 117 rendered frames/second on this machine,
including loading and transitions, with a 120-Hz display; this is not a guarantee
for other hardware. Separate missing-file tests performed R/retry (two logged
failures), returned to the solar system, and retained Mercury selection. Neither
body allocated terrain for the missing-file case.

Final launch checks confirm `--start mercury --landmark 3 --relief 1` configures
Mercury without changing Mars's initial view; `--start mars --landmark 1` leaves
Mercury at its default camera. The final preview uses the rebuilt executable.
The bounded QA scripts/logs/screenshots are in ignored `build/`; durable numerical
checks are in `tests/mercury_tests.cpp` and the manual checklist is in
[MERCURY.md](MERCURY.md).

## Field Guide, archive scanning and system previews — 2026-09-27

The Linux Release application and SDL-free data-tool configuration build without
warnings. All **six CTest suites** pass in both configurations. New guide tests
cover article completeness, wrapping, scrolling, location selection, scan timing,
preview identity/datum validation and geometry. Layout checks cover 800×500 through
2560×1600 and 75–200% UI scale. No runtime dependencies were added.

Native OpenGL screenshots were reviewed at 2560×1600 fullscreen and in a
1242×1508 tiled window, including fitted 200% UI. The window manager retained its
tiled size when the test requested 800×500; that minimum size was checked by the
layout tests, not claimed as a native visual test. Windows and mixed-DPI monitor
changes remain untested.

An app-local SDL event tour opened the Mercury guide, scrolled to the last facts,
browsed all four location articles, focused Beethoven, reopened the guide, closed
it with Esc, replayed a scan, changed UI scale, left fullscreen, and returned to
the local system. The final camera matched Beethoven (20.92° S, 236.34° E,
1.22 radii). Esc closed the panel without exiting the application or fullscreen.

Additional bounded native launches verified:

- Mars's Elysium article follows the selected destination; its elevation readout
  remains −2725.66 m at the marker.
- Initial Mercury loading displays the relief preview, moving contours and real
  archive-loading stage. Two captured loading frames differ; a later R replay
  also visibly sweeps the detailed surface without another load.
- Mars and Mercury local systems use their separate **65,536-triangle** previews
  while both detailed-terrain allocations remain empty until requested.
- A missing Mercury dataset leaves a recoverable error, usable guide and preview.
- The guide renders and scrolls alongside the existing paused Mars satellite view.
- OpenGL error checks and detailed-view depth-unprojection checks pass.

The review also removed a misleading Retry label during an active load; retry
remains available after failure. The rebuilt executable includes that correction.
The Source button's external-browser launch was not exercised. Source definitions,
controls and the remaining manual checklist are in [FIELD_GUIDE.md](FIELD_GUIDE.md).
Test scripts, logs and screenshots remain in ignored `build/`.

## Left sidebar and inline location notes — 2026-09-27

The Field Guide now contains planet information only and opens below the left
breadcrumbs. Globe destination selections expand independent notes beneath the
location list. The selected paragraph can collapse and scroll; the Mars orbit
button moves below it without covering the elevation readout. The solar body
list reserves a stable position whether or not its selected planet has a guide,
so enabling the guide cannot shift a double-click target.

The Release build completes without warnings and all six CTest suites pass.
Extended layout/state checks cover left docking, independent notes, selection
changes, scroll limits, compact 800×500 layout, 75–200% scaling, elevation/orbit
clearance and stable body-list hit areas.

Native SDL event tours for both planets verified selecting destinations, collapsing
notes, reopening on another selection, planet-guide wheel scrolling, inline-note
wheel scrolling, and 200% fitted UI. The final landmark distances remained 1.22
Mercury radii at Rembrandt and 1.65 Mars radii at Valles Marineris, confirming that
wheel events over text did not also zoom. Mars entered satellite mode, opened and
closed the left guide, then used the relocated Exit Satellite button to restore
the landmark view. Screenshots of normal/fitted notes, guide, solar body list and
satellite controls were reviewed at 2560×1600. These UI tours used 4-ppd meshes;
the default detailed globe settings are unchanged.

The final native solar-navigation check also double-clicks Mercury after opening
and closing its guide and verifies entry into the Mercury system. The app-local
QA scripts and screenshots are in `build/`; no desktop input or external-browser
launch was used. Windows remains untested in this environment.


## Initial Jupiter atmosphere — 2026-09-28 (superseded appearance)

The final implementation uses a faceted procedural loop, Hubble-derived latitude
color reference, a 589,824-triangle globe and about 40.5 MiB of GPU vertex data.
The source image is not repeatedly advected and is not presented as current weather.
No new runtime library is linked. The original Mars project is untouched.

- Release build completed with the existing warning flags; all **seven CTest
  suites** passed. Jupiter tests cover frame-rate-independent clock advancement,
  pause/reset/rate selection, simultaneous cloud/rotation loop closure, latitude
  conversion, opposite jet directions, pole limits, UI bounds at multiple sizes,
  reference-map orientation, dimension/length/checksum rejection. Navigation
  tests include Jupiter dispatch and return to its selected parent.
- Native OpenGL 3.3 checks on Radeon 680M at **2560 × 1600** averaged approximately
  **119 FPS** over the 10.8-second animation check and 14.2-second navigation tour
  (vsync enabled). This is an observed result on this machine, not a cross-device
  guarantee. `--jupiter-low` retains 65,536 triangles for less GPU work/storage.
- Hidden-UI framebuffer captures were byte-identical while paused, after resetting
  the animation to its initial state, and at loop phases **0° and 360°**. Mid-loop
  captures differ. Near-wrap phases 359.99° and 0.01° were also rendered for review.
- The SDL event tour exercised solar → Jupiter system → Jupiter globe, Track Red
  Spot, time +/- and pause buttons, Watch/Follow controls, left guide scrolling,
  inline region notes and 200% fitted UI. It then visited the unchanged Io
  placeholder and returned with Jupiter's camera, tracking selection and paused
  time preserved. It returned to solar, opened Mars, then Mercury successfully.
- Mars/Mercury coexistence checks used their 4-ppd meshes (2,073,600 triangles each),
  retaining native source readouts. Their default centre probes remained
  +5283.25 m for Mars and +124.625 m for Mercury. This change does not modify their
  terrain generation, surface sampling or shaders.
- Both poles, the atan longitude seam, the low-detail option, normal and 200% UI,
  source disclosure and true-ratio diameter bars were visually checked. No holes
  or black no-data strips were visible. Portrait windows retain usable controls;
  the close globe can extend beyond the window intentionally.
- Windows build support remains in CMake; this feature has not been compiled or
  executed on Windows in this session.

Reproduce exact loop endpoint comparisons from the project root:

```sh
./build/solar_system_sim --start jupiter --jupiter-paused --jupiter-phase 0 --hide-ui --fullscreen --frames 30 --screenshot build/jupiter-loop-start.ppm
./build/solar_system_sim --start jupiter --jupiter-paused --jupiter-phase 360 --hide-ui --fullscreen --frames 30 --screenshot build/jupiter-loop-end.ppm
cmp build/jupiter-loop-start.ppm build/jupiter-loop-end.ppm
```

Use the same drawable size for both captures. The local event driver and logs are
in `build/check_jupiter_native.py`, `build/jupiter_ui_driver.cpp` and
`build/jupiter-*.log`. Screenshots: [globe](jupiter-globe.png) and
[Red Spot close-up](jupiter-region.png). See [Jupiter manual checks](JUPITER.md#manual-check).


## Jupiter redesign and physical Jovian scale — 2026-09-28

The current renderer replaces the earlier per-face procedural colors with the
native 3600 × 1800 archival Hubble cloud map, Juno-inspired fine cloud structures,
feathered storm circulation, cloud relief shading and subtle geometric facets.
The index-buffer conversion retains 589,824 triangles while reducing mesh storage
to about 11.25 MiB. The comparison panel is removed. The four Galilean moons now
use JPL mean radii and mean orbital distances on Jupiter's physical length scale.

- Release build and all **seven CTest suites** pass. Updated tests validate the
  native asset dimensions, orientation, checksum and trailing-data rejection;
  exact moon radius/distance ratios; orbit-guide alignment; tiny moon centre and
  label picking across window/UI sizes; approach/whole-system framing limits;
  and saved camera/selection after a moon round trip.
- Native SDL/OpenGL tests on Radeon 680M at **2560 × 1600** averaged **110.5 FPS**
  over the 11-second atmosphere check, **113.8 FPS** over the 19-second navigation
  tour, and **115 FPS** over the 9-second system framing tour. These measurements
  include startup and vsync and are specific to this machine.
- Pause captures and reset captures were byte-identical. Phases 0 and 360 were
  byte-identical; mid-loop frames differed. Near-wrap phases were captured too.
  Poles, the longitude seam, the low-detail mesh and 200% fitted UI were rendered
  and visually reviewed. The final polar shading pass reduces bright contour-like
  streaks; it still explicitly represents unsupported source coverage artistically.
- App-local SDL events exercised both new framing buttons and F/J, then opened
  Ganymede and returned with selection retained and system distance restored to
  7.8. The earlier navigation tour visited Io and returned to the detailed Jupiter
  globe with its paused clock, Red Spot selection and camera intact. It also
  exercised guide scrolling, inline notes, time controls and Watch/Follow.
- Mars and Mercury both loaded successfully at 4 ppd in the coexistence tour.
  Their native-grid centre readings remain +5283.25 m and +124.625 m respectively.
  The original standalone Mars project was not edited.
- One preliminary UI tour ended before Mercury finished asynchronous loading;
  its QA timeout was extended and the complete tour then passed. No Mercury
  loading behavior or dataset was changed to satisfy the check.
- No engine, image library or additional runtime dependency was added. Windows
  remains supported by CMake but was not compiled or run in this Linux session.

Current screenshots: [globe](jupiter-globe.png), [Red Spot](jupiter-region.png),
[whole Jovian system](jupiter-system.png), [approach Jupiter](jupiter-system-close.png).
The app-local driver and logs are in `build/check_jupiter_native.py`,
`build/jupiter_ui_driver.cpp`, and `build/jupiter-*.log`. Exact sources, limitations
and manual checks are in [Jupiter documentation](JUPITER.md).


## Jupiter control simplification — 2026-09-28

Both focused and system views now share only 1×, 60×, 600× and 3,600× time rates.
The G shortcut and duplicate right-side Red Spot button are removed; 1 and the
region list retain storm tracking. The focused control panel is one row shorter.
Release build and all seven CTest suites pass, including rate wraparound and
rejection of 21,600×. The CLI also rejects the removed rate with the updated help.
Native focused and Red Spot views were rendered and the compact panel inspected;
README/Jupiter controls and their screenshots were updated.


## Physical solar-system overview — 2026-09-28

The overview now uses one physical conversion (one scene unit = 10 AU) for the
Sun/planet radii and reference orbital distances. Circular guides and angular
placements remain static and are labeled accordingly. Crosshair locators identify
subpixel bodies; they do not enlarge geometry. F/N and matching buttons frame the
whole solar system or the four inner planets.

- Release build and all **seven CTest suites** pass. Navigation checks cover
  Sun/Earth size ratio, Neptune/Earth distance ratio, body-versus-orbit scale,
  all nine scene radii and positions, subpixel visibility/picking, portrait and
  landscape fitting, zoom/clip bounds, framing controls and saved inner-system
  context after opening Mercury and returning.
- The native SDL event tour used both framing shortcuts and both buttons, clicked
  Mercury's floating label in the inner view and Neptune's in the full view,
  opened their local systems and returned. It also opened Jupiter's local system
  and globe, selected the Red Spot with 1 and returned. Final selection was
  Neptune, camera distance 9.5, with Jupiter's paused state and tracking preserved.
- The 17.3-second tour averaged approximately **119 FPS** at 2560 × 1600 on Radeon
  680M with vsync. This is a local observation, not a cross-device guarantee.
- A second native run checked 200% fitted UI in a narrow window, inner framing,
  zoom to the closest allowed Sun-centred camera and Home reset. Screenshots of
  full/inner framing, restored selection, the compact overlay and closest zoom
  were inspected. No additional runtime library or external dataset is required.
- Windows was not compiled or run in this session. Terrain data/renderers and
  the original standalone Mars project were not modified.

Native QA: `build/check_solar_scale.py` (add `--compact` for the second run),
`build/solar_scale_driver.cpp`, and `build/solar-scale-*.log`. Current screenshots:
[full overview](solar-overview.png), [inner planets](solar-inner.png).
[Physical values, primary sources and interpretation](SOLAR_SCALE.md).

## Moon implementation / 28 September 2026

- Release build completed with the existing warning flags; all **8 CTest suites**
  passed, including the new lunar data, polar-slope and navigation tests.
- Downloaded LOLA LDEM_64 V3.1 label/IMG and NASA SVS 2019 8K LROC/LDAM appearance
  map. Download script verified/skipped both existing elevation source files.
  Exact source and prepared hashes are in MOON.md.
- Full-grid conversion/readback passed strict DN-range and checksum validation.
  Independent raw-IMG decoding at the six Apollo sites agrees with the C++
  bilinear sampler within 0.002 m. Apollo 11: −1925.604346 m (sphere datum).
- Native OpenGL on Radeon 680M: full 16 ppd Moon mesh, 33,177,600 triangles,
  64 ppd readouts and 8K appearance loaded successfully. Centre depth/coordinate
  smoke check passed. Also inspected the whole globe in the tall desktop window.
- An app-local SDL event tour visited all 16 destinations using keys and page/row
  clicks; opened/scrolled the guide; toggled relief/palette and replayed the scan;
  returned Moon → Earth → Moon → Earth → Sun. Final retained Moon state was
  Shackleton (index 15), page 3, −89.63°/132.32°/1.1 radii and 1× relief.
  The tour recorded about 119 overall FPS at 2560×1600; this includes loading
  and is not a per-scene performance guarantee.
- Polar QA found streaks from subpixel cylindrical facets. Added two small
  source-derived slope maps for polar lighting, then rechecked Shackleton in
  the elevation palette and in LROC appearance. The geometry/probe are unchanged;
  the streaks are gone. Tested the same location at 200% UI (automatically fitted).
  Paging, inline notes with scrolling controls, guide and probe remain inside
  the usable layout without overlap.
- Native Mars and Mercury regression smoke checks passed with 4 ppd geometry;
  existing source readouts, independent cameras and default settings were retained.
- Jupiter's native render smoke check also passed. A deliberately absent Moon
  grid produced the recoverable data-unavailable view without crashing.
- No native Windows run was performed. PowerShell download/preparation commands
  are supplied and the new app/converter code uses the existing portable APIs.

Review images: [Moon](moon-globe.png), [Apollo 11](moon-apollo11.png),
[polar elevation](moon-polar.png). App-local QA driver, tour log and additional
screenshots remain in ignored build/. Full source documentation: [MOON.md](MOON.md).

## Phobos, Deimos, Europa and Ganymede — 2026-09-29

Added one shared satellite explorer supporting checked NASA textured irregular
meshes and checked USGS faceted image atlases. All 13 CTest targets pass, including
satellite geometry, file validation, source registration, controls, guides,
responsive hit regions and parent navigation. Native OpenGL screenshots were
inspected for each globe and the Mars/Jupiter systems. See
[SATELLITES.md](SATELLITES.md) for source pins, representation limits and checks.


## Saturn and Mars system proportions — 2026-09-29

The release build and all **14 CTest targets** pass. Saturn adds checked NASA
VTAD appearance assets, independent atmosphere controls, an oblate globe,
transparent rings, analytic reciprocal shadows, four regional guides and detailed
system/solar previews. Native Radeon 680M OpenGL renders were inspected at the
north pole, equator and southern hemisphere, with default and low-detail meshes,
and in Saturn's system. Hidden-UI loop phases 0 and 360 produce byte-identical
PPM files. Both assets validate at runtime; malformed headers, corrupted payloads
and trailing bytes are covered by tests.

Mars's system now uses physical reference body sizes and JPL mean orbital
distances on one scale. Irregular model volumes agree with Phobos/Deimos mean
radii within 0.0001 km. Tests verify locator picking, camera limits, whole/approach
framing and saved system state. Native rendering confirms the 1× terrain preview
and uninflated tiny moons with usable labels.

Sources, reproduction, representation limits and review images:
[SATURN.md](SATURN.md), [MARS_SYSTEM.md](MARS_SYSTEM.md).


## Major Saturn moons, physical scale and space backdrop — 2026-09-30

The release build and all **15 CTest targets** pass. Added checked NASA VTAD
explorers for Mimas, Enceladus, Tethys, Dione, Rhea, Titan and Iapetus, with
independent cameras, guides and model-view notes. Titan has a haze/surface toggle.
Saturn, rings and moons share physical sizes and mean orbital distances.

New tests validate source geometry, calibrated volume-equivalent radii, outward
facets, Mimas diffuse-versus-normal texture selection, orbital spacing, catalog
routing, labels, framing, saved camera state, checked-file rejection and Titan
controls. All seven prepared assets match their packaged build/assets copies.
The converter discards near-zero-area polar triangles in six supplied models
without adding topography or altering the original irregular outlines.

Native OpenGL screenshots were inspected for all seven moons, both Titan modes,
the whole/inner Saturn system and ring framing in a tall window. A local SDL event
tour verified F/N/A, framing buttons, T and its button, the field guide, model
views, returning to the system, selection and camera restoration. The adjusted
approach view fits the rings within the tall window.

The shared faint star backdrop was checked with Saturn, moon models, Earth,
Jupiter, Mars terrain and the solar overview. Stars remain sparse, deterministic
camera directions behind opaque bodies; this is an authored sky, not a catalog.
No native Windows run was performed. Sources, limits and review images:
[SATURN_MOONS.md](SATURN_MOONS.md).


## Uranus and Neptune — 2026-09-30

All 16 CTest targets passed, including `outer_worlds`. That target verifies
NASA cloud mesh bounds and axis conversion, physical moon volumes and distances,
tilts, framing, small-body locator picking, selection/camera restoration,
atmosphere pause/speed controls and loop closure. A clean incremental build
completed without warnings.

Native OpenGL review covered Uranus and Neptune globes, physical systems,
F/N/A keys and framing buttons, atmosphere pause and speed buttons, polar
presets, field-guide opening, all five major Uranian moon explorers, Triton,
and return navigation. Proteus's final globe labels explicitly identify its
untextured reference sphere. The opening moon views were adjusted toward the
better-imaged hemisphere, and Miranda and Triton were re-rendered afterward.
The faint sky remained visible in every view.

Reproduce direct views with `build/solar_system_sim --start uranus` or
`--start neptune`; systems use `--start system --body uranus` or `--body neptune`.
`--frames 3 --windowed --screenshot /tmp/review.ppm` captures a native frame.
Body sources, pinned checksums, physical scales and coverage limitations are
in [OUTER_WORLDS.md](OUTER_WORLDS.md).

## Ice giant atmosphere detail — 2026-09-30

Both planets now render 589,824 triangles, up from 96,768, with checked
2048 × 1024 source cloud maps and layered periodic atmosphere shading.
All 16 CTest targets passed. The extended outer-world tests verify the dense
shell's physical ellipsoid, triangle count, atlas dimensions and identity
rejection alongside the existing scale, navigation and clock checks.

Native OpenGL close views were inspected for both planets. Fixed-size,
UI-free captures at loop phases 0, 180 and 360 confirmed changing clouds at
the same planetary orientation and seamless loop closure. The maximum phase
0/360 difference was one RGB8 level for either planet. A 120-frame Neptune
playback run at 1242 × 740 completed at about 105 FPS on the AMD Radeon 680M;
this is a short local check, not a cross-hardware benchmark. The updated globe
review images are linked from [OUTER_WORLDS.md](OUTER_WORLDS.md).


## Selected-body system approaches — 2026-09-30

A and the Approach selected button now move and zoom toward the selected body
within any planet system. The camera eases its centre as well as its distance;
close zoom limits use the approached body's radius, and all system renderers
share an adaptive near clipping distance. Physical sizes and positions remain
unchanged. F, inner framing and Home restore a planet-centred camera; opening a
globe and returning preserve the approached body and camera.

The navigation tests cover every visible body in all eight planet systems,
including smooth movement, physical framing, small-moon clipping, portrait and
landscape centring, close zoom limits, globe returns and overview resets.
Native OpenGL tours checked keyboard and button approaches, F, globe return
and sidebar selection in Mars, Earth, Jupiter, Saturn and Neptune systems.
Deimos and Proteus were inspected at close range without clipping. All 16
CTest targets passed.


## Sun explorer — 2026-09-30

The Sun now has an analytic emissive photosphere, moving granulation, dark
sunspots, flaring active regions, streaming corona and fading plasma ejections.
The close explorer includes a true-relative-size Earth reference; the solar
overview retains the existing physical scale. Field-guide notes distinguish
flares from ejected material and label the palette and activity as authored.

All 16 CTest targets passed. Added checks cover activity pause, speed, frame
independence and closure, the Earth reference radius and common camera depth,
Sun controls, navigation and field-guide routing. Fixed-size native captures at
activity phases 0 and 360 differ by at most one RGB8 level; phase 90 changes the
surface and eruption fronts visibly. A native SDL tour checked all three views,
keyboard and button activity controls, reset, guide opening and overview return.
The 1,195-frame tour averaged about 114 FPS at 1242 × 1508 on the local Radeon
680M; this includes multiple views and asset loading, not a portable benchmark.
See [SUN.md](SUN.md) for controls, sources and review images.


## Organic sunspots — 2026-09-30

Removed the Sun explorer's Earth reference globe, locator and comparison
labels. The size panel retains the numerical diameter comparison, and view 3
is now Full Sun view. The Sun explorer no longer prepares Earth's GPU assets.

Sunspots now form irregular groups with dark umbrae, textured penumbrae and
neighboring pores. Coherent shape warping, periodic drift, changing group tilt
and staggered growth/decay replace fixed circular Gaussian dots. All temporal
terms remain periodic across the existing activity loop.

All 16 CTest targets passed, including a check that the Sun scene contains
only the Sun. Native views were reviewed at full and close distances, with
phase-0/90 captures adjusted for the Sun's rotation to confirm intrinsic
sunspot evolution. Phase-0/360 GPU captures differ by at most one RGB8 level.
The Sun review images and current [explorer notes](SUN.md) were updated.


## Automatic selection approaches — 2026-09-30

Single sidebar or locator clicks now approach the selected body automatically
in planet systems and the solar overview. Keyboard selection follows the same
path. A remains an optional way to reframe the current selection; Enter and
double-click still open the next view. Whole/inner framing and Home reset the
camera centre, and returning from a system or globe preserves the close view.

Solar approaches retain physical sizes and orbital positions, including ring
framing. The render origin is rebased before the camera converts to float to
keep distant planets stable at close range; orbit guides share that translated
origin. The close clipping distance adapts to the selected body's scale.
Focused solar views use the existing Mercury/Mars terrain previews at 1×
relief and initialize Venus's existing atmospheric renderer when needed.

All 16 CTest targets passed. Navigation tests cover automatic approaches to
every visible local-system body and all nine solar bodies, portrait/landscape
centring, picking, physical scale, clipping, saved views and overview resets.
Native sidebar tours used no A key or approach button: they checked Mercury,
Earth, Neptune, Saturn, Venus and Sun in the solar view, plus Phobos, Deimos,
Proteus and Triton in local systems. Inner/whole buttons, Enter, overview
return and restored selection/cameras were also reviewed.


Asteroid/Kuiper belt and dwarf planet verification (30 Sep 2026): clean build; 17/17 CTest checks passed (4.72 s). The new belt test covers one-click POI routing, saved camera returns, physical radius/distance conversion, ellipse perihelion/aphelion, Kirkwood depletion in semimajor axes, all four illustrative Kuiper orbital families, portrait/landscape picking, numerical precision at dwarf approaches, mesh validation, and Haumea's axis ratios. Existing solar/local-system tests remain passing.

Native OpenGL review used a 24-stage SDL UI tour, without pressing A: Sol -> main belt -> Ceres approach -> Ceres globe -> saved belt view -> Sol -> Kuiper belt -> Pluto/Haumea/Eris/Makemake approaches and globes -> whole Kuiper region -> Sol. All assets loaded; no missing-data/invalid-facet errors remained. Reviewed regional, approach, globe and return screenshots at 1242 x 1508. `build/belt_review_driver.cpp` and the `build/outer-qa-belt-*.ppm` captures record the tour. Science and rendering limits are documented in [BELTS.md](BELTS.md).
