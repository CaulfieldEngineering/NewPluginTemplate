# Agent changes

## Native performance host and first-block measurement (Codex, 2026-10-30)

- `plugin/tests/PerfTest.cpp`: ported the independent VST3 performance host from Tertiary's
  `fix-first-block` commit `a89d739` (also present in `99d42ed`). It measures steady DSP, editor CPU/memory and serial instances; `runInstances`
  also prints the first batch and writes `firstUs`, so initialization cost is visible independently
  of warm mean/p99. It also carries the reviewed harness correction: Windows' final 300 microseconds
  use `YieldProcessor` instead of surrendering the thread to `SwitchToThread`, and timing storage is
  allocated before the first scheduled period. No plugin callback is warmed or excluded; workload and
  exit thresholds are unchanged. The final port also carries Windows MMCSS Pro Audio, active-host
  HighQoS/timer policy with restoration, Mac time-constraint/QoS scheduling and the App Nap guard.
  JSON exposes scheduling API errors and distinguishes processing over budget from a late start.
- `plugin/CMakeLists.txt`: adds `PerfTest` under the existing optional `WLA_TESTS` switch. The host
  links JUCE only, loads the supplied built plugin, and gets its window name from `wPLUGIN_NAME`
  through `WLA_TEST_PRODUCT_NAME`. No product processor, template DSP or deployment workflow changed.
- `plugin/tests/README.md` and `tests/README.md`: document the build, commands, first-batch meaning,
  and the generic stereo-effect workload's limits.

Validation: `git diff --check` passed. After normalizing the configurable window title, generic command
help and one demo-limit comment, every line from the includes to the end of the source is identical
to Tertiary `99d42ed`, including timing, scheduling and exit thresholds. Local Windows configure and
final Release build passed with `WLA_TESTS=ON`, copying disabled and two build workers. The generated
build definitions set the display name to `NewPluginTemplate` from config. Python syntax checking of
the changed Cubase helper also passed.

One smoke run each on the final Tertiary RC1c VST3, 48 kHz / 128, two seconds, completed on 2026-10-30
at 23:21 EDT. All three returned exit 0, processed 751/751 blocks, and had zero late blocks or missed
periods. No retries were made. The console first-block metric matched JSON `firstUs`:

| Instances | First batch (us) | Mean (us) | p99 (us) | Maximum (us) |
|---|---:|---:|---:|---:|
| 0 (scheduler control) | 0.0 | 0.03 | 0.1 | 0.3 |
| 1 | 29.5 | 21.5 | 31.6 | 54.5 |
| 30 | 720.2 | 523.4 | 636.7 | 861.1 |

MMCSS registration/priority/revert and timer policy requests/restoration reported no errors. Logs,
JSON and the source-equivalence proof are under local ignored `build-tests/results/first-block-metrics/`.
The loaded RC1c binary SHA-256 was `7098844621f802f627b5ec54031cb569f9018bc9299dc6320222e252e1545d47`.
These brief checks validate the generic host integration, not a 60-second product performance pass or a
Mac runtime result. The inherited zero-instance console memory-per-instance display is NaN; its JSON
contains finite zero total memory and correct timing/control results.

Undo: revert this performance-host commit. The optional executable is not part of any shipping plugin.

## Cubase native export chooser focus (Codex, 2026-10-30)

`tests/win/cubase-tools/cubase_export.py`: restore focus to the already verified native chooser and
use the named Save button's actual input click. The previous message-based click left the chooser open
and timed out while an unrelated application error window was in front. PID, geometry, filename
read-back and no-overwrite checks remain in place. Nothing interacts with the unrelated application.

Validation: this correction in Tertiary's copied local harness completed a fresh Cubase 12 run in
32 seconds, loading the verified RC1c VST3, exporting a bit-identical golden (`max diff 0`, tolerance 0),
and quitting normally. The prior installed VST3 was restored with its original hash. Earlier failed
startup/export attempts and the delayed recovery's differing render remain preserved as evidence.
No Cubase process was killed. This template copy has the same two changed action lines; it was not
run against a nonexistent template-specific Cubase golden.

Undo: revert the chooser-focus change; it is test infrastructure only.

## Browser demo test suite (Codex, 2026-10-30)

Ported the generic browser checks from Tertiary to the `pipeline-sync` work branch, starting at `5382fbe2b71669c7389401cd937ddb80115dcedb`. Plugin sources, DSP, the shipping editor and `.github/workflows/workflow.yml` are unchanged.

