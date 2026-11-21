<!--
| Date | Summary | Reason | Ticket | Model |
| 2026-11-21 | Document the prefetch option and comparison procedure. | Make the performance change reproducible. | | gpt-6.2-sol |
| 2026-11-21 | Add anonymous aggregate navigation timings and validation results. | Identify the dominant phases using the initialized viewer. | | GPT-6 (Codex) |
| 2026-11-21 | Record synthetic baseline and EXIF initialization prerequisite. | Separate decoding, scaling and rotation costs before prefetch work. | | GPT-6 (Codex) |
| 2026-11-21 | Document anonymous viewer measurements. | Establish a baseline before prefetch work. | | GPT-6 (Codex) |
-->

# Measuring PictView navigation

The tables below record the baseline before prefetch was implemented. Use the
same timing diagnostics to compare it with the current viewer.

## Enable prefetch

In PictView, open **Tools → Configuration → Performance** and enable
**Preload the next image in the direction of browsing**. Existing configurations
start with the option off. The viewer prepares one nearby image for **Fit whole**
or **Full screen**. It follows the most recent browsing direction and starts
forward when opening a new viewer.

The worker handles local JPEG, PNG and single-page TIFF images decoded by WIC.
It is limited to files of up to 128 MiB and rasters of up to 32 megapixels.
The viewer can reuse the decoded raster, EXIF transform and display bitmap.
Navigation opens the target as usual if preparation is unavailable, unfinished
or stale. After a resize, the viewer recomputes the scaled bitmap if needed.

For a comparison, open a synthetic sequence with the option off, record a few
transitions, then repeat with it on. Give the worker time to finish before each
transition and test a rapid transition separately. For a fully prepared image,
the navigation row has zero decode, transform and scale attempts. The elapsed
time describes the transition after background preparation, with no claim about
the total work performed for the image.

In a Release x64 smoke test with fit-to-window, six prepared 24 MP synthetic
transitions took 6.9–20.6 ms. These included both 90-degree directions and a
180-degree orientation. All six rows recorded zero decode, transform and scale
attempts on the viewer thread. A 48 MP image used ordinary loading. Changing a
prepared file's modification time caused a fresh read. A separate sequence
verified prepared single-page TIFF and PNG images with Unicode names under a
320-character directory, ordinary opening of a multipage TIFF and reload from
disk. The test also confirmed autorotation on the first open in a fresh process
and persistence of the Performance option across a restart.

## Measurement environment

The baselines below were collected on 2026-11-21 with the following
environment. It is a diagnostic baseline, not a comparison against a prefetched
implementation.

| Item | Value |
| --- | --- |
| Plugin source base | Release 0.17, `aef7a2dba7f38614a9166cff66662480efc1d5fc`, plus the timing instrumentation |
| Host source | Personal Branch View build, `7ca0d39c2723eb2079038047e1e1dd173b59ccb5` |
| Compiler and configuration | MSVC 14.51.36231, v145, Release x64, `/warnaserror` |
| CPU exposed to the environment | 2 cores, 4 logical processors |
| RAM | 8,309,796,864 bytes, approximately 7.74 GiB |
| Viewer window | 1758 x 838, fit-to-window, EXIF autorotation enabled and initialized |
| Displayed 6/24 MP raster | 1110 x 740 landscape; 493 x 740 portrait |
| Displayed 48 MP raster | 986 x 740 landscape; 555 x 740 portrait |
| Measured plugin SHA256 | `5E739C1599D0E31DC882130B327ED385F0009069440E7D7B96A04A90B6B837E8` |

The host is not the unmodified release-base host; its displayed 0.16 title is
stale. Use the recorded source revision to identify it. After this measurement,
the diagnostic `Begin` placement was adjusted for unsupported clipboard-image
navigation. The image-processing code was unchanged, but the recorded binary
hash identifies the artifact actually measured.

The Windows file cache was not forced cold. Phase costs include lazy file reads,
allocation and pixel copies where these occur within the instrumented functions.
The display boundary is completed GDI work, not physical screen presentation.

## Anonymous navigation baseline

