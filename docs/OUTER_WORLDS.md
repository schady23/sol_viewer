# Uranus and Neptune

Open Uranus or Neptune from the solar overview, then open the planet or an
included moon. Direct launches use `--start uranus`, `--start neptune`, or a
moon name. `--start system --body uranus` and `--body neptune` open local systems.

## Controls

- Drag / arrow keys: orbit the camera; wheel / +/-: approach or retreat.
- 1–4: model inspection presets; Home: restore the camera.
- L: atlas lighting or unlit appearance in a globe.
- Space: pause the atmosphere; S: cycle 1 / 60 / 600 / 3600 times speed.
- R: reset atmospheric time and retry an unavailable source asset.
- F / N / A in a system: whole system, inner system, approach the selected body.
- I: body field guide; Enter: open selection; Alt+Left / Up: return.

Each planet has an independent clock, camera, lighting setting and selection.
Moons preserve their own cameras. Returning restores the system camera and
selected moon. The faint shared space backdrop follows camera direction.

## Physical proportions

Both local systems use **1,000,000 km per scene unit** for body radii, rings and
orbital distances. Moon visualization meshes are calibrated by signed volume to
JPL mean radii. Tiny locator crosses are selection aids, not enlarged bodies.
The solar overview keeps its existing common physical scale.

| Body | Radius (km) | Mean orbital distance from planet centre (km) |
| --- | ---: | ---: |
| Uranus | 25,559 equatorial / 24,973 polar | — |
| Miranda | 235.8 | 129,846 |
| Ariel | 578.9 | 190,929 |
| Umbriel | 584.7 | 265,986 |
| Titania | 788.9 | 436,298 |
| Oberon | 761.4 | 583,511 |
| Neptune | 24,764 equatorial / 24,341 polar | — |
| Proteus | 208.0 | 117,600 |
| Triton | 1,352.6 | 354,800 |

