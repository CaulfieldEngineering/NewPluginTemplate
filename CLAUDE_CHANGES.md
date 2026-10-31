# Agent changes

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
