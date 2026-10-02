# Mercury / MESSENGER terrain

Mercury now opens a measured terrain globe from its local-system view. This is an
independent dataset and view in Solar System Sim; the original Mars project is
untouched. The solar/system overview still uses the lightweight identifying sphere.
The focused view uses real heights, selectable locations and the existing custom
industrial HUD. No new runtime dependency was added.

## Exact sources

The starting reference is NASA SVS's [First Global Topographic Model of Mercury](https://svs.gsfc.nasa.gov/30820/).
Its color-shaded pictures are visualizations. They are not used as elevation data.
Credit: NASA / USGS / Arizona State University / Carnegie Institution of Washington /
JHU Applied Physics Laboratory.

The numeric source is the corrected **V02** USGS MESSENGER MDIS stereo DEM:

- [MSGR_DEM_USG_SC_I_V02.IMG](https://planetarydata.jpl.nasa.gov/img/data/messenger/MDIS/messdem_1001/DEM/GLOBAL/IMG/MSGR_DEM_USG_SC_I_V02.IMG)
- [MSGR_DEM_USG_SC_I_V02.LBL](https://planetarydata.jpl.nasa.gov/img/data/messenger/MDIS/messdem_1001/DEM/GLOBAL/IMG/MSGR_DEM_USG_SC_I_V02.LBL), preserved [locally](mercury/MSGR_DEM_USG_SC_I_V02.LBL)
- [USGS product description](https://astrogeology.usgs.gov/search/map/mercury_messenger_global_dem_665m)
- [PDS DEM Software Interface Specification](https://planetarydata.jpl.nasa.gov/img/data/messenger/MDIS/messdem_1001/DOCUMENT/MSGR_DEM_SIS.HTM)
- [Archive errata](https://planetarydata.jpl.nasa.gov/img/data/messenger/MDIS/messdem_1001/ERRATA.TXT), preserved [locally](mercury/ERRATA.TXT)
- [Projection catalog](https://planetarydata.jpl.nasa.gov/img/data/messenger/MDIS/messdem_1001/CATALOG/DEM_EQU_MAP.CAT), preserved [locally](mercury/DEM_EQU_MAP.CAT)
- [USGS ISIS pixel and mapping conventions](https://isis.astrogeology.usgs.gov/Isis2/html/key_defs.html)

The errata identifies V01 as preliminary and replaces it with V02. The USGS
product description explains the stereo control network, filtering and gridding;
this is not an MLA-only laser-altimeter map. Its grid spacing is not an accuracy
claim, and local interpolation/smoothing can erase or distort small features.

Values below are read from the actual V02 label, not the V01 example in the SIS:

| Property | Value |
| --- | --- |
| Dataset / product | `MESS-H-MDIS-5-DEM-ELEVATION-V1.0` / `MSGR_DEM_USG_SC_I_V02` |
| Raster | 23,040 samples × 11,520 lines; one band |
| Records | 11,520 records × 46,080 bytes; no header or line padding |
| Samples | Signed 16-bit `LSB_INTEGER` (little-endian) |
| Heights | `metres = DN × 0.5 + 0.0` |
| Missing | Raw DN `-32768`, tested before scaling |
| Label height range | −5,382 to +4,497 metres |
| Datum | Sphere, radius 2,439.4 km |
| Map | Simple cylindrical, planetocentric, positive-east longitude |
| Extents | 90° N to 90° S; 0° to 360° E |
| Central meridian / latitude | 180° E / 0° |
| Sampling | 64 pixels/degree, 665.24315270546 m at equator |
| Projection offsets | line 5759.5; sample 11519.5 |

SHA-256, verified on the downloaded files (2026-09-27):

```text
IMG  b1bb587a9414ff542a74337866582eb7b1b16e325f90a6367116f9f4cc9cdf30
LBL  6433d227b380cfe86b6589a5bed2f3623351475bf49b6d85c95cea093a4bef14
```

## Coordinates and scientific readouts

For zero-based column `x` and row `y`, the pixel centre is:

```text
latitude  = 90 - (y + 0.5) / 64
longitude =      (x + 0.5) / 64       [east, 0..360)
```

These follow the label's full rectangular bounds and the ISIS area-pixel
convention: the bounds are outer edges, and integral ISIS pixel coordinates are
centres. In zero-based coordinates the offsets give `lat=(5759.5-y)/64` and
`lon=180+(x-11519.5)/64`. The catalog's prose/equations contain a `TBD` origin and
an ambiguous longitude expression; they are not applied literally to reverse the
explicit east-positive full-global label. The first centre is 89.9921875° N,
0.0078125° E. The last is 89.9921875° S, 359.9921875° E.

The shared right-handed renderer uses +X at 0° E, +Y north, and −Z at 90° E:

```text
direction = (cos(lat)*cos(lon), sin(lat), -cos(lat)*sin(lon))
position  = direction * (1 + heightMetres * exaggeration / 2439400)
```

The radius is normalized to one for rendering. The camera remains local to
Mercury, so solar-system distances do not enter OpenGL coordinates. Elevation
readouts bilinearly sample the **full 64 ppd grid**, independent of mesh density,
lighting, palette and exaggeration. They are metres above the reference sphere,
not Mars's areoid. The readout rounds to metres; raw source quantization is 0.5 m,
not a promise of 0.5-m vertical accuracy.

Longitude interpolation wraps. Each unobserved half-cell polar cap converges to
one mean from its terminal row, so a pole has a single position at every longitude.
Missing contributors invalidate interpolation unless their weight is zero;
triangles touching missing samples are omitted and the readout says `NO DATA`.
The downloaded V02 file contains zero missing samples.

## Detail, appearance and limits

Default geometry is **16 vertices/degree**: 5,760 columns, 2,880 cell-centred rings,
two shared poles, **33,177,600 triangles**. This matches Mars's current angular mesh
density. It samples Mercury's native grid bilinearly and has about **2.66-km**
spacing at the equator. `--mercury-detail 8` or `4` reduces geometry; readouts still
use the full source. No texture is interpreted as height and no procedural
craters or invented microterrain are added.

**E** switches between 10× relief (default) and true-height 1×. **C** switches
between restrained stone height tints and a six-color elevation palette. Both
are explicitly cartographic, not true-color imagery. Lighting follows the camera
for readable relief; it is not a Sun/epoch illumination model. Above 75° latitude,
facet lighting gradually softens toward the radial normal to avoid shimmering
from extremely thin cylindrical triangles; measured geometry is unchanged. Mars's weather and
satellite controls do not appear on Mercury.

Zoom spans 1.035–8 reference radii, approximately 85 km to 17,076 km above the
sphere; this is a view distance, not an orbit calculation. The closest camera and
near plane clear the highest 10× terrain. Zoom cannot reveal boulders or ground-scale
detail. Source spacing is ~665 m; rendered geometry is coarser, and the DEM itself
is interpolated regional topography. Future local tiles can use all 64 ppd geometry
without allocating a roughly 531-million-triangle global mesh.

There is one mesh resolution per globe today. Five-degree drawing batches share
vertices and wrapped seam/pole indices, so there are no independently displaced
edges or mixed-LOD cracks. A later LOD implementation must coordinate neighbour
edges (matching edge vertices or stitching); independent overlapping tile meshes
are not introduced by this change.

## Prepare and run

The prepared file is already present on this machine. From the project directory:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/solar_system_sim --start mercury --fullscreen
```

For a fresh checkout, obtain the 506.25-MiB source and prepare it once:

```sh
bash tools/download_mercury.sh
./build/mercury_convert data/source/mercury/MSGR_DEM_USG_SC_I_V02.LBL data/source/mercury/MSGR_DEM_USG_SC_I_V02.IMG data/mercury64.dem
./build/solar_system_sim --start mercury
```

On Windows, after the SDL/CMake setup in the main README:

```powershell
powershell -ExecutionPolicy Bypass -File tools/download_mercury.ps1
.\build\Release\mercury_convert.exe data/source/mercury/MSGR_DEM_USG_SC_I_V02.LBL data/source/mercury/MSGR_DEM_USG_SC_I_V02.IMG data/mercury64.dem
.\build\Release\solar_system_sim.exe --start mercury --fullscreen
```

The scripts verify both pinned hashes before installing downloaded files. The
converter validates product identity, dimensions, byte order, scaling, datum,
missing value and projection; refuses existing outputs; and preserves every raw
half-metre sample. Conversion also works without SDL using `-DMARS_BUILD_APP=OFF`.
Choose a new output path when reconverting an existing file.

The app never downloads data. It finds `data/mercury64.dem` relative to the working
directory, executable directory or its first two ancestors; use `--mercury-data`
for an explicit path. A missing/corrupt file shows a recoverable error. **R** retries
after preparation, and **Alt+Left** still returns to the local/solar-system view.

```sh
./build/solar_system_sim --start mercury --landmark 1
./build/solar_system_sim --start mercury --landmark 3 --relief 1
./build/solar_system_sim --start mercury --height-colors --ui-scale 1.5
./build/solar_system_sim --start solar --mercury-detail 8 --detail 8
```

`--start globe --body mercury` is equivalent to entering the detailed Mercury view.
Mercury-specific options default to Mercury unless `--start` or `--body` chooses
another view. `--lat`, `--lon`, `--distance`, `--landmark` and `--relief` work with
either detailed globe. `--detail` remains the Mars mesh setting;
`--mercury-detail` is independent.

## Landmarks

Centres are from the latest coordinate entries shown by the IAU/USGS Gazetteer;
its west-positive longitude was converted with `east = 360 - west`. On the source
sphere, planetographic and planetocentric latitude coincide. These are navigation
anchors, not outline polygons or higher-resolution local datasets.

| Location | Latitude | East longitude | Interpolated height |
| --- | ---: | ---: | ---: |
| [Caloris Planitia](https://planetarynames.wr.usgs.gov/Feature/979) | 31.65 | 161.98 | −441.591 m |
| [Rembrandt](https://planetarynames.wr.usgs.gov/Feature/14563) | −32.83 | 87.54 | −2047.700 m |
| [Rachmaninoff](https://planetarynames.wr.usgs.gov/Feature/14653) | 27.79 | 57.58 | −5276.349 m |
| [Beethoven](https://planetarynames.wr.usgs.gov/Feature/660) | −20.92 | 236.34 | −1060.581 m |

## Implementation and memory

- `src/mercury.*`: strict PDS import, signed half-metre storage, checked file I/O,
  sampling and landmark anchors; `tools/mercury_convert.cpp` is its offline CLI.
- `src/mercury_view.*`: Mercury controls/state and on-demand SDL worker.
- `src/renderer_mercury.cpp`: focused Mercury instruments and landmark markers.
- Shared `terrain.*` accepts a sampling function and per-body culling radius/range.
  Existing Mars sampling and behavior remain in place.
- Shared renderer has two independently owned terrain buffer slots and a radius
  uniform. Catalog, main routing, navigation and HUD recognize Mercury's dataset.

Disk format `MERCDEM2` v1: 32-byte header, followed by unchanged little-endian signed
16-bit samples in north-to-south row order. Bytes 0–7 are the magic; little-endian
uint32 fields at 8/12/16/20/24/28 are version, width, height, ppd, payload FNV-1a
checksum and reserved zero. V1 fixes the scale/datum/missing value to the validated
V02 product. Loading checks the entire header, length, checksum and sample range.
FNV catches corruption; the download's SHA-256 pins the source identity.

The worker loads ~506 MiB of raw samples and builds the CPU mesh. Only the main
thread uploads GL buffers, which can briefly pause presentation. Each detailed
body loads on first entry and stays cached for the session; camera, destination,
relief and palette survive returning to the map. Mars retains its independent
weather and orbital state.

Mercury's GPU mesh uses ~633 MiB at 16 ppd (~158 at 8, ~40 at 4). Both default
cached bodies plus source fields use roughly **1.76 GiB**, excluding driver/UI
allocations. The second body's CPU mesh temporarily adds ~633 MiB; loading is
bounded to these two datasets but is not a tile cache or configurable memory cap.
Use both detail options on constrained hardware. Full 64 ppd Mercury geometry,
textures, accurate lighting, physical Mercury orbits and higher-detail local tiles
remain future work.

## Verification

```sh
ctest --test-dir build --output-on-failure
./build/mercury_tests docs/mercury/MSGR_DEM_USG_SC_I_V02.LBL data/mercury64.dem
./build/solar_system_sim --start mercury --landmark 3 --frames 90 --screenshot build/mercury.ppm
```

Manual checks:

- Enter Mercury from Solar → Mercury system → Mercury. Return with Up/breadcrumbs.
- Select all four locations with buttons and 1–4; rotate and zoom to both limits.
- Hover terrain: check the east-positive coordinate and sphere-relative readout.
- At a fixed centre, toggle E: relief changes, measured elevation does not.
- Toggle C: palette changes, geometry/elevation do not. Tab hides/restores overlays.
- Cross 0°/360°, inspect both poles, and change UI scale/fullscreen/window size.
- Visit Mars, use its weather/orbit controls, then return to Mercury: both retain
  settings and the correct body's terrain. Repeat without another data load.
- Launch with `--mercury-data missing.dem`: confirm a useful error, retry and Up.

Linux native visual checks and numerical results are recorded in
[VERIFICATION.md](VERIFICATION.md). Windows paths are supported but untested here.
