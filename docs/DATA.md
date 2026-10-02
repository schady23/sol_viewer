# MOLA provenance and interpretation

Accessed **2026-09-25**. Dataset: **MGS-M-MOLA-5-MEGDR-L3-V1.0**, product
**MEGT90N000EB.IMG**, product version **2.0**, creation time **2003-04-03T01:00:00**.
NASA PDS Geosciences Node now maintains the PDS4 bundle; it retains the original
detached PDS3 labels. The converter intentionally supports this single verified
product rather than attempting a general PDS parser.

## Exact source files

- [Topography IMG, 16 ppd](https://pds-geosciences.wustl.edu/mgs/urn-nasa-pds-mgs_mola_topography_derived/meg016/megt90n000eb.img)
- [Original detached PDS3 label](https://pds-geosciences.wustl.edu/mgs/urn-nasa-pds-mgs_mola_topography_derived/meg016/megt90n000eb.lbl), retained as [megt90n000eb.lbl](megt90n000eb.lbl)
- [Maintained PDS4 label](https://pds-geosciences.wustl.edu/mgs/urn-nasa-pds-mgs_mola_topography_derived/meg016/megt90n000eb.xml), retained as [megt90n000eb.xml](megt90n000eb.xml)
- [Original archive copy](https://pds-geosciences.wustl.edu/mgs/mgs-m-mola-5-megdr-l3-v1/mgsl_300x/meg016/megt90n000eb.lbl)

| Label property | Interpretation used |
| --- | --- |
| `LINES=2880`, `LINE_SAMPLES=5760` | 16,588,800 samples, rows north to south |
| `SAMPLE_TYPE=MSB_INTEGER`, `SAMPLE_BITS=16` | Signed two's-complement, big-endian; PDS4 confirms `SignedMSB2` |
| `RECORD_BYTES=11520`, `FILE_RECORDS=2880` | 33,177,600 bytes; no embedded header or row padding |
| `UNIT=METER` | Raw signed value is elevation in metres |
| Scale/offset | No elevation scaling or offset is declared in either label; raw metres are used directly |
| Missing-data values | No missing/null/special constant is declared in either label; no sentinel is invented |
| Empty observation bins | Product supplies interpolated values; topography alone does not identify which bins were interpolated |
| Coordinates | IAU2000, planetocentric, body-fixed; east-positive 0–360° longitude |
| Projection | Simple cylindrical, 16 pixels/degree, coverage 90°N–90°S / 0–360°E |
| Heights | Planetary radius minus local GMM3 areoid radius; **not** distance above a fixed sphere |

The label reports dataset extrema −8177 and +21171 m. The converter checks the
documented range and rejects unexpected values, formats, scaling, padding, or
special-value declarations rather than silently interpreting another product.
Every raw sample is retained; conversion only adds a header and changes byte order.

SHA-256 of the downloaded files (pinned by the download scripts):

```text
d18d9b9ab8c5516d02e157dd2cde0f1d0d160c21940e953ba22391269a545e7b  megt90n000eb.img
0f2bd919dddb86d83489a008d3007bd915507c892a0f22ce49ad5dd5d2fc7d8a  megt90n000eb.lbl
c393221db2de31916b30113ce28841dd967456dadb9bf2735eb30496eb37afa2  megt90n000eb.xml
```

These are local verification hashes of the cited files, not claims of a
PDS-published checksum manifest. If PDS updates a file, inspect the change before
updating the pins. The app's lightweight FNV-1a checksum detects accidental
payload damage; it is not a cryptographic authenticity check.

## Pixel centres and coordinates

The accompanying [DSMAP.CAT](https://pds-geosciences.wustl.edu/mgs/urn-nasa-pds-mgs_mola_topography_derived/catalog/dsmap.cat)
defines **one-based pixel centres**:

```text
SAMPLE = 2880.5 + 16 * (longitude - 180)
LINE   = 1440.5 - 16 * latitude
```

Thus source pixel `[row=0, col=0]` is centred at **89.96875°N, 0.03125°E**,
not at a pole or on the prime meridian. Our zero-based sample positions are
`x = wrap(lon) * 16 - 0.5`, `y = (90 - lat) * 16 - 0.5`.
Bilinear interpolation wraps columns across 359.96875°/0.03125°. Within the
last half pixel next to either pole, heights blend toward the average of the
terminal row, so all longitudes meet at one common vertex. That cap and its
readout are an interpolation convention, not a direct measurement at the pole.

## Source limits and documentation consulted

- [MEGDR overview and archive migration notice](https://pds-geosciences.wustl.edu/missions/mgs/megdr.html)
- [MOLA EGDR Software Interface Specification, version 2.2, Appendix C](https://pds-geosciences.wustl.edu/mgs/urn-nasa-pds-mgs_mola_topography_derived/document/egdrsis.htm)
- [Dataset catalog: coordinates, processing and confidence](https://pds-geosciences.wustl.edu/mgs/urn-nasa-pds-mgs_mola_topography_derived/catalog/megdrds.cat), local [copy](megdrds.cat)
- [Projection catalog](https://pds-geosciences.wustl.edu/mgs/urn-nasa-pds-mgs_mola_topography_derived/catalog/dsmap.cat), local [copy](dsmap.cat)

The 16 ppd bins hold median observed topography or a supplied interpolation.
The product label says only about 55% of equatorial bins contain a shot and
profile gaps reach 12 km. Grid spacing therefore does not guarantee independent
measurements. From the label's map radius, 16 ppd spacing is approximately
3.704 km north–south and `3.704*cos(latitude)` km east–west. The 128 ppd grid
would be approximately 463 m at the equator, but resampling and denser meshes do
not recover missing measurements. Counts tiles will be needed for coverage
indicators in Milestone 2.

The catalog's instrument/track precision figures do not make a 16 ppd median
grid accurate to sub-metre surface detail. No procedural craters or noise have
been added. Close exploration will require appropriate higher-resolution DEMs
and their own datum, coverage and uncertainty checks.

## Landmarks

Feature centres are navigation anchors, not summit points or entire-feature
elevations. Names and planetocentric east-positive coordinates come from the
IAU/USGS Gazetteer:

| Feature | Latitude | Longitude east | Source |
| --- | ---: | ---: | --- |
| Olympus Mons | 18.65 | 226.20 | [Feature 4453](https://planetarynames.wr.usgs.gov/Feature/4453) |
| Valles Marineris | −14.01 | 301.41 | [Feature 6288](https://planetarynames.wr.usgs.gov/Feature/6288) |
| Hellas Planitia | −42.43 | 70.50 | [Planitia listing](https://planetarynames.wr.usgs.gov/SearchResults?Feature+Type=31_Planitia%2C+planitiae&Target=20_Mars) |
| Elysium Planitia | 2.98 | 154.74 | [Planitia listing](https://planetarynames.wr.usgs.gov/SearchResults?Feature+Type=31_Planitia%2C+planitiae&Target=20_Mars) |

## Application file format v1

Exactly 33,177,632 bytes. All numeric fields are explicitly little-endian;
no C++ struct layout or host endianness is serialized.

| Byte offset | Content |
| --- | --- |
| 0–7 | ASCII `MARSMOLA` |
| 8–11 | uint32 version = 1 |
| 12–15 | uint32 width = 5760 |
| 16–19 | uint32 height = 2880 |
| 20–23 | uint32 pixels/degree = 16 |
| 24–27 | uint32 FNV-1a checksum of payload bytes |
| 28–31 | Reserved uint32 = 0 |
| 32 onward | int16 little-endian metres, original row/column order |

The header version binds the coordinate and datum interpretation above. A
future tiled format must use a different version and carry tile metadata.
