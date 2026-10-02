# Bundled planetary assets

These small text grids were generated from the local checked `data/mars16.mola`
and `data/mercury64.dem` files, using `preview_generate`.

Source identities and credits: [Mars MOLA](../docs/DATA.md) and
[Mercury MESSENGER MDIS V02](../docs/MERCURY.md). NASA/PDS/USGS derived topography;
These two preview grids contain elevations, not photographic textures.

Format, interpretation, visual limits and reproduction commands:
[Field Guide and previews](../docs/FIELD_GUIDE.md#small-models-in-planetary-system-views).
The complete source grids remain authoritative for globe elevation readouts.

## Jupiter cloud map

`jupiter2015.rgb` is a 3600 × 1800 RGB8 conversion of NASA SVS
`Hubble_Jupiter_color_global_map_2015a.tif`, observed 19 January 2015.
Credit: NASA, ESA, A. Simon (NASA GSFC), M. Wong (UC Berkeley),
and G. Orton (JPL-Caltech). It is an archival color composite, not elevation data.

[Exact source URL, checksum, format, credits and regeneration](../docs/JUPITER.md#exact-source-and-preparation).
The runtime preserves the native cloud map, adding Juno-inspired fine filaments,
stylized lighting and a seamless model animation. Fine procedural structure and
polar fill are illustrative. Juno images served as visual references and are not
bundled textures. The map is not elevation data or measured evolving weather.

## Moon

moon.preview: 256 × 128 display grid derived from LOLA LDEM_64 V3.1,
1737.4 km sphere. moon2019.rgb: 8192 × 4096 RGB from NASA SVS's 2019
LROC/LDAM appearance map. The map contains brightness/color, never terrain heights.
The checked RGB format has one ASCII header followed by north-to-south RGB rows;
zero longitude is at its horizontal centre. It is loaded with native C++ and OpenGL.

NASA/GSFC/Arizona State University / Ernie Wright.
Exact source URLs, checksums, preparation and polar limitations: [docs/MOON.md](../docs/MOON.md).

## Earth

- `earth2004.rgb`: 4096 × 2048 / full RGB adaptation of NASA Blue Marble:
  Next Generation July 2004. NASA Earth Observatory; appearance, not elevation.
- `earth-clouds.gray`: 2048 × 1024 alpha from NASA SVS GEOS-5 visualization
  3837, frame clouds.0350.tif. Cindy Starr / NASA SVS; illustrative cloud structure.
- `earth2022.dem`: 2400 × 1200 signed metre heights from a 9-arc-minute subset
  of NOAA ETOPO2022 v1's 60-second Ice Surface grid. EGM2008 vertical datum;
  land and seafloor elevations. Runtime row order is north to south.

[Exact sources, hashes, checked formats, limits and regeneration](../docs/EARTH.md).

## Venus

`venus-magellan.dem` contains the prepared USGS Magellan v02 elevation grid.
`venus-surface.rgb` is the native 1440 × 720 RGB8 conversion of JPL's
`ven0ajj2.tif` Magellan/Venera/Pioneer radar composite. Credit: NASA/JPL-Caltech
(McAuley). Surface hues are simulated, not measured optical color or heights.
See [Venus sources and preparation](../docs/VENUS.md). Preserve both assets with
the executable's assets folder; dimensions, lengths, and checksums are checked at runtime.

## Mars moons and remaining Galilean atlases

- `phobos-nasa.mesh` and `deimos-nasa.mesh`: checked, uniformly normalized NASA
  textured 3D model geometry, preserving UV registration. NASA/JPL-Caltech.
- `europa-voyager-galileo.rgb` and `ganymede-voyager-galileo.rgb`: 4096 × 2048
  tinted monochrome USGS Voyager/Galileo mosaics. Artistic display colors, no DEM.

[Exact source URLs, pinned SHA256, conversion, formats and coordinate limits](../docs/SATELLITES.md).

## Saturn

- `saturn-vtad.rgb`: 2048 × 1024 checked RGB8, reprojected from the NASA VTAD
  Saturn visualization's cube UV atlas using its original textured geometry.
- `saturn-rings.rgba`: 4096 × 1 checked radial RGBA8 strip, reduced from the
  same model's ring texture in premultiplied alpha space.

Credit: NASA Visualization Technology Applications and Development (VTAD).
Appearance is a visualization, without measured elevation or evolving weather.
[Source pin, preparation, physical ring radii and limits](../docs/SATURN.md).


## Seven major Saturn moons

`mimas-nasa.mesh`, `enceladus-nasa.mesh`, `tethys-nasa.mesh`, `dione-nasa.mesh`,
`rhea-nasa.mesh`, `titan-nasa.mesh` and `iapetus-nasa.mesh` preserve supplied NASA
VTAD textured visualization geometry. Colors follow each material's base-color
texture; Mimas's normal map is not used as color. Runtime shapes are calibrated
by volume to JPL mean radii in the Saturn system. Titan's amber haze is authored
in the shader; the underlying asset is its surface visualization.
[Credits, pinned sources, format and offline reproduction](../docs/SATURN_MOONS.md).


## Outer worlds

`uranus-nasa.mesh` and `neptune-nasa.mesh` retain NASA VTAD cloud appearance with
explicit polar-axis conversion and physical oblate calibration.
`uranus-clouds.rgb` and `neptune-clouds.rgb` reproject their source UV textures
into checked 2048 × 1024 RGB maps. The renderer samples these maps on 589,824
triangle ellipsoids and adds illustrative layered atmospheric flow.
`miranda-nasa.mesh`, `ariel-nasa.mesh`,
`umbriel-nasa.mesh`, `titania-nasa.mesh`, `oberon-nasa.mesh` and `triton-nasa.mesh`
preserve NASA's visualization textures and UVs. Credit: NASA VTAD.
`proteus-reference.mesh` is an untextured reference sphere, **not** a NASA shape
or global image map. [Source pins and reproduction](../docs/OUTER_WORLDS.md).

Dwarf planet assets: `ceres-nasa.mesh`, `pluto-nasa.mesh` (NASA VTAD), and `haumea-reference.mesh`, `makemake-reference.mesh`, `eris-reference.mesh` (untextured reference shapes). Reproduction and provenance: [BELTS.md](../docs/BELTS.md), `tools/prepare_dwarfs.py`. No invented surface features are asserted for the unvisited worlds.
