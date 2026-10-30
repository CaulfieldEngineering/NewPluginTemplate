# Tests

Ported from Tertiary (2026-10-29). Everything here reads its names, codes and formats from `config.cmake`; nothing has a product name in it. A new plugin gets all of it by filling in `config.cmake`, then adds its own checks in the places marked PRODUCT.

## What's here

| Path | What | Generic or product |
|---|---|---|
| `PluginTests.cpp` | Unit tests on the processor (JUCE UnitTest). Built as `<wPLUGIN_NAME>Tests` with `-DWLA_TESTS=ON` | Generic: rates and block sizes, no allocation in processBlock, same output at any block size, state restore, the demo cycle. PRODUCT: `ProductTests` at the bottom |
| `HostTest.cpp` | Loads the BUILT plugin (VST3, or the installed AU) as a DAW does. Built with `-DWLA_TESTS=ON` | Generic sections 1 to 5. PRODUCT: `goldenSettings()` and `productChecks()` |
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

## Writing a product check

A bug Joe finds gets its check first, then the fix (it fails before, passes after). Put it in `ProductTests` (processor-level), in `productChecks()` (through the host), or as a script in `regression/product/` (through ReviewRender renders). Tertiary's versions are the examples: its LFO tempo checks, band mute and solo, the demo gap, licence states with a pretend server, presets, and an old-session load.
