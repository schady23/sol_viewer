# Sun explorer

Open the Sun from the solar overview with Enter, or run
`build/solar_system_sim --start sun`. The Sun now renders in both the overview
and its close explorer; the overview retains its common physical size/distance
scale.

The explorer shows an emissive photosphere with moving granulation, dark
sunspots and bright flaring active regions. A faint streaming corona surrounds
it, with luminous prominence arches and expanding, fading plasma fronts.
The sphere is intersected analytically per pixel, so its silhouette remains
smooth even in a close view. Noise is deterministic and no assets are downloaded.

The Sun's mean radius is 695,700 km, making its diameter about 109 times
Earth's. The scale panel keeps this comparison in text; the explorer contains
only the Sun. Sunspot groups have irregular dark cores, striated penumbrae and
small neighboring pores. Their size, outline and location evolve smoothly,
with staggered growth and decay rather than fixed circular dots.

## Controls

- **1**: approach the photosphere, letting the Sun fill the view.
- **2**: pull back to see the corona and ejections.
- **3**: full Sun view.
- **Space**: pause or resume solar activity.
- **S**: cycle activity speed through 1×, 3× and 10×.
- **R**: restart the activity loop.
- Drag or arrows: rotate; wheel or +/−: zoom; **Home**: reset the camera.
- **I**: open the Sun field guide; **Alt+Left**: return to the solar overview.

The same actions have on-screen buttons. `--sun-paused --sun-phase 0..360`
provides reproducible loop frames; `--distance` sets the initial Sun camera
in solar radii.

## Interpretation and sources

This is an authored emission-color illustration. Granulation, magnetic-region
placement, flare brightness, corona visibility and ejection shapes are
procedural. The 90-second loop, its one-turn rotation and eruption speed do
not reproduce the Sun's physical rotation, current activity, a particular
observation or a forecast. The corona is emphasized so it remains visible
beside the bright photosphere.

Flares release energy, whereas coronal mass ejections expel solar material and
magnetic field. Sunspots are cooler, darker parts of active regions; the bright
hotspots surrounding them depict activity rather than measured temperature.
The expanding shells fade to zero with zero slope before resetting. Every
other time-dependent field uses integer periodic harmonics, closing the loop.

Physical references: [NASA Sun fact sheet](https://nssdc.gsfc.nasa.gov/planetary/factsheet/sunfact.html),
[NASA Sun facts](https://science.nasa.gov/sun/facts/),
[NASA solar storms and flares](https://science.nasa.gov/sun/solar-storms-and-flares/).

## Verification

Navigation tests cover the absence of reference bodies, activity clock pause/rate/frame independence and loop closure,
Sun routing and control hit areas. The field-guide tests verify Sun routing
and reject unsupported geographic locations. Native GPU checks cover the
three views, activity controls, guide, overview return, rotation and loop frames.
Review images: [scale and activity](sun-scale.png), [photosphere closeup](sun-close.png).
