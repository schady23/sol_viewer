# Jupiter: atmospheric globe and Jovian system

Jupiter combines a native-resolution archival cloud map with Juno-inspired fine
filaments, folded belt edges, blue-gray cloud gaps, storm circulation and subtle
cloud relief. A dense triangular ellipsoid retains faceted lighting. The atmosphere
runs through a seamless artistic loop; it is not observed evolving weather or a
forecast. The globe has no measured terrain or solid landing surface.

The Jovian system uses one physical scale for Jupiter, its four included moons and
their mean orbital distances. [Io](IO.md) and [Callisto](CALLISTO.md) have faceted Voyager/Galileo surface atlases;
Europa and Ganymede have tinted USGS archival surface atlases; see [SATELLITES.md](SATELLITES.md).

## Run and controls

The prepared map is bundled in `assets/jupiter2015.rgb` and copied beside the
executable by CMake. No download occurs at launch. SDL3 is the only third-party runtime
library beyond the platform's OpenGL implementation; this redesign adds no dependency.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/solar_system_sim --start jupiter --fullscreen
./build/solar_system_sim --start system --body jupiter --fullscreen
```

On Windows, use the existing SDL3/CMake setup, build with
`cmake --build build --config Release`, and run
`build\Release\solar_system_sim.exe --start jupiter --fullscreen`.

| Control | Action |
| --- | --- |
| Drag; wheel / +/- | Orbit; zoom |
| Space / Play-Pause | Freeze or resume clouds and rotation together |
| S / time buttons | 1×, 60×, 600×, 3,600× model time |
| V / camera-frame button (globe) | Follow Rotation or Watch Rotation |
| 1–4 / region list | 1 tracks the Red Spot; 2–4 visit the equatorial zone/belts |
| F / Frame whole system (system) | Frame the four moon orbits |
| J / Approach Jupiter (system) | Move close to Jupiter on the same scale |
| R / Reset Atmosphere | Set the cloud and rotation clock to zero |
| Home | Reset the current camera, retaining atmosphere time |
| I / Field Guide | Open planet information on the left |
| Region selection | Expand its paragraph below the region list |
| Alt+Left / Up / breadcrumbs | Return to the parent view |
| F11; Tab; [ / ] | Fullscreen; hide UI; change UI scale |

The loop spans ten reference rotations: **99.25 Earth hours**, or **99.25 real
seconds** at the default 3,600× speed. Pattern speeds are authored, not calibrated
wind measurements. `--jupiter-rate 3600`, `--jupiter-paused`, `--jupiter-low`, and
`--jupiter-phase 0..360` support inspection; phase 360 closes onto phase 0.
`--landmark 1` opens the Red Spot. Generic `--lat` / `--lon` arguments are
planetocentric latitude / east-positive longitude. The cloud readout explicitly
converts to **planetographic** latitude.

Follow Rotation fixes the camera in the rotating System III frame. Watch Rotation
includes the 9 h 55 min 30 s reference rotation. Track Red Spot follows the authored
anchor; dragging leaves tracking. Switching frames preserves the pose. Reset
preserves speed, pause and viewing direction. Navigation retains camera, selection
and time. The clock stops outside Jupiter's system/globe, during transitions and
while minimized.

## Juno visual references

These images informed the color, fine structures and shading; their pixels are
**not** pasted onto the globe or included as runtime assets:

- [Jupiter's Colorful Cloud Belts / PIA21974](https://science.nasa.gov/photojournal/jupiters-colorful-cloud-belts/),
  Juno's 16 December 2017 flyby. The enhanced view shows intertwined pale ribbons,
  rust belts and dark blue-gray gaps. Credit: NASA/JPL-Caltech/SwRI/MSSS;
  processing Kevin M. Gill (CC BY).
  [Reference image](https://assets.science.nasa.gov/dynamicimage/assets/science/psd/photojournal/pia/pia21/pia21974/PIA21974.jpg).
- [Juno Sees Turbulence in Jupiter's Atmosphere / PIA26595](https://science.nasa.gov/photojournal/juno-sees-turbulence-in-jupiters-atmosphere/),
  28 January 2025, flyby 69. Its folded fronts and small eddies informed the nested
  filament fields. Credit: NASA/JPL-Caltech/SwRI/MSSS; processing Jackie Branc (CC BY).
  [Reference image](https://assets.science.nasa.gov/dynamicimage/assets/science/psd/photojournal/pia/pia26/pia26595/PIA26595.jpg).
- [Great Red Spot Rotation Animation / PIA22178](https://science.nasa.gov/resource/great-red-spot-rotation-animation/)
  provides a reference for depicting storm circulation through animation.

This is an artistic interpretation of enhanced imagery, not calibrated natural
color or a reconstruction of the Juno observation geometry.

## Exact source and preparation

The large cloud structures come from the global Hubble map, which supplies complete
longitude coverage that the selected Juno close-up references do not:

- [NASA SVS: New Hubble Maps of Jupiter](https://svs.gsfc.nasa.gov/12021/).
- Exact source:
  [Hubble_Jupiter_color_global_map_2015a.tif](https://svs.gsfc.nasa.gov/vis/a010000/a012000/a012021/Hubble_Jupiter_color_global_map_2015a.tif).
- Credit: NASA, ESA, A. Simon (NASA GSFC), M. Wong (UC Berkeley), G. Orton
  (JPL-Caltech). SVS production: NASA Goddard Space Flight Center / STScI.
- Source SHA-256:
  `c3b915227ef88899a07a2b62584d9303d77d3491b9bb7de8824fb6df4a52f7b2`.
- Interpretation: Simon, Wong & Orton (2015), *First Results from the Hubble OPAL
  Program: Jupiter in 2015*, [DOI 10.1088/0004-637X/812/1/55](https://doi.org/10.1088/0004-637X/812/1/55),
  [author paper](https://esahubble.org/static/archives/releases/science_papers/heic1522a.pdf),
  sections 2–3, figures 1, 2 and 4.

The **3,600 × 1,800** map combines 631/502/395-nm channels from rotation 1 on
19 January 2015. Rows run north to south in planetographic latitude; longitude
increases eastward, equivalent to decreasing System III west longitude. Coverage
ends at ±79.9°. The native 0.1° grid corresponds to about 125 km per equatorial
pixel; finer shader details are invented visual structure, not additional observations.

Python 3 and ImageMagick 7 are needed **only to regenerate** the bundled file:

```sh
mkdir -p data/source/jupiter
curl --fail --location --output data/source/jupiter/Hubble_Jupiter_color_global_map_2015a.tif \
  https://svs.gsfc.nasa.gov/vis/a010000/a012000/a012021/Hubble_Jupiter_color_global_map_2015a.tif
