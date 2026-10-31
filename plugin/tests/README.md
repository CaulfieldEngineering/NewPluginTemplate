# Tests

Ported from Tertiary (2026-10-29). Everything here reads its names, codes and formats from `config.cmake`; nothing has a product name in it. A new plugin gets all of it by filling in `config.cmake`, then adds its own checks in the places marked PRODUCT.

## What's here

| Path | What | Generic or product |
|---|---|---|
| `PluginTests.cpp` | Unit tests on the processor (JUCE UnitTest). Built as `<wPLUGIN_NAME>Tests` with `-DWLA_TESTS=ON` | Generic: rates and block sizes, no allocation in processBlock, same output at any block size, state restore, the demo cycle. PRODUCT: `ProductTests` at the bottom |
| `HostTest.cpp` | Loads the BUILT plugin (VST3, or the installed AU) as a DAW does. Built with `-DWLA_TESTS=ON` | Generic sections 1 to 5. PRODUCT: `goldenSettings()` and `productChecks()` |
| `PerfTest.cpp` | Independent VST3 host: DSP cost, editor CPU/memory, paced instances with first-block timing. Built with `-DWLA_TESTS=ON` | Generic stereo-effect workload; adapt input/MIDI/layout for products needing another workload |
| `golden/` | Reference renders. `HostTest --vst3 <plugin> --make-golden` writes them; commit them when a sound change is meant | Generic (`default.wav`); add one per product setting |
| `regression/render_matrix.py` | Renders every sample through ReviewRender with every setting in `render_settings.json` | Generic; PRODUCT: the settings file |
| `regression/compare_renders.py` | Compares two sets of renders (one build with another, Windows with the Macs) | Generic |
| `regression/product/` | This plugin's own regression scripts; `run_all.ps1` runs every `.py` here | PRODUCT |
| `regression/examples/` | Tertiary's LFO checks, as examples of a regression script. Not run | Example |
| `run_all.ps1` | The local run on the mini PC: build, pluginval, host test, unit tests, product scripts, renders, one summary | Generic |
| `ci/` | Scripts the workflows call: `config.sh` (reads config.cmake), `mac_checks.sh`, the installer tests, the unsigned self-test installer | Generic |
| `reaper/` | REAPER harness (parked, not yet run). See its README | Generic |
| `ui/` | UI robot tests: placeholder, arriving from Tertiary's ui-tests branch | Placeholder |
| `../review/ReviewRender.cpp` | Runs the processor (and editor) offline. Built with `-DWLA_REVIEW_RENDER=ON` | Generic |

## Run it locally (Windows)

```
powershell -ExecutionPolicy Bypass -File plugin\tests\run_all.ps1
```

Options: `-JuceDir <folder>` to use a JUCE checkout instead of downloading it, `-Reference <folder>` to compare the renders with an earlier build's, `-SkipBuild`. Nothing is installed. Exit code = number of failures.

## Performance host

<!-- [Codex 2026-10-30] Port first-callback evidence without hiding it in a warmed average. -->

Build the optional `PerfTest` target with `WLA_TESTS=ON`, then select the exact VST3 to measure:

```text
PerfTest --vst3 <plugin.vst3> dsp --out dsp.json
PerfTest --vst3 <plugin.vst3> live --seconds 120 --out live.json
PerfTest --vst3 <plugin.vst3> instances --count 30 --block 128 --seconds 60 --out instances.json
```

The instance test reports the first batch explicitly as `firstUs`, alongside mean, p99, maximum,
late blocks and missed periods. That first batch is not warmed up or excluded. Timing includes signal
generation and serial processing of all selected instances; it is distinct from callback-only profiling.
Exit 3 still means a late block or missed period; no threshold is relaxed. Scheduler delays can also cause
late blocks, so preserve the raw result and use a `--count 0` control when investigating them.
The DSP mode intentionally warms each fresh instance for one second before measuring steady-state cost.
The live mode measures each of open/playing, closed/playing and loaded/idle for the selected duration.
The display name comes from `wPLUGIN_NAME` in `config.cmake`; the test does not link product processor code.
The Windows pacing loop uses a CPU pause for its final 300 microseconds instead of yielding the thread
to the scheduler. Its timing vector is reserved before the first deadline starts; no plugin callback is
processed before timing begins.

The host requests the platform's audio scheduling: Windows MMCSS Pro Audio with active-process
HighQoS and honored timer-resolution requests, or Mac time-constraint scheduling with a QoS fallback
and an App Nap guard. Windows process policy and timer-resolution requests are restored on exit.
JSON records scheduling/API errors, `maxStartLatenessUs`, `processingOverBudgetBlocks` and
`lateWithinBudgetBlocks` so a scheduler delay remains visible instead of being counted as plugin work.
These requests do not guarantee deadlines, and the original deadline failure rule still applies.

## Writing a product check

A bug Joe finds gets its check first, then the fix (it fails before, passes after). Put it in `ProductTests` (processor-level), in `productChecks()` (through the host), or as a script in `regression/product/` (through ReviewRender renders). Tertiary's versions are the examples: its LFO tempo checks, band mute and solo, the demo gap, licence states with a pretend server, presets, and an old-session load.