After initializing EXIF on a synthetic fixture, a separate sequence measured
30 successful previous/next navigations, 15 forward and 15 backward. The initial
open is excluded. The two dimension groups each contain 15 timing samples.
Dimensions below are ordered short edge first; no image identifiers, format
details or metadata are retained in this report. Only file sizes and dimensions
describe the inputs.

| Raster dimensions | Encoded bytes, min-max | Total median | Total mean | Total p95 |
| --- | ---: | ---: | ---: | ---: |
| 1200 x 1800 | 510,556-980,547 | 46.823 ms | 46.837 ms | 59.168 ms |
| 3648 x 5472 | 3,904,498-6,146,353 | 411.593 ms | 407.993 ms | 463.432 ms |

The following exclusive phase costs are arithmetic means in milliseconds. They
partition the mean total, subject to rounding; they must not be added to the
median total. The nearest-rank p95 with 15 samples is the maximum observed value,
so these are descriptive measurements rather than stable tail-latency estimates.

| Phase | 1200 x 1800 | 3648 x 5472 |
| --- | ---: | ---: |
| Navigation | 0.394 | 0.386 |
| Open, excluding nested work | 3.451 | 10.855 |
| Decode | 20.115 | 143.142 |
| EXIF lookup | 0.349 | 5.473 |
| Transform | 0.000 | 123.610 |
| Scale and scaled bitmap preparation | 16.657 | 105.496 |
| Paint, including final GDI flush | 2.212 | 3.705 |
| Other | 3.659 | 15.325 |

For the larger dimension group, decode accounts for 35.1% of measured time,
transform for 30.3% and scale for 25.9%; together, approximately 91.2%. Paint
accounts for 0.9%. Every measured navigation entered decoding and scaling work,
including the return traversal. Repeated viewing therefore still pays these
costs with the Windows file cache already available.

This supports preparing decoded, transformed and display-sized image data before
navigation. A presentation buffer alone would address only a small measured
phase. The baseline predates the implementation and its settings.

## Synthetic baseline

The corpus consists only of generated textures, not real photographs. JPEGs use
quality 92 and 4:4:4 chroma sampling. At each resolution, all orientation variants
share the identical compressed JPEG image payload. The 24 MP PNG uses the same
source RGB pixels before JPEG compression. These deliberately textured inputs
are useful for controlled comparisons; their timings are not representative
estimates for arbitrary photographs.

The run contained one initial open and 16 successful navigations. The initial
open is excluded below. Traversal started at the unrotated 6 MP fixture, moved
forward through the listed orientation/format variants to the rotated 48 MP
fixture, then returned to the first fixture. The PNG was visited between the
24 MP JPEG orientations 1 and 3. Interior fixtures therefore have two samples;
the first and last have one. This small sample count supports observed ranges
only, not p95 or stable performance claims.

All timings are milliseconds. Decode and scale columns are arithmetic means;
total and transform columns show the observed minimum and maximum, or the single
observed value. EXIF orientation labels in this table describe synthetic inputs
only: 1 = normal, 3 = 180 degrees, 6 = 90 degrees clockwise, 8 = 90 degrees
counterclockwise.

| Synthetic fixture | Orientation | N | Total observed | Decode mean | Transform observed | Scale mean |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 6 MP JPEG | 1 | 1 | 132.9 | 87.5 | 0.0 | 35.4 |
| 6 MP JPEG | 6 | 2 | 176.5-190.3 | 86.2 | 30.2-32.9 | 36.2 |
| 24 MP JPEG | 1 | 2 | 491.6-497.4 | 340.4 | 0.0 | 135.2 |
| 24 MP PNG | 1 | 2 | 494.9-532.1 | 343.5 | 0.0 | 139.5 |
| 24 MP JPEG | 3 | 2 | 512.2-518.5 | 347.4 | 0.3 | 130.7 |
| 24 MP JPEG | 6 | 2 | 658.4-738.2 | 360.7 | 152.4-183.1 | 134.9 |
| 24 MP JPEG | 8 | 2 | 652.7-715.2 | 342.2 | 153.9-164.9 | 125.8 |
| 48 MP JPEG | 1 | 2 | 962.6-1009.0 | 703.8 | 0.0 | 252.7 |
| 48 MP JPEG | 6 | 1 | 1362.4 | 682.4 | 370.4 | 247.6 |

