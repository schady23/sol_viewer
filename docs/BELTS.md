# Asteroid belt, Kuiper belt and dwarf planets

From Sol, click **MAIN ASTEROID BELT** or **KUIPER BELT**. One click opens the region. Select a dwarf in the sidebar or on its locator to approach automatically; Enter or double-click opens its globe. F or the whole-region button restores the regional view. Alt+Left and the breadcrumbs return to the saved parent camera. I opens source-linked field guides.

Direct launches: `--start asteroid-belt`, `--start kuiper-belt`, `--start ceres`, `--start pluto`, `--start haumea`, `--start makemake`, `--start eris`. Existing `--start system --body 'KUIPER BELT'` and `--start globe --body PLUTO` also work.

## Scale and orbital structure

The main belt is an idealized orbital distribution between approximately 2.1 and 3.3 AU, with Mars and Jupiter reference-orbit guides. Its scene uses **one AU per unit**, including the Sun and Ceres radii. The Kuiper region uses **40 AU per unit** for both bodies and distances, so the main 30–50 AU region and Eris's much larger orbit can be inspected together. Sol retains its existing ten AU per unit and displays no belt sample dots. Each dedicated belt view displays only its own sample distribution. Transitions join independently scaled views; dwarf globe views normalize their own bounding radius to one unit.

The five recognized dwarfs are included: Ceres in the main belt; Pluto, Haumea and Makemake in the outer region; Eris in the scattered population. Belt parents in the navigation catalog mean organizational regions, not gravitational primaries. **Every dwarf physically orbits the Sun.**

Dwarf guides use rounded JPL SBDB eccentricity, inclination, node, argument of perihelion and semimajor axis. The query snapshots in `docs/belts/` include the source epochs. Different bodies have different source epochs; orbital phases are intentionally authored for readable atlas placement, so this is **not a current ephemeris**. Kepler's equation is solved for positions and ellipse guides. Positive ecliptic Y maps to negative scene Z, and ecliptic north maps to scene Y.

| Body | Reference radius, km | a, AU | e | inclination, degrees |
|---|---:|---:|---:|---:|
| Ceres | 469.7 mean | 2.77 | .0797 | 10.6 |
| Pluto | 1188.3 mean | 39.6 | .252 | 17.1 |
| Haumea | ~798 equivalent; 1161 longest semiaxis | 43.1 | .194 | 28.2 |
| Makemake | ~715 reference | 45.6 | .159 | 29.0 |
| Eris | 1163 reference | 67.9 | .438 | 43.9 |

Ceres and Pluto mesh volumes are calibrated to their reference mean radii. Their bounding radii are respectively 482.4802672 and 1188.6480562 km; uniform normalization preserves NASA's supplied proportions.

The main belt contains 3600 deterministic sample markers, with depletion near a = 2.50, 2.82, 2.96 and 3.28 AU (3:1, 5:2, 7:3 and 2:1 Jovian resonances). The illustrative eccentricities range .015–.175 and inclinations 0–20 degrees. These are gaps in **semimajor axes**, not empty radial trenches: eccentric paths can enter their radial distances.

The Kuiper view contains 2800 deterministic markers sampled from four illustrative families: cold classical (42–47 AU, low eccentricity and 0–5 degree inclination), hot classical (40–50 AU, up to 30 degrees), 3:2 resonant (around 39.4 AU) and scattered (a = 55–95 AU, perihelion 30–40 AU). Population fractions, individual elements and phases are authored to communicate structure. They do not fit an observed catalog, survey bias, resonant-angle libration or measured intrinsic population fractions. The scattered component extends beyond the main belt; Eris's ellipse reaches approximately 97.6 AU.

**Dots are enlarged locators, not physical boulders.** Real bodies would be invisible from these regional distances. The Sun and dwarf geometry use physical sizes. Most of the volume is empty; the display does not make a dense cinematic rock field or a glowing dust torus. All orbit paths are atlas guides, not solid matter. The sampled populations are static, matching the atlas's existing static solar layout.

## Surfaces and Haumea's ring

Ceres and Pluto use NASA VTAD textured GLBs, converted through the existing facet pipeline. They retain the supplied geometry, UV sampling, variable coverage and image illumination. They are visualization assets, not calibrated DEMs. Both contain 327168 flat-colored triangles after planar subdivision.

Haumea, Makemake and Eris have no spacecraft surface maps. They use bundled **untextured reference shapes**, with no invented craters. Broad pale or reddish tints are illustrative. Haumea's triaxial semiaxes are an approximate 1161 × 852 × 513 km, following the 2017 occultation/light-curve shape interpretation, not an exact surveyed figure. Its equivalent radius is approximately 798 km. Its equatorial ring has a reference radius of 2287 km, width 70 km and display opacity .5. Ring plane orientation and illumination are illustrative, rather than an inertial pole/ephemeris model. Dwarf moons and Pluto's atmospheric haze are not included.

`tools/prepare_dwarfs.py` reproducibly builds these assets, verifies SHA256 for downloaded GLBs, and emits checksummed mesh files. Example:

```
python3 tools/prepare_dwarfs.py ceres assets/ceres-nasa.mesh --source /tmp/ceres.glb
python3 tools/prepare_dwarfs.py pluto assets/pluto-nasa.mesh --source /tmp/pluto.glb
python3 tools/prepare_dwarfs.py haumea assets/haumea-reference.mesh
```

The tool refuses to overwrite existing outputs. NASA source SHA256 values:

- Ceres: `c43ec43aa8fb90aa8cf30660c2f2853cfac408d38149d1dc470490f4f4aa9899`
- Pluto: `9915e6874ff7e7cd64faf8ef016ea16de6c8058417c42909efa099a88f2dd25b`

## References (checked 30 September 2026)

- [NASA Dawn FAQ: belt spacing and immense empty volume](https://science.nasa.gov/mission/dawn/faq/).
- [JPL main-belt semimajor-axis histogram and Kirkwood gaps](https://ssd.jpl.nasa.gov/diagrams/mb_hist.html).
- [NASA Kuiper belt facts: main belt, resonances and scattered populations](https://science.nasa.gov/solar-system/kuiper-belt/facts/).
- [NASA's five recognized dwarf planets](https://science.nasa.gov/dwarf-planets/).
- [JPL SBDB API](https://ssd-api.jpl.nasa.gov/doc/sbdb.html): numbered objects 1, 134340, 136108, 136472 and 136199, `phys-par=true`.
- [NASA Ceres facts](https://science.nasa.gov/dwarf-planets/ceres/facts/) and [NASA Ceres 3D model](https://science.nasa.gov/resource/ceres-3d-model/); [GLB](https://assets.science.nasa.gov/content/dam/science/psd/solar/2023/09/c/Ceres_1_1000.glb).
- [NASA Pluto facts](https://science.nasa.gov/dwarf-planets/pluto/facts/) and [NASA Pluto 3D model](https://science.nasa.gov/resource/pluto-3d-model/); [GLB](https://assets.science.nasa.gov/content/dam/science/psd/solar/2023/09/p/l/Pluto_1_2374.glb).
- [Ortiz et al.: Haumea's size, shape and ring](https://arxiv.org/abs/2006.03113), Nature 550 (2017).
- [NASA Makemake](https://science.nasa.gov/dwarf-planets/makemake/) and [NASA Eris](https://science.nasa.gov/dwarf-planets/eris/).

- [New Horizons measured Pluto radius](https://arxiv.org/abs/1603.00821).
- [ESO occultation measurement of Eris: 2326 km diameter](https://www.eso.org/public/news/eso1142/).
