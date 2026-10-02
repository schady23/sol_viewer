# Satellite orbit model

The satellite view is an ideal two-body ellipse around Mars, with the camera at the
satellite rather than looking at a satellite icon from outside. There is no orbital
library or added runtime dependency. Implementation is in `src/orbit.*`; all public
element values are validated before replacing the current configuration.

## Units and sources

Sources consulted 2026-09-26:

- **Mars GM = 42828.37362069909 km³/s²**, `BODY499_GM` from the official
  [NAIF gm_de440.tpc constants file](https://naif.jpl.nasa.gov/pub/naif/generic_kernels/pck/gm_de440.tpc).
  This is Mars itself, rather than `BODY4_GM` for the Mars system.
- **Sidereal rotation period = 24.6229 hours**, from the
  [NASA Mars Fact Sheet](https://nssdc.gsfc.nasa.gov/planetary/factsheet/marsfact.html).
  We use uniform spin about +Y; no pole precession/nutation or dated rotation model.
- Periapsis, eccentricity, inclination, node, periapsis argument and mean anomaly
  conventions follow the quantities described in
  [NAIF's conic-element documentation](https://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/conics_c.html).
  SPICE is a reference only and is not linked or downloaded by the app.
- **R = 3396 km** is the existing MOLA display sphere from the
  [exact dataset label](megt90n000eb.lbl), not a newly substituted radius/datum.

The periapsis and apoapsis fields are **altitudes above that reference sphere**,
not radii from the centre, heights above the local terrain, or areoid elevations.
Their accepted range is 100–20,000 km, with periapsis ≤ apoapsis. Inclination is
0–180° relative to the Mars equator: 0° prograde, 90° polar, 180° retrograde.
Equal apsides produce a circular orbit. Invalid edits preserve the previous orbit.

## Propagation

For input altitudes `hp`, `ha`, and gravitational parameter `mu`:

```text
rp = R + hp                 ra = R + ha
a = (rp + ra)/2             e = (ra - rp)/(ra + rp)
n = sqrt(mu/a^3)            T = 2*pi/n
M(t + dt) = M(t) + n*dt     (wrapped to 0..2*pi)
E - e*sin(E) = M            (48-step bracketed solve)
x = a*(cos(E) - e)          y = a*sqrt(1-e^2)*sin(E)
r = a*(1 - e*cos(E))        speed = sqrt(mu*(2/r - 1/a))
```

The velocity uses the time derivative of these orbital-plane coordinates.
Consequently it conserves two-body energy/angular momentum and moves faster near
periapsis. Rotating the orbital plane by inclination gives inertial position and
velocity. A negative Mars-spin rotation then expresses both in the body's map
axes, so the ground track drifts underneath a fixed inertial orbit. The displayed
speed remains **inertial**, not speed relative to the rotating ground.

On first entry, periapsis is placed at the ascending equatorial crossing, near the
atlas camera's current longitude. The argument of periapsis is fixed at zero. The
session starts at an arbitrary epoch with aligned inertial/body map axes; this is
not an observation at a real UTC date. `--lon` chooses the starting meridian, and
`--orbit-phase` chooses the initial mean anomaly in degrees. Editing elements keeps
the current mean anomaly and spin phase, immediately defining a new ideal orbit.
It does not calculate a manoeuvre or delta-v.

Simulation speed is 1×, 10×, 60× or 300× (default 60×). Pause or leaving flight stops
the orbit clock, preserving its phase. Minimized windows also suspend it. Each
frame contributes at most 0.25 real seconds to avoid jumping after a long stall.
Weather retains its separate illustrative clock and pause control.

## Camera and readouts

The flight frame uses radial outward, transverse inertial velocity, and their
cross product. It stays defined through the poles and for retrograde orbits; the
camera does not derive its attitude from singular longitude values. Its position
is stored directly in Cartesian coordinates rather than round-tripping through
latitude/longitude.

- **Horizon view:** looks along flight direction, 12° below the reference-sphere
  horizon. At very high altitude it approaches a planet-centred view.
- **Straight down:** looks at nadir, with flight direction toward the top of the view.
- Drag adds yaw and look tilt. The wheel changes vertical FOV between 25° and 90°;
  it does not change orbital elements or move the satellite off its ellipse.
- `V` changes the view and resets look offsets/FOV. `O` restores the previous atlas
  view and relief setting; `Home` returns to the default whole-planet view.

Flight geometry uses **1× elevation**, fixed while in flight. At the 100 km minimum
altitude, the camera remains above the highest source terrain and both illustrative
weather shells. Near/far clipping is 0.005/20 display radii. The flight camera has
its own allowed radial distance, so the atlas's 1.085 minimum does not lift a 100 km
orbit to an incorrect height.

The globe shader, weather ray intersection, labels and depth unprojection use the
same eye/basis/lens. A pointer hit reads the full MOLA sampler. With no pointer hit,
flight samples the centre ray; if that points into space, the readout explicitly
says **SUBSATELLITE**, meaning the geographic point underneath the spacecraft.
These heights remain interpolated MOLA metres above the areoid. The separate
telemetry panel shows sphere-relative altitude, inertial speed, period and lens FOV.

## Limits and checks

There is no drag, atmospheric decay, oblateness/J2, precession, solar/lunar gravity,
eclipse model, real mission ephemeris or manoeuvre planner. The sphere with
areoid-relative topography remains an approximation, even at 1×. Lighting and
weather are visual aids; neither follows orbital time as a physical climate/Sun
simulation. The new mesh fully exposes the 16 ppd source spacing, about 3.70 km at
the equator, but cannot add rover-scale terrain or recover missing observations.

Automated tests check apsides, circular and high-eccentricity cases, energy,
angular momentum, speed ordering, Mars rotation, polar/retrograde inclination,
pause/disabled state, invalid parameters, projection/unprojection, camera basis,
near-plane clearance and scaled control layout. Native GUI checks cover numeric
editing, rejected input, pause/speed/view controls, optical zoom, look-around,
fullscreen changes, UI scale, returning to the atlas and terrain-depth preservation.