python3 tools/prepare_jupiter.py \
  data/source/jupiter/Hubble_Jupiter_color_global_map_2015a.tif \
  data/source/jupiter/jupiter2015-prepared.rgb
```

In PowerShell, use `New-Item -ItemType Directory -Force data/source/jupiter`,
`curl.exe`, and `py -3 tools/prepare_jupiter.py SOURCE OUTPUT`, on single lines.
The converter refuses a different source checksum or an existing output. Verify its
result before replacing the bundled asset, then rebuild to copy assets.

Format: `JUPITER_RGB 2 3600 1800 CHECKSUM\n`, then exactly **19,440,000 RGB8 bytes**.
CHECKSUM is FNV-1a-64 over the payload (offset 14695981039346656037, prime
1099511628211), written as 16 hex digits. Bundled payload: `41148b72cea29df5`.
The loader checks version, dimensions, byte count and checksum; it also accepts the
older 2048 × 1024 version-1 format. The format is byte-order independent. It stores
color, not elevation. Native resolution is preserved without downsampling.

## Rendering and seamless motion

Equatorial/polar radii are 71,492 / 66,854 km at the 1-bar reference level. The
ellipsoid uses +Y north, +X zero longitude and -Z 90° east. For axis ratio q,
`tan(graphic latitude) = tan(centric latitude) / q²`. Picking intersects the actual
ellipsoid, converts latitude, and reports no height.

The native map is mipmapped for distant views. Fine cloud fields are sampled per
fragment, independently of triangle density, using a small deterministic 3D noise
volume. Domain warping folds elongated filaments into the belts. Subtle derivative
shading suggests cloud relief; it does not displace measured geometry. Face normals
retain restrained facets, with ivory, ochre, rust and blue-gray grading and soft limb
haze. Readability lighting follows the camera rather than the Sun.

A sparse, rounded approximation to the OPAL 2015 wind profile blends opposing
pattern directions. It does not set physically calibrated pattern speeds. The
Red Spot's source anchor is approximately 22.4°S, 233.2°E. Its source-map structure
circulates inside a feathered ellipse at one and two turns per loop; fine tracers
co-rotate within it. Real storm drift and changing morphology are outside this model.

All time dependence uses integer phase harmonics, closed rotations, or sine/cosine
paths. Values and velocities match at loop closure. Longitude-dependent fields
also close at 0°/360°. Ten whole-planet rotations fit the cloud loop, so system
and Watch Rotation views repeat too. Each frame evaluates the original source:
there is no accumulated texture diffusion, catch-up simulation or reset crossfade.

Unsupported polar rows are blended from covered rows into a latitude-average tone,
then into illustrative polar filaments. The readout labels polar clouds as
illustrative beyond 79.9°. This does not reconstruct Juno's observed polar cyclone
arrangement or claim cloud-height measurements.

The default **589,824-triangle** mesh uses shared indexed vertices (768 longitude
columns), about **11.25 MiB** of GPU mesh buffers. RGB8 source plus mipmaps consumes
about 24.7 MiB, or about 33 MiB if the driver pads RGB to RGBA. Noise adds 1 MiB and
wind data 4 KiB: roughly **37–46 MiB** total before existing framebuffers.
`--jupiter-low` uses 65,536 triangles and about 1.25 MiB of mesh buffers; map quality
is unchanged. CPU source/mesh staging is released after upload. Solar, system and
globe views share the renderer and clock. Missing/corrupt assets leave navigation
available, with a terminal error and retry button.

## Jovian system scale

One scene unit represents **714,920 km**, with Jupiter's equatorial radius 0.1.
All moon radii and orbital radii use that same conversion. Neither moon sizes nor
orbital distances are exaggerated. Jupiter retains its oblateness; moon shapes
use mean radii.

| Body | Radius (km) | Representative centre-to-centre orbital distance (km) |
| --- | ---: | ---: |
| Jupiter | 71,492 equatorial / 66,854 polar | — |
| Io | 1,821.49 | 421,800 |
| Europa | 1,560.80 | 671,100 |
| Ganymede | 2,631.20 | 1,070,400 |
| Callisto | 2,410.30 | 1,882,700 |

Moon values: JPL's [satellite physical parameters](https://ssd.jpl.nasa.gov/sats/phys_par/)
and [satellite mean elements](https://ssd.jpl.nasa.gov/sats/elem/), JUP365 table,
mean elements at 2000 Jan 1.5 TDB. Distances use semimajor axes as representative
circular orbital radii. Paths are coplanar with the equator; phases are fixed,
authored placements, not a date-specific ephemeris. Jupiter's radius references are
listed below.

The system opens with all four moon orbits in view. **J** approaches Jupiter and
**F** restores whole-system framing; ordinary zoom spans both. The HUD labels
true scale, mean distances and static phases. Moons can be only a few pixels wide
in the whole-system view. Their labels, sidebar and generous picking areas remain
usable without inflating the rendered bodies. Focused moon globes still use their
own normalized cameras. All four Galilean moons open detailed archival maps. Europa and Ganymede use
[shared faceted atlas explorers](SATELLITES.md). Returning restores system framing
and the selected moon. The diameter-comparison panel has been removed.

The [solar overview](SOLAR_SCALE.md) also uses physical sizes and orbital spacing.
Its camera scale differs from this local system. Other planetary systems retain
their labeled schematic layouts, except Mars and Saturn, which use physical sizes and mean orbital distances (see [MARS_SYSTEM.md](MARS_SYSTEM.md) and [SATURN_MOONS.md](SATURN_MOONS.md)).

## Physical references and scope

- [NASA Jupiter Fact Sheet](https://nssdc.gsfc.nasa.gov/planetary/factsheet/jupiterfact.html):
  reference radii, rotation, year, atmospheric temperatures, composition, gravity,
  inclination and obliquity. Gravity and temperature are referenced to atmospheric
  pressure levels, not solid terrain or a surface day/night pair.
- [NASA Jupiter Facts](https://science.nasa.gov/jupiter/jupiter-facts/):
  planetary context and the dated moon count displayed in the guide.
- [Cassini Red Spot Movie / PIA02829](https://science.nasa.gov/photojournal/red-spot-movie/):
  opposite jet directions and counterclockwise storm circulation.

Planet articles and region notes remain in the left guide and inline location
panel. Their Source buttons open the relevant NASA page.

## Manual check

1. Open solar → Jupiter system → Jupiter globe and return through both parents.
2. In the system use F/J and both framing buttons. Check tiny moon labels/sidebar
   selection, open a moon and return. Body sizes and orbital spacing keep one scale.
3. Track the Red Spot, accelerate time, then pause/resume/reset. Watch a full loop;
   cloud motion and whole-planet rotation should not jump at closure.
4. Switch Follow/Watch, drag out of tracking, and use all four regions. Inspect
   fine cloud structure at close zoom and the whole globe at Home.
5. Inspect both poles and cross the longitude seam. Expect illustrative polar
   structure, no holes or black source strips, and no elevation claim.
6. Open/scroll the field guide and location notes at normal and 200% UI sizes.
   The former comparison panel area should accept globe dragging.
7. Visit Mars/Mercury and return. Jupiter retains its paused time, selection and
   camera; the terrain explorers retain their controls and elevation readouts.
8. Try `--jupiter-low`. Test a missing/corrupt copied asset in an isolated folder,
   restore it and press R; parent navigation remains available throughout.
