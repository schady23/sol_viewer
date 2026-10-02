# Testing and release preparation

The Linux staging tree is a local release candidate. Windows compilation, native
Windows testing, and a Linux build against Valve's runtime SDK remain outstanding.
Nothing in these commands uploads a build or changes a Steam store page.

## Automated checks

Build and run the 19 CPU/data/input suites:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 6
ctest --test-dir build --output-on-failure
```

Tests cover navigation and retained view state, geometry, source assets, planetary
models, guides, strict numeric arguments, and Earth input ownership. Earth tests
include returning from the globe to the Moon selection, hidden controls, cloud
preferences, trench selection, landmark notes, and clicks at four UI scales.

Enable the optional real OpenGL test on headless Linux with Mesa:

```sh
cmake -S . -B build -DSOLAR_RENDER_TESTS=ON -DSOLAR_TEST_VIDEO_DRIVER=offscreen
ctest --test-dir build --output-on-failure
```

This adds a twentieth CTest suite with ten rendered scenes, eight rejected
launches, and cloud image comparisons. It requires Python 3.9 or later. For a
native window, set `SOLAR_TEST_VIDEO_DRIVER` to an empty string. Do not use
offscreen software-rendering timings as player performance measurements.

The larger suite exercises every included world and both belt regions:

```sh
python3 tools/render_smoke.py build/solar_system_sim --suite full --video-driver offscreen --output build/render-full
```

Each run uses an empty working directory, records terminal logs and PPM images,
and writes `report.json` with the executable hash and per-image hashes. Failures
include unsuccessful launches, unavailable assets, shader errors, incomplete
images, static clouds, mismatched cloud-loop endpoints, or moving clouds leaking
into ocean-free topography. Mars, Moon and Mercury use reduced geometry in this
suite; their source data is still loaded. These are launch/render checks, not a
substitute for interactive testing or a full-detail GPU memory test.

For memory and undefined-behavior checks with Clang on Linux:

```sh
cmake -S . -B build-sanitize -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ \
  '-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined
cmake --build build-sanitize --parallel 6 --target cli_tests earth_view_tests earth_tests guide_tests navigation_tests
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir build-sanitize --output-on-failure -R '^(cli|earth_view|earth|guide|navigation)$'
```

LeakSanitizer needs a process environment without ptrace; run these tests directly
on the host if a tracing sandbox prevents leak detection from starting.

## Stage a complete package

Prepare `data/mars16.mola`, `data/mercury64.dem`, and `data/moon64.dem` using the
body-specific preparation instructions. Source downloads and conversion tools
are development inputs and are excluded from the installed package.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSOLAR_PACKAGE_TERRAIN=ON
cmake --build build --parallel 6
cmake --install build --prefix build/stage-linux
python3 tools/verify_package.py build/stage-linux
python3 tools/render_smoke.py build/stage-linux/solar_system_sim --suite full --video-driver offscreen --output build/release-smoke
cpack --config build/CPackConfig.cmake -B build/packages
```

Use a fresh staging directory for each release candidate so removed files cannot
survive from an earlier build. `verify_package.py` checks every shipped resource
against the checkout and writes `MANIFEST.sha256` in the staging tree. The CPack
ZIP uses its own fresh install tree and does not include that staging manifest.
Unzip it and run the same verifier and smoke suite before distributing an archive.

`SOLAR_PACKAGE_TERRAIN=ON` fails configuration if any required terrain file is
missing. With it off, installation omits these three datasets and is unsuitable
as a complete release. An alternative prepared-data directory can be supplied
with `-DSOLAR_TERRAIN_DIR=/path/to/data`; give the same path to the verifier via
`--terrain-dir /path/to/data`.

The package includes the executable, assets, terrain, documentation, third-party
notices and SDL license. With the default `SOLAR_BUNDLE_SDL=ON`, shared SDL is
installed beside the executable on Windows and under `lib/` on Linux. Linux uses
`$ORIGIN/lib` to find it after relocation. System C/C++ libraries and graphics
drivers are not bundled. A host-built Linux executable still needs validation
against the runtime targeted for distribution.

On Windows, first build SDL and the app as described in the main README, then:

```powershell
cmake -S . -B build -DSOLAR_PACKAGE_TERRAIN=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release --prefix build/stage-windows
python tools/verify_package.py build/stage-windows
python tools/render_smoke.py build/stage-windows/solar_system_sim.exe --suite full --output build/release-smoke-windows
cpack --config build/CPackConfig.cmake -C Release -B build/packages
```

Windows instructions are provided for the next platform validation pass; they
have not been executed on Windows here. Verify the appropriate Visual C++ runtime
is present on a clean test machine or configured as a Steam prerequisite.

## Interactive acceptance before upload

- Enter Earth, toggle clouds and topography, return to the Earth system, select
  the Moon, enter it and return. Earth controls must appear only for Earth;
  returning to Earth must preserve its settings.
- Inspect cloud silhouette and motion at Earth-system approach distance and in
  the globe. Pause, resume and reset; switch relief and ocean-free topography.
- Open field guides and landmark notes, scroll them, follow a source, and check
  cyan shortcut labels at 75%, 100%, 150% and 200% UI settings. Select the Mariana
  Trench and verify the seafloor is revealed.
- Exercise mouse, keyboard, resize, fullscreen, hide/show UI and Alt+Tab. Check
  landscape, portrait and mixed-DPI displays on supported operating systems.
- Visit every full-detail terrain globe in one session, return through its
  system, and monitor memory and frame time on the minimum supported hardware.
- Test the installed package offline, as a non-admin user, from a directory
  containing spaces. Test explicit missing-data paths and retry/navigation.

Record device, OS, GPU/driver, resolution, build hash and results. Establish store
minimum/recommended specifications from these measurements. Controller support
and Steam Deck verification have not been established.

## Steam handoff

Use separate platform content roots/depots and configure the matching executable
launch option. Fill in the real application and depot IDs in the Steamworks
SteamPipe configuration; keep credentials outside this checkout. The staging
tree above supplies the content root. Follow Valve's [Uploading to Steam guide](https://partner.steamgames.com/doc/sdk/uploading).

For Linux distribution, build and test against the chosen Steam Linux Runtime
SDK, and select the corresponding runtime in Steamworks. The local host build
and bundled SDL alone do not establish runtime compatibility. See Valve's
[Linux platform instructions](https://partner.steamgames.com/doc/store/application/platforms/linux).

Before the first upload, finish a per-asset distribution-rights and credit review
using the pinned sources in `assets/README.md` and `docs/`, and settle the
application's own distribution terms. `THIRD_PARTY_NOTICES.md` records provenance
and SDL attribution; it is not a completed rights audit.

Upload a candidate to a private test branch, verify a clean Steam install/update
on each supported platform, and complete the store and build review checklists.
Valve requires both store presence and build approval before release; see the
official [release process](https://partner.steamgames.com/doc/store/releasing).
