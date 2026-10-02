# Solar overview: physical scale

The overview now uses one length conversion for the Sun, all eight planets, and
reference orbital distances: **one scene unit = 10 AU = 1,495,978,707 km**.
Body diameters and distances receive no separate enlargement or compression.

```sh
./build/solar_system_sim --start solar --fullscreen
```

## Navigation and visibility

- **F / Whole solar system** frames all eight reference orbits.
- **N / Inner planets** frames Mercury through Mars. The wheel or +/- zooms
  continuously; drag rotates the view. Home resets the full-system camera.
- Click a locator, floating label or sidebar entry to select. Enter or double-click
  opens that body's local system; selecting the central planet opens its globe.
  Sun opens directly to its placeholder globe. Alt+Left or breadcrumbs returns
  with the overview's camera and selection retained.
- Most bodies are smaller than a pixel at full-system framing. Small hollow
  crosses are **locator markers**, as labeled in the HUD. They are overlays, not
  enlarged planet models. Label leader lines identify their anchors; the sidebar
  remains available when labels are crowded or offscreen. Tab hides overlays.
- The right panel gives camera distance from the Sun in AU. The selected-body
  readout shows its reference radius and orbital distance in AU.

These are static reference circles at each planet's semimajor-axis distance, with
fixed representative angular placements. They communicate physical **scale**, not
current planetary positions. Ellipticity, orbital inclination and ephemeris/time
calculations are outside this change. The HUD explicitly says static/circular
orbits. At the same camera setting, the inner planets crowd around the Sun while
most of the orbital span lies between the outer planets.

## Exact values and sources

| Body | Reference radius (km) | Reference orbital radius (AU) |
| --- | ---: | ---: |
| Sun | 695,700 | 0 |
| Mercury | 2,440.53 | 0.38709927 |
| Venus | 6,051.8 | 0.72333566 |
| Earth | 6,378.1366 | 1.00000261 |
| Mars | 3,396.19 | 1.52371034 |
| Jupiter | 71,492 | 5.20288700 |
| Saturn | 60,268 | 9.53667594 |
| Uranus | 25,559 | 19.18916464 |
| Neptune | 24,764 | 30.06992276 |

Sources consulted 28 September 2026:

- Planetary **equatorial radii**: [JPL Planetary Physical Parameters](https://ssd.jpl.nasa.gov/planets/phys_par.html).
  The Sun uses its **volumetric mean radius** from the [NASA Sun Fact Sheet](https://nssdc.gsfc.nasa.gov/planetary/factsheet/sunfact.html).
- Orbital dimensions: the J2000 `a` values in **Table 1** of
  [JPL Approximate Positions of the Planets](https://ssd.jpl.nasa.gov/planets/approx_pos.html).
  These are fitted semimajor axes. Only `a` is used; the position formulae and time
  rates are not implemented here. Earth's reference distance uses that table's
  Earth-Moon barycentre value as a proxy for Earth's orbit.
- AU conversion: [JPL Astrodynamic Parameters](https://ssd.jpl.nasa.gov/astro_par.html),
  **149,597,870,700 metres exactly**, from IAU 2012 Resolution B1.

The radii set overview geometry size only. They do not change the existing Mars
MOLA or Mercury MDIS elevation datums. Remaining placeholder shapes are approximations. Jupiter and Saturn have oblate
atmospheric renderers; Saturn's major ring radii share the physical size scale.

## Coordinates and implementation

`catalog.*` stores physical values as doubles separately from schematic local-view
presentation sizes. Scene radius is `radiusKm / astronomicalUnitKm * 0.1`;
orbit-guide radius is `orbitAu * 0.1`. The existing fixed angular placements remain.
The Sun is at the origin, with all reference circles in the XZ plane and +Y up.

The overview camera remains Sun-centred. Its full-system distance is 9.5 scene
units (95 AU); inner framing is 0.52 (5.2 AU). Zoom spans 0.008–12 units. This
bounds coordinates to small floats and keeps the Sun outside the existing near
clip plane, including the navigation dolly, and Neptune inside the far plane at
maximum distance. Portrait windows preserve horizontal fit. The overview is for
interplanetary navigation; detailed inspection uses the existing separately
normalized local-system and globe cameras, rather than planet-surface precision
in a Sun-centred frame.

Subpixel visibility uses a double-precision ray through the body's centre, so
rounding a screen coordinate cannot suppress a tiny planet's label. Actual-size
geometry remains available to picking; locator selection chooses the nearest
visible screen anchor within nine logical pixels. Label hit areas do not affect
rendered sizes. Label placement avoids other markers and UI panels.

No dependency, terrain data or globe renderer was added. The Jovian system keeps
its existing physical scale. The [Earth–Moon system](EARTH.md) now also uses a
shared physical length scale. Remaining local systems are explicitly schematic;
this document describes the **overall solar view**.

## Manual verification

1. Launch the overview: all eight orbit guides fit; the Sun and inner planets are
   close together. Confirm the true-scale and locator disclosures.
2. Use N and F, plus both buttons, then zoom and rotate. The four inner orbits
   separate in the closer view; no body grows independently of the camera.
3. Select Mercury in the inner view, open its system, and return. Repeat with an
   outer planet at full-system framing. Camera and selected body should persist.
4. Open Jupiter's system and detailed globe; check its existing time controls and
   1 / Red Spot selection, then return to the overview.
5. Repeat in a narrow window and at 200% UI. Use the sidebar if a marker or label
   lies behind an overlay. Tab hides all locator/UI overlays.
