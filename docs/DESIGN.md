> This document describes the inherited Mars subsystem. For the implemented
> solar-system framework, see [SOLAR_SYSTEM_PLAN.md](SOLAR_SYSTEM_PLAN.md).

# Implementation and next milestones

## Milestone 1 — implemented, deliberately bounded

The code has one height grid, one camera, one renderer, a small weather state and a two-body orbit. It has no entity system,
plugin system, job scheduler, asset database, or simulation abstraction. The
three build checkpoints in the README correspond to the implemented increments:
data import/sampling, native globe, then HUD/destinations and verification.

### Geometry and camera

The world is right-handed: **+X = equator/0°E**, **+Y = north**,
**−Z = equator/90°E**. Angles in geographic APIs are degrees. For latitude φ
and east longitude λ, the unit direction is:

```text
d = (cos φ cos λ, sin φ, −cos φ sin λ)
position = d * (1 + elevation_metres * exaggeration / 3396000)
```

The display radius is one world unit, corresponding to the **3396 km map radius
in the label**. Draping areoid-relative height over this sphere is a deliberate
visualization approximation. Even at 1× it is not an exact planetary-radius
reconstruction: that would require the radius product or spatially varying
areoid as well. Scientific readouts remain the unmodified topographic datum.

The default indexed sphere has **5760 columns × 2880 cell-centred rings** and two
shared poles: **16,588,802 vertices / 33,177,600 triangles**. Every non-pole vertex
uses an original signed MOLA sample directly, at `(89.96875 - row/16,
0.03125 + column/16)` degrees. This is 4× finer linear spacing than the previous
0.25° mesh (approximately 16× the triangles), using the same source file.

| Option | Spacing | Triangles | GPU vertex/index bytes |
| --- | --- | ---: | ---: |
| Default / `--detail 16` | 0.0625° | 33,177,600 | 663,552,032 |
| `--detail 8` | 0.125° | 8,294,400 | 165,888,032 |
| `--detail 4` | 0.25° | 2,073,600 | 41,472,032 |
| `--coarse` | 0.5° | 518,400 | 10,368,032 |

Lower-detail meshes sample the source at their own cell centres. Triangles share
indices across the longitude seam and both pole fans. There are no mixed-resolution
boundaries. Height exaggeration remains a shader uniform; buffers are not rebuilt.

Indices are grouped into 2664 fixed-resolution latitude/longitude draw batches,
roughly 5° across (including thin pole fans). Each retains conservative angular and
height bounds. CPU culling first tests occlusion by an inscribed sphere below all
terrain, including a correction for straight triangle chords. It then tests a
bounding sphere against the six view-frustum planes. Visible ranges are submitted
with `glMultiDrawElements`; all batches reference the same vertex buffer. This
reduces vertex work, especially in low orbit, without cracks or LOD transitions.
The `--no-cull` diagnostic flag permits pixel-for-pixel comparisons. **This is
visibility culling, not streaming or adaptive LOD**: the full mesh remains resident.

The atlas camera keeps north up, clamps latitude to ±90°, wraps longitude, and eases
along the short longitude arc. Distance is limited to 1.085–8 display radii,
with a closer 2.68-radius starting view; the near plane is 0.005. At 10×, the documented maximum terrain radius is about
1.0624, leaving clearance even at the closest camera setting. There is no
walking camera or claim of additional data when zooming in.

Pointer picking reads a depth pixel **before** the HUD is drawn and unprojects
it using the same camera basis and projection as the vertex shader. Its radial
direction yields latitude/longitude, then the full source grid supplies the
unexaggerated, bilinearly interpolated metre readout. Outside the globe, the
readout explicitly changes to the camera-centre coordinate. The nearest
rendered pixel is used; finite raster resolution limits picking precision.

Satellite mode supplies an independent eye and orthonormal view basis, shared by
terrain projection, weather ray intersections, marker projection and CPU depth
unprojection. Flight uses 1× relief; atlas settings are restored when leaving it.
Orbit dynamics, camera conventions and scientific limits are in [ORBIT.md](ORBIT.md).

### Rendering, style and cost

Eight warm hypsometric colours, triangle normals from derivatives of local affine coordinates,
stepped directional lighting, a dark neutral background, original 5×7 text,
an industrial instrument layout, and small numbered markers form the visual style. Lighting is
attached to the camera for readable relief, not a simulated Sun. Terrain fills the drawable at native pixel resolution, with no framed viewport or
letterboxing. Small translucent HUD panels occupy the corners. Text uses adjustable
pixel-snapped scaling; Tab hides the entire HUD and markers. The app starts maximized;
F11 toggles SDL borderless fullscreen, and Escape leaves fullscreen before quitting.
Resizes recreate only the colour/depth attachments, update aspect ratio and HUD
anchors, and clear drag state so stale coordinates cannot jump the camera.
No game assets or existing game's interface have been copied.

