# Solar System Sim framework

## Implemented Milestone 1

Sun, eight planets and ten included moons form a compiled catalog of 19 stable
body IDs. Each body records a parent, kind, name, presentation color/style and
optional dataset ID. Physical units are never inferred from display radii or lane
positions. The MOLA descriptor identifies format, path, coordinates, areoid datum,
reference radius, resolution, coverage and provenance. Other bodies have no
surface dataset. A second real dataset can establish the next shared data adapter.

Navigation has Solar, System and Globe views. Planets always open their local
system, even when no moons are included. Moons open globes and return to their
planet's system. Sun opens its globe directly. Mars globe dispatches to the existing
Mars renderer and controller. Other bodies share one 960-triangle sphere; Saturn
adds a simple two-sided ring. Bands are symbolic identifying colors, not textures
or measured atmospheric maps.

One SDL window, OpenGL context, renderer and pixel font serve all views. The new
renderer entry points share existing framebuffer/UI resources. No scene graph,
entity framework, plugin system, JSON parser or additional runtime library is used.

## Scale, precision and transitions

Map positions use hand-authored angles and ordered schematic lanes. The HUD
explicitly labels compressed distances, enlarged bodies and a static layout.
Solar and local systems are independently normalized to order-one display units.
Each globe uses one reference radius; Mars preserves +X at zero east longitude,
+Y north, -Z at 90 east longitude, the 3396 km MOLA display sphere, and scientifically
unexaggerated heights above the areoid.

New camera angles, distances and targets use CPU doubles. Only bounded local
positions/bases become GPU floats. Portrait views widen the vertical field of view to preserve horizontal fit;
Saturn starts farther away to include its rings. Current near/far planes remain 0.005/20 local
units; no solar-system-sized physical coordinates enter the Mars depth buffer.
Future physical coordinates must subtract a local origin in double precision
before conversion. Additional terrain/tangent-frame precision work belongs with
actual higher-resolution data.

A 0.44-second transition eases toward a selected body, fades across the frame
change, then settles into the target view. Up transitions enlarge the departure
view's camera distance before restoring its parent. Navigation input is held
while transitioning. Wheel zoom never silently changes levels.

At most 39 small view-state slots store current and target cameras plus selection;
repeated visits do not create an unbounded history. Each view restores its saved
state. Mars keeps its own atlas/flight poses, relief, landmark selection, orbital
phase/settings and weather state. Inactive Mars and navigation transitions do not
advance its clocks. Settings currently last only for the session.

## Loading and resources

The overview needs no MOLA data. First entry into Mars starts one SDL worker to
load and construct its mesh. GL upload stays on the main thread and may briefly
stall presentation. There is no streaming scheduler. CPU mesh memory is freed
after upload; the CPU source heights and one Mars GPU mesh remain cached until
exit. A worker in progress is joined before the renderer/context are destroyed.
Returning while a load is in progress reuses that operation.

Default Mars resolution remains 0.0625 degrees: 33,177,600 triangles and roughly
633 MiB of GPU geometry, with the previous lower-detail launch options preserved.
Startup preparation overlaps CPU and GPU geometry. Placeholder mesh storage is
under 50 KiB. Other planets never allocate Mars terrain buffers. Failed file
validation/allocation leaves the shell operational, clears failed resources, logs
the cause and allows retry or Up navigation.

## Later milestones

1. Add verified body datasets individually, including surface reference shape,
   coordinate conventions, datum, coverage, sample representation and provenance.
   Follow the inherited Mars tile/LOD design for bounded higher-resolution terrain,
   seam/pole handling and stitched edges. Do not manufacture fine measured relief.
2. Add sourced astronomical positions, a shared time, body orientations and
   Sun-relative lighting. These are independent from the existing Mars two-body
   satellite camera and from the schematic navigation layout.
3. Add detailed moon surfaces, local cameras, landmarks and optional simulation
   only where source data supports the intended scale.

One body's identity and data support all its representations. Later levels of
detail should vary with screen size; they should preserve orientation, geographic
features and palette across transitions. Comparable visual care does not require
equal triangle counts or equal terrain coverage for every body.

## Detailed Mercury extension

Mercury now joins Mars as a measured globe: MESSENGER MDIS global DEM V02,
64 ppd source readouts and 16 ppd default geometry. The lightweight overview body
is retained. Independent terrain allocations and surface-view state survive
navigation. See [Mercury implementation, exact sources and limits](MERCURY.md).
Other bodies remain placeholders; tiled LOD and astronomical motion remain later work.