| Files | Change and reason |
|---|---|
| `plugin/tests/web/build.mjs`, `paths.mjs` | Build native BinaryData and both Wonderlab web targets, then assemble the demo. Product name/version come from `config.cmake`. Support existing JUCE/WizardCore checkouts and optional `plugin/web_demo.json`; write build provenance. |
| `plugin/tests/web/inspect.cpp` | Add a read-only component/parameter/program/build-ID inspector to test targets only. No parameter setters or DSP changes; the Tertiary-only inspection header is absent. |
| `plugin/tests/web/config.mjs` | Select the passthrough adapter and an empty plugin control/parameter inventory. Configure dependency pins, viewports and pixel tolerances. |
| `plugin/tests/web/run.mjs`, `browser.mjs`, `wrapper-tests.mjs` | Use Playwright gestures and actual audio measurements to check loading, user-gesture audio, bypass equivalence, negative-control RMS stability, fallback audio, layouts, sample playback, upload, pause/resume, stop, seek, loop and removal. Write JSON/HTML reports. |
| `plugin/tests/web/serve.mjs` | Serve the built demo on loopback with COOP/COEP/CORP headers for SharedArrayBuffer. |
| `plugin/tests/web/visual.mjs`, `refs/` | Compare phone/tablet/desktop screenshots against reviewed references. A change above tolerance or a size change fails and writes a diff; CI cannot approve references. |
| `plugin/tests/web/ai-review.mjs` | Optional local read-only `codex exec` image review for differing screenshots. Its advice cannot override the pixel result; absent Codex and CI skip cleanly. |
| `plugin/tests/web/harness.test.mjs`, `package.json`, `package-lock.json` | Pin Node test dependencies and verify screenshot failure/diff behavior, reference-update rules and advisory-review invocation/skip behavior. |
| `plugin/tests/web/README.md` | Explain setup, results, approved references and where future products must add control/audio expectations. |
| `.github/workflows/test.yml`, `.gitignore` | Run the build/browser suite in job `web` on Windows for every eligible work-branch push, including docs-only pushes. Pin Emscripten 5.0.3, Wonderlab `5248a157cfdf472caf1fafc7b9b71dec09485947` and WizardCore `02306c2fce875141e83aa7540b7e4c86ea76081b`. Missing private dependency credentials fail clearly. Upload `out/` as `web-test-results`, including on failure; generated test output is ignored. |

Validation on Windows, using the baseline commit above and Playwright 1.64.0 / Chromium 1248:

- Native configure, `NewPluginTemplateBinaryData`, primary WASM, fallback WASM and demo assembly succeeded.
- Harness tests: **10 passed, 0 failed**. A synthetic changed image failed its pixel check and produced a readable diff; missing Codex skipped cleanly.
- Reference-creation browser run: **33 passed, 0 failed**.
- Repeat browser run without reference updates: **33 passed, 0 failed**; all three screenshots had zero changed pixels, and `out/differences.json` was empty.
- Visually inspected `out/phone-baseline.png`, `out/tablet-baseline.png` and `out/desktop-baseline.png`. The expected gray "Hello World!" editor and player were visible with no clipping or overlap.
- `.github/workflows/workflow.yml` has no diff. CI execution is separate from these local results.

Limits: the template has no effect, parameters, plugin controls, factory presets or scope. Its passthrough checks do not certify a future plugin's sound or control behavior. Future products must replace the passthrough assertions and empty inventory with meaningful checks. The web build also lacks the native `build_id.h`; the hidden editor build line is therefore "unknown". The generic load test verifies the assembled configuration's commit, not that hidden line. No plugin code was changed to hide either limitation.

Undo: revert the browser-test commit. The shipping plugin and deployment workflow do not depend on this suite. Generated files under `build/` and `plugin/tests/web/out/` are disposable; approved references stay in `plugin/tests/web/refs/`.


## Codex 2026-10-30: browser demo fixes and device coverage

- Added production web glue used by local builds, both CI web builds and QuickScripts: desktop build-ID generator for both WASM targets, visual-viewport fitting/refitting, close stale popup menus on rotation, and upload removal clears name/selection/transport and invalidates pending loads. Wonderlab stays pinned and its checkout is not modified.
- Tertiary alone keeps the existing size submenu and About panel, with disabled size/licensing fields and plain hints under JUCE_EMSCRIPTEN. Native plugin behavior remains RC1b. Local full Windows run: all eight groups passed, 90 host checks, all unit/UI/smoke checks, and 63 renders bit-identical to RC1b.
- Added iPhone 13/15 WebKit and Pixel 7 Chromium profiles in both orientations. Windows WebKit has no AudioContext, so CI runs iPhones on macOS using the same Windows-built WASM artifact. ci-ok requires both browser jobs. Real taps, native Chromium touch drags, WebKit TouchEvent handler checks, fit/rotation/visual viewport checks, build identity, disabled activation, continuous audio over a real 65-second wait, explicit sub-40px target warnings, and per-device screenshot references.
- Codex read-only review found and corrected test issues (build separator, touch axis/native injection, visual-only resize and audio observation over the whole timer interval). Final browser counts and screenshots are in plugin/tests/web/out/report.json and report.html; native results are in build-tests/results. No release, staging/production push, or tag.