The measured expensive stages were full-frame decoding, scaling and, for
90-degree orientations, pixel transformation. The 24 MP 90-degree transformations
took 152.4-183.1 ms; the single 48 MP transformation took 370.4 ms. The 180-degree
path used flags and spent 0.320 ms in the transform scope on each visit, without
entering the backend pixel-transform counter. It should not be treated as the
same operation as a 90-degree pixel rotation.

Across the 16 navigations, the counters recorded 16 decode attempts, 16 scale
attempts, 7 backend transform attempts, 16 draw attempts and 25 paint calls.
The mean navigation phase was 0.401 ms and mean paint phase 4.572 ms. This shows
why merely swapping a final presentation buffer would leave substantial work
unless the next image's decode, required transforms and scaling were also ready.
All rows passed validation; the largest difference between the exclusive-phase
sum and total was 0.001 ms after CSV rounding.

## EXIF initialization in the baseline

In the baseline build the first autorotation could be skipped because the EXIF
library had not been loaded. Opening the EXIF dialog on a synthetic fixture
initialized the library for those runs. The current viewer initializes EXIF
before the first orientation lookup and shares the orientation rules between
foreground opening and prefetch.

## Reproduce a measurement

Use a Release x64 build of the standalone PictView project. Record the source
revision, compiler version, build configuration and host revision separately.
Substitute local values for the placeholders below; none are image identities.

```powershell
& '<MSBUILD_EXE>' '<CHECKOUT>\src\plugins\pictview\vcxproj\pictview.vcxproj' `
  /t:Build /m:2 /p:Configuration=Release /p:Platform=x64 `
  /p:PlatformToolset=v145 /p:PreferredToolArchitecture=x64 `
  /p:OPENSAL_BUILD_DIR=<BUILD_ROOT>\ /warnaserror /nr:false /v:minimal