The persistent CPU height grid is **31.64 MiB**. Default GPU terrain buffers use
**632.81 MiB**; the table above lists lower-detail alternatives. Colour/depth
attachments use about 8 bytes per drawable pixel (31.25 MiB at 2560×1600).
The CPU mesh is freed after upload, but startup temporarily overlaps about 665 MiB
of CPU grid/mesh storage and 633 MiB of GPU mesh storage, plus attachments and driver
overhead: allow roughly 1.3 GiB or more during upload. Culling reduces draw work,
not residency. This cost is explicit because the requested 4× finer spacing needs
approximately 16× more geometry. No higher-resolution source product is loaded.

Each frame submits visible terrain batches, up to two procedural weather layers
and one HUD draw. Depth reads are synchronous: one pointer probe, a centre probe
when needed in satellite mode, and up to four markers. Vsync is requested; without
it the app limits itself to about 60 FPS. Minimized windows suspend both clocks.

### Industrial UI and scale

`ui.hpp` shares rectangles between drawing and input. The relative UI scale ranges
from 0.75 to 2 in quarter steps. The automatic base is the largest integer that
fits 1000×650 logical pixels (at least 1). The final scale is fitted to at least
760×480 logical pixels, with a minimum one-native-pixel glyph stroke when the
window permits. `FIT` identifies clamping. The UI shader rounds vertices to native
pixel edges; terrain rendering and depth precision are independent of UI scale.
Mouse/window coordinates are converted using the same scale. A scale change
reprojects the pointer and cancels any drag. Settings apply for the current session;
`--ui-scale` supplies a startup value. `Tab` hides UI/markers, leaving weather visible.

Visual references reviewed on 2026-09-26:

- [Nous Research](https://nousresearch.com/) and its earlier
  [homepage screenshot](https://image.moge.ai/screen/nous-research.jpg): spare
  typographic hierarchy, separated rows, technical labels and generous empty space.
- [Bungie's Marathon starter guide](https://www.bungie.net/7/en/News/Article/marathon_starter_tips),
  specifically the first Codex screenshot: charcoal/off-white contrast, a limited
  high-visibility accent and compact instrument-like controls.

The result is an original layout using the project's own glyphs and geometric
marks. No reference assets, logos, game fonts or interface artwork ship with it.

### Illustrative ice and weather

MOLA supplies surface heights, not ice classification or atmospheric observations.
The UI labels these overlays **ILLUSTRATIVE / NOT OBSERVED**. Clean topography (`T`)
suppresses ice, cloud and dust without changing the stored layer selections or the
source grid. The legend identifies when visual layers supplement the base height
palette. CPU height sampling, geometry displacement and scientific readouts are
unchanged. Ice adds no thickness: source topography already describes the surface.

- **Ice:** two fixed, approximate residual-cap masks in the terrain fragment shader.
  The rough northern boundary is near 81°N, the smaller southern one near 84°S,
  with irregular edges from 3D noise. These are artistic footprint choices, not
  mapped boundaries. Warm-white shading softly retains terrain facets. No annual
  CO₂ growth/retreat or distinct compositional map is simulated.
- **Clouds:** sparse, translucent, stretched noise on a sphere, advected and slowly
  reshaped on a repeating visual clock. Maximum alpha is 0.19 before latitude and
  limb attenuation; most of the terrain remains directly visible. The pattern is
  sampled in Cartesian unit directions, with no UV seam or pole singularity.
- **Dust:** one local/regional patch at a time, a smooth growth/fade envelope,
  expanding radius and drift. Seeded xorshift state makes the schedule repeatable;
  event boundaries are processed independently of frame rate. A storm lasts 60–120
  seconds, followed by 240–420 quiet seconds; initial wait is also 240–420 seconds.
  These are accelerated presentation timings, not Martian statistics. Preview starts
  a storm at 35% of its lifetime near the view. Pause freezes both drift and lifecycle.
  Minimized windows suspend animation; a long frame contributes at most 0.25 seconds.

Each weather layer draws a fullscreen triangle and analytically intersects the
front of its shell. Dust draws first, clouds second. The shell radii are
`1 + 21171 * exaggeration / 3396000 + clearance`, using the validated source maximum;
clearances are 0.004 (dust) and 0.010 (cloud). At 10× the outer shell is about 1.07234,
below the closest camera at 1.085 with a 0.005 near plane. These are **display
clearances**, not claimed atmospheric heights. They keep weather clear of mountains
without a separate terrain mesh. Opacity fades inside the minimum terrain silhouette
(using the validated −8177 m bound), preventing a detached halo, and near the camera.
The coarse source still limits close-up detail; weather adds no measured terrain.

Render order is terrain → weather → HUD. Weather depth-tests with **depth writes
disabled** and restores GL state. Picking uses terrain depth; bounded smoke runs also
compare an off-centre pick before/after weather. There is no shadow map, particle
system, fluid solver, physical scattering, or global storm simulation.

Scientific context (not input maps or a calibration of the procedural effects):

- [NASA/JPL: polar caps](https://www.jpl.nasa.gov/news/nasa-radar-finds-ice-age-record-in-mars-polar-cap/):
  residual north water ice, south water ice with a thin remaining CO₂ cover, and
  seasonal CO₂ advance/retreat. Our fixed masks approximate only their visual presence.
- [NASA: clouds sailing overhead](https://science.nasa.gov/resource/clouds-sailing-overhead-on-mars-unenhanced/):
  wispy, drifting clouds, likely water-ice crystals. This motivates thin moving wisps.
- [NASA: Martian dust storms](https://science.nasa.gov/solar-system/10-things-dust-in-the-wind-on-mars-and-well-beyond/):
  local storms can grow into regional events, with occasional planet-wide storms.
  Only a bounded regional visual event is implemented here.

## Milestone 2 — planned, not implemented

1. Add an **offline tile packer** for specifically requested 32/64/128 ppd PDS
   products, validating each accompanying label. Keep the 16 ppd global grid as
   a fallback. Do not download a world-sized 128 ppd grid on startup. Repack source
   images into small independently readable tiles; for example 128×128 cells
   plus shared border samples, with conservative min/max height bounds.
2. Select a quadtree using projected geometric error and hysteresis. Cull by
   frustum and horizon. Load only selected tiles from local storage, using a
   small worker queue; perform OpenGL upload on the main thread. Keep the parent
   visible until all replacement children and their edge dependencies are ready.
3. Start with hard budgets of **128 MiB total CPU terrain cache** (including the
   16 ppd fallback), **64 MiB GPU terrain cache**, and **16 MiB in-flight IO**.
   Account for bytes before admission. Pin visible tiles and ancestors, evict
   other tiles by LRU, cancel obsolete requests, and fall back to a coarser level
   rather than exceeding a budget. Limit upload work per frame.
4. Prevent cracks by enforcing at most **one LOD difference between neighbours**,
   using a common global integer sample lattice and shared edge samples, and
   selecting stitch index patterns on fine/coarse edges. Fine-only boundary
   vertices must lie on the **actual Cartesian coarse edge**, not merely at the
   same interpolated radial height: the latter leaves gaps on a curved globe.
   Lock/morph boundary positions consistently with the coarse neighbour. Morph
   interior vertices from parent positions to reduce popping. Small skirts may
   mask brief loading transitions but are not the primary crack fix.
5. Treat longitude adjacency modulo 360°; two sides of the seam refer to the same
   edge/sample IDs. Use shared pole fans for coarse coverage. The cylindrical
   128 ppd products stop at ±88°: retain the coarse polar coverage until dedicated
   polar products have been label-validated and resampled offline into a common
   polar mesh. Stitch their collar to the global grid with shared positions.
6. Verify adversarial cases: oscillating zoom thresholds, rapid camera turns,
   cancelled reads, delayed children, exhausted caches, seam crossings, both
   poles, and large exaggeration. Measure frame time, residency, and projected
   error. Add MOLA counts for coverage cues; grid density alone is not confidence.

This adds resolution where the camera needs it without turning Milestone 1 into
a geospatial framework. Render tiles can later be replaced independently of the
sampler and input layer.

## Milestone 3 — further exploration and simulation

A small idealized satellite orbit is now implemented at the user’s request.
Future additions include local tangent-frame surface cameras, annotated landmarks, distances/profiles, and
small simulation experiments only when their input data and units are defined.
For close-up navigation, choose higher-resolution terrain with documented
coverage and datum, maintain a floating local origin if needed, and display the
resolution/uncertainty. Keep simulation state separate from source topography.
Do not invent metre-scale terrain and present it as MOLA measurements.

Fullscreen handling follows [SDL_SetWindowFullscreen](https://wiki.libsdl.org/SDL3/SDL_SetWindowFullscreen).
The transition can be asynchronous, so drawable dimensions are refreshed each
frame and on SDL resize events. The HUD displays the actual fullscreen flag.
The terrain vertex shader and CPU depth picking share camera projection constants.