Planet radii, flattening, axial tilt and magnetic reference rotation periods:
[NASA Uranus fact sheet](https://nssdc.gsfc.nasa.gov/planetary/factsheet/uranusfact.html),
[NASA Neptune fact sheet](https://nssdc.gsfc.nasa.gov/planetary/factsheet/neptunefact.html).
Moon mean radii: [JPL physical parameters](https://ssd.jpl.nasa.gov/sats/phys_par/sep.html).
Mean semimajor axes and inclinations: [JPL mean elements](https://ssd.jpl.nasa.gov/sats/elem/sep.html),
URA182 and NEP097. Checked 2026-09-30.

Uranus's cloud ellipsoid and ring plane are tilted 97.77 degrees to the authored
orbital-plane normal; Neptune uses 28.32 degrees. Uranus rotates about that tilted
axis, naturally giving retrograde rotation relative to the overview plane.
The major moon lanes follow their planet's tilted equator, with representative
inclinations (Miranda 4.4 degrees; Triton 157.3 degrees). Triton's retrograde
inclination is shown by its guide plane and explained in the field guide.
**Orbital phases, node directions and pole azimuths are illustrative**: these are
static circular mean-distance layouts, not an epoch-specific ephemeris. Small
Laplace-plane offsets, orbital eccentricity and precession are not simulated.

## Atmospheres and rings

NASA VTAD source models supply the archival cloud colors and large structures.
The cloud converter reprojects the original UV atlas to a checked **2048 × 1024
RGB map**, applying the polar-axis convention and removing exporter offsets.
The runtime renders an analytic 1-bar ellipsoid with **589,824 triangles per
planet**, about 6.1 times the previous 96,768. The denser shell improves the
silhouette at close range while retaining accurate flattening and physical scale.

Mipmapped source imagery drifts through latitude-dependent cloud layers, with
seeded three-dimensional noise adding folds, fine wisps and restrained methane
cloud streaks. Smooth atmospheric lighting, faint facets and limb tint follow
Jupiter's visual treatment. Uranus stays pale cyan with restrained detail;
Neptune uses stronger cloud structure and a softened blue palette.
Colors and motion are artistic, not calibrated natural-color
measurements, current weather, wind retrievals or forecasts. Voyager-era storms
in the source are archival. Each loop lasts ten magnetic reference rotations
(17.24 hours for Uranus and 16.11 hours for Neptune); these periods are not the
speed of every cloud. Every moving layer completes an integer number of turns
per loop, so both appearance and motion close smoothly at the boundary.

Ring radii and representative widths follow the NASA
[Uranian rings](https://nssdc.gsfc.nasa.gov/planetary/factsheet/uranringfact.html) and
[Neptunian rings](https://nssdc.gsfc.nasa.gov/planetary/factsheet/nepringfact.html)
tables. Uranus includes the listed narrow rings plus diffuse outer annuli.
Neptune includes Galle, LeVerrier, Lassell, Arago and Adams. Lassell spans
53,200–57,200 km, rather than placing its 4,000 km width around its inner edge.
Annuli have physical widths, transparent compositing, planet/body occlusion and
no depth writes. Very narrow rings can occupy less than a pixel. Brightness is
illustrative; ring eccentricity, thickness, dust scattering, shadows and the
Adams arcs are not reconstructed. These are dark, faint rings rather than
Saturn-like broad bright bands.

## Moon coverage and source limits

Miranda, Ariel, Umbriel, Titania, Oberon and Triton retain NASA's original supplied
UV imagery on approximate visualization spheres. Images include baked lighting,
mixed resolution and incomplete coverage. Smooth or dark areas do not establish
unobserved terrain. Presets use supplied model axes, not surveyed geographic
landmarks. No measured terrain elevation is reported.

Proteus has an **untextured mean-radius reference sphere**, clearly identified in
its HUD, guide and catalog. This is not a NASA shape model or a fabricated global
image map; it does not reproduce Proteus's irregular, cratered shape. Its radius
and mean orbital distance are physical. See
[NASA's Proteus imagery and overview](https://science.nasa.gov/neptune/moons/proteus/).
Neptune's much more distant Nereid and small remaining moons are not included.

## Pinned assets and reproduction

Credit: NASA Visualization Technology Applications and Development (VTAD).
All eight original GLBs were downloaded from their NASA resource pages.

| Body | Original GLB | Original SHA256 |
| --- | --- | --- |
| Uranus | [NASA model](https://assets.science.nasa.gov/content/dam/science/psd/solar/2023/09/u/Uranus_1_51118.glb) | `9c56f96bbfa82d923479c3dd83a00c35238a36addcfbcc61916f1377fe586bd5` |
| Neptune | [NASA model](https://assets.science.nasa.gov/content/dam/science/psd/solar/2023/09/n/Neptune_1_49528.glb) | `2fc3af08d56ed429b9cc56b95d151102aa5807ea958e7cb8806a42e12a699ba8` |
| Miranda | [NASA model](https://assets.science.nasa.gov/content/dam/science/psd/solar/2023/09/m/Miranda_1_472.glb) | `5a4d24c5fb237ae6d21d1a58857efb32fd0e53162754f17e4156618a541c4c57` |
| Ariel | [NASA model](https://assets.science.nasa.gov/content/dam/science/psd/solar/2023/09/a/Ariel_1_1158.glb) | `b7a5cd8ad3d48687cc3162861971a76a19fe9fad4726563e24b6cc917e9bd858` |
| Umbriel | [NASA model](https://assets.science.nasa.gov/content/dam/science/psd/solar/2023/09/u/Umbriel_1_1169.glb) | `b523de7d1406289d540369c635d0b13576460c1fa85a885352afc71c3f118d71` |
| Titania | [NASA model](https://assets.science.nasa.gov/content/dam/science/psd/solar/2023/09/t/Titania_1_1577.glb) | `08556622082b283f2f52c9214636c22acf27300889374dd37197f11a4448324c` |
| Oberon | [NASA model](https://assets.science.nasa.gov/content/dam/science/psd/solar/2023/09/o/Oberon_1_1523.glb) | `7b42c417e523db2a22c744cd471587c26f07581564dae7c3b0c20986eea93add` |
| Triton | [NASA model](https://assets.science.nasa.gov/content/dam/science/psd/solar/2023/09/t/Triton_1_2707.glb) | `a25df7ee7fe8679e3da8cca7f1a09ec7241ad5e0c7ad97a39f057b8c1a0043ce` |

`tools/prepare_outer_worlds.py BODY ORIGINAL.glb NEW_OUTPUT.mesh` verifies the
pinned SHA256 before baking. It uses the checked common moon converter, follows
the material's base-color texture, preserves source UV islands and splits faces
planarly. Only float32 pole slivers are removed. Planet preparation also verifies
the node rotation before its explicit axis conversion. Outputs refuse overwrite.
Python 3 and ImageMagick are offline build-time requirements, not app dependencies.

`tools/prepare_ice_clouds.py BODY ORIGINAL.glb NEW_OUTPUT.rgb` checks the same
pinned SHA256 and follows the material's original base-color texture and UVs.
The new grid uses parametric latitude, north to south, and positive-east
longitude starting at column zero. Both prepared maps interpolate 42 isolated
south-polar seam pixels (0.002% of the map) from mapped neighbors within three
pixels. Larger or disconnected coverage gaps are rejected. The RGB format is
`BODY_CLOUD_RGB 1 2048 1024 FNV1A64`, then RGB8 bytes; runtime checks identity,
dimensions, length and checksum. No new source topography is inferred.

`tools/prepare_proteus_reference.py NEW_OUTPUT.mesh` creates the explicitly
untextured reference sphere. Its finite-mesh volume correction is recorded in
`catalog.cpp`; it is not an observed Proteus shape.

Runtime format: `BODY_MESH 1 VERTEX_COUNT FNV1A64` followed by little-endian
float32 XYZ and RGBA8 per vertex. The loader checks identity, count, byte length,
checksum, finite normalized vertices, outward triangle winding, opacity and
flat face colors. Missing/corrupt files produce a labeled fallback with R retry.
The app packages all required assets and performs no runtime downloads.

## Verification

`outer_worlds_tests` checks the pinned cloud meshes, dense shell triangle count,
checked cloud atlas identity and dimensions, physical flattening, signed
moon volumes, distances, tilted presets and guides, locator picking, camera and
selection restoration, framing, loop closure, pause and frame-rate-independent
time. Existing regression targets also run. Native screenshots and a desktop
tour check both systems, every supplied outer moon, globe presets, guide opening,
atmosphere buttons and return navigation. See `docs/VERIFICATION.md`.

For reproducible loop frames, use `--start uranus` or `--start neptune` with
`--ice-paused --ice-phase 0..360 --distance 2.8`. Phase 180 returns the planet
to the same orientation after five rotations while changing its cloud flow;
phase 360 closes the full loop. Native GPU comparisons at phases 0 and 360
differ by at most one RGB8 level, with no visible seam.


Reviewed native views: [Uranus globe](uranus-globe.png),
[Uranus system](uranus-system.png), [Neptune globe](neptune-globe.png),
[Neptune system](neptune-system.png), [Miranda](miranda-globe.png) and
[Triton](triton-globe.png).