```

Prepare a dedicated runtime copy outside the repository, preferably in the
system temporary directory. Stage `pictview.spl`, `exif.dll`,
`pictview-previewhost.exe` and `lang\english.slg` from the build output at
`<BUILD_ROOT>\salamander\Release_x64\plugins\pictview`. Verify the runtime uses
its own `config.reg` via `configstorage.ini` with `StorageType=RegFile`.
Runtime configuration can contain recent-file history and must remain private.

Set the following environment variable before starting a new dedicated host
process. The log directory must already exist and the CSV path must be absolute.
Use a fresh CSV for every run; the logger appends to an existing file.

```powershell
$env:PICTVIEW_PERF_LOG = '<LOG_DIRECTORY>\navigation.csv'
& '<TEMP_RUNTIME>\salamand.exe'
```

The setting is read once per process. An already-running host does not inherit
changes to the launching shell's environment. An unset variable disables logging.
Do not run several host processes against the same CSV.

Keep the window dimensions, zoom mode, EXIF autorotation setting and navigation
sequence fixed within each comparison. Record those settings with the aggregate
results. Measure initial opens separately from previous/next navigation. Repeat
each navigation sequence in both directions and collect enough samples to make
the median and p95 useful. Keep initial traversal and repeated traversal in
separate runs. Restarting the viewer does not make the Windows file cache cold.
Avoid concurrent thumbnail generation or other heavy work where practical.

For shareable synthetic inputs, the optional generator requires Python 3.11+,
Pillow and NumPy:

```powershell
python '<CHECKOUT>\tools\pictview\create_perf_fixtures.py' '<FIXTURE_DIRECTORY>'
```

It produces textured synthetic JPEGs at 6, 24 and 48 MP, with EXIF orientation
variants, plus a 24 MP PNG. These are not real photographs. Orientation variants
have identical JPEG image payloads; only the EXIF orientation differs. Keep the
binary fixtures outside the repository. The generator's manifest describes only
these synthetic fixtures; do not create a similar catalog for private images.

## CSV contract and timing boundaries

Each completed sample emits one CSV row. The fields are:

| Fields | Meaning |
| --- | --- |
| `pid`, `tid`, `sample` | Process/thread-local sample identity, used only for validation. |
| `action`, `outcome` | Fixed action (`open` or `navigation`) and completion status. |
| `file_bytes`, `width`, `height` | Encoded file size and raster dimensions; dimensions may reflect applied rotation. |
| `display_width`, `display_height` | Requested display dimensions at drawing time. |
| `total_ms` | Wall-clock interval from instrumented operation entry to completion. |
| `navigation_ms` | Navigation work excluding nested measured phases. |
| `open_other_ms` | Opening work excluding nested measured phases. |
| `decode_ms` | Instrumented WIC frame-decoding work. |
| `exif_ms` | EXIF orientation lookup and transform dispatch, excluding nested decoding. |
| `transform_ms` | Instrumented rotation/flip work. |
| `scale_ms` | Instrumented scaled-frame preparation. |
| `paint_ms` | Instrumented drawing and final `GdiFlush`, excluding nested phases. |
| `other_ms` | Remaining time, including gaps between instrumented scopes. |
| `decode_attempts`, `scale_attempts`, `transform_attempts`, `draw_attempts`, `paint_calls` | Instrumented work attempts and drawing/paint calls; see the counter semantics below. |

The phase timers are exclusive: their sum must match `total_ms` within the
0.02 ms CSV rounding tolerance. A scope temporarily suspends its parent timer.
Decode and scale counters increment after their already-decoded/already-scaled
fast returns: cache hits do not increment them. These are attempts that entered
the work path, not guarantees of success. The transform counter counts backend
pixel-transform attempts; a flag-only 180-degree path can contribute to
`transform_ms` without incrementing that counter. Draw attempts and paint calls
have different scopes and need not have the same count.
`open_other_ms` is not a pure disk-I/O measurement: it can include format probes,
allocation and decoding outside the specifically instrumented WIC path.

Successful `painted` samples finish only after the expected image was drawn and
queued automatic transforms have completed. Timing includes `GdiFlush` but does
not measure DWM presentation, vertical synchronization or physical monitor output.
It also excludes input-queue delay before the navigation handler runs. CSV
formatting and writes happen after the measured interval; they can still perturb
subsequent work, so compare diagnostic runs consistently.

Failures, cancellation, superseded requests and unsupported image kinds have
their own outcomes. They are not successful display-latency samples. Animated,
multipage and interactive-preview behavior needs separate measurement coverage.
This logger does not collect CPU utilization, memory peaks or actual disk traffic.

## Validate and summarize

The summary tool uses only the Python standard library:

```powershell
python '<CHECKOUT>\tools\pictview\summarize_perf.py' '<LOG_DIRECTORY>\navigation.csv' `
  --navigation-only --output-json '<RESULT_DIRECTORY>\summary.json'
```

It validates every row before filtering, rejects incomplete rows, duplicate
process/thread/sample identities, unknown schema or status values, non-finite
or negative numeric values and phase-sum mismatches. It prints anonymous JSON
and optionally writes the same JSON without embedding the input path.

Counts, outcomes and total call counters cover all selected samples. Latency
median, mean, p95, phase means and mean call counters use only `painted` samples.
The p95 is nearest rank, `ceil(0.95 * N)`, rather than interpolation. Phase
percentages are the phase sums divided by the sum of successful total times;
they are not averages of per-sample percentages. Groups use sorted raster
dimensions so portrait and landscape orientations share a resolution group.
Each group includes the encoded-file-size range and sample count. Small groups
and mixed image characteristics still limit what an aggregate can establish.

The CSV and summary omit image names, paths, hashes and EXIF contents. Keep
private image sources, screenshots and runtime history out of source control,
documentation and uploaded artifacts. Only encoded file sizes and raster
dimensions should describe private inputs. Source CSV paths and sample identities
are omitted from aggregate output.

## Validation of the measurement change

The standalone plugin builds in Debug x64 and Release x64 with warnings treated
as errors. The native decoder contract test passes. WIC output tests pass 39
checks per configuration, and thumbnail path tests pass 51 per configuration.
The new timing API test compiles the actual header in two translation units and
checks independent threads, exclusive timers, stale scopes, pending transforms,
disabled logging and anonymous CSV output. Debug and Release each pass 64 native
checks and four CSV validations, including Unicode, raw long and extended-length
log paths. The timing test is registered in `tools/run_native_tests.ps1`.
