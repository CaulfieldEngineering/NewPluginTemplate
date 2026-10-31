# Browser demo tests

<!-- [Codex 2026-10-30] Build, run and maintain the browser checks without changing the approved plugin. -->

This suite builds the template's browser demo using the same native BinaryData, Wonderlab and Emscripten sequence as the `web-build` deployment job. The template baseline is `5382fbe` on the `pipeline-sync` work branch. Product names and versions come from `config.cmake`. These test changes do not authorize a staging push, tag or release.

The shipping plugin sources are unchanged. `inspect.cpp` is added only to the generated test build targets. It reads JUCE component bounds, control state, parameters, presets and the compiled build ID. It has no parameter setters, timer overrides or DSP replacements. Playwright sends the mouse and keyboard gestures. The test also calls Wonderlab's existing audio entry points with the live AudioContext suspended; the plugin still processes the samples itself. The inspector is not added to deployment builds.

## Windows setup

Use Node 24 (Node 20 or later is required), Git, CMake, Python 3, Ninja, FFmpeg, and Visual Studio's C++ build tools. The native compiler builds the JUCE BinaryData generator before Emscripten builds WebAssembly. Put `cmake`, `python`, `ninja` and `ffmpeg` on PATH. CI uses Windows so its Chromium rasterization matches the Windows references.

Install Emscripten **5.0.3**, the version pinned in the deployment workflow. These commands run in Command Prompt; `call emsdk_env.bat` activates the SDK in that same shell. Installation and activation do not require prompts.

```bat
git clone https://github.com/emscripten-core/emsdk.git C:\Users\Joe\tools\emsdk
call C:\Users\Joe\tools\emsdk\emsdk.bat install 5.0.3
call C:\Users\Joe\tools\emsdk\emsdk.bat activate 5.0.3
call C:\Users\Joe\tools\emsdk\emsdk_env.bat
```

If the SDK checkout already exists, omit the clone. Activate the pinned version again in each new build shell. Do not install or activate `latest` for these references.

From the worktree root, get the pinned private Wonderlab and WizardCore dependencies in this worktree's ignored build folder. Access uses the developer's existing Git authentication; do not place credentials in scripts or the repository.

```bat
git clone https://github.com/CaulfieldEngineering/Wonderlab.git build\Wonderlab
git -C build\Wonderlab checkout 5248a157cfdf472caf1fafc7b9b71dec09485947
set WONDERLAB_DIR=%CD%\build\Wonderlab
git clone https://github.com/CaulfieldEngineering/WizardCore.git build\WizardCore
git -C build\WizardCore checkout 02306c2fce875141e83aa7540b7e4c86ea76081b
set WIZARDCORE_DIR=%CD%\build\WizardCore
cd plugin\tests\web
npm ci
npx playwright install chromium
npm run build
```

Existing dependency checkouts can be selected with `WONDERLAB_DIR` and `WIZARDCORE_DIR`. Keep their commits at the pins above. `WIZARDCORE_DIR` supplies the native CPM source and the web include/source folders. `CPM_JUCE_SOURCE` can select an existing native JUCE checkout; the WebAssembly build uses Wonderlab's JUCE fork. `WEB_BUILD_DIR` overrides `build/web`, and `WEB_BUILD_JOBS` overrides the default four compiler jobs. `plugin/web_demo.json` is optional. The build records the product name, version, commit, SDK and Wonderlab commit in `build/web/test-build.json`.

## Run and inspect

```bat
npm run test:harness
npm test
```

The runner starts its own loopback HTTP server and headless Chromium, then closes them. `npm run serve` keeps a manual preview open at `http://127.0.0.1:4173`; set `PORT` to change that port. The server sends COOP `same-origin`, COEP `require-corp`, and CORP `same-origin` for the shared-memory audio build. The runner also exercises the fallback without SharedArrayBuffer.

Phone, tablet and desktop viewports are 390x844, 820x1180 and 1440x900. The checks inspect loading errors, actual audio after a user gesture, measured passthrough output against bypass, build identity, layout and player controls. This template paints "Hello World!" and has no plugin controls, parameters, presets or scope. The passthrough adapter explicitly expects that empty inventory; it does not claim to test a future effect's sound or controls. Browser requests to the analytics hosts explicitly listed in `config.mjs` are fulfilled with an empty script so a local test sends no analytics. Plugin assets, fonts, audio and other UI requests are not excluded from the failure checks.

`out/` contains `report.html`, `report.json`, screenshots, pixel diffs, `controls.json`, `differences.json` and `ai-review.md`. CI also saves the build log there and uploads the folder as `web-test-results`, including on failure. The job is `web` in `.github/workflows/test.yml`; it runs on work-branch pushes. CI requires `CE_GITHUB_PAT` to read Wonderlab and WizardCore and fails explicitly if the secret is absent. It runs `npm test -- --no-ai`. `workflow.yml` is unchanged.

Read the latest `report.json` for counts and individual failures. This README does not certify a successful run. A product defect remains a hard failure; record its screenshot path and report it without editing the plugin to make this task pass.

The existing web build does not provide the native generated `build_id.h` to the plugin. Its optional hidden build line therefore reports "unknown". The generic load check verifies the assembled page's `WONDERLAB_CONFIG.buildId` against `test-build.json` and the checkout; it does not certify that hidden line. The test harness does not inject a header to disguise this build limitation.

## References and visual review

`refs/` holds the reviewed template screenshots and `build.json` provenance. Reference creation is an explicit action:

```bat
npm run refs
npm test
```

Use that command only for an intentionally approved visual baseline after inspecting the screenshots. CI cannot update references. Missing references fail normal runs. Pixel comparison uses a per-pixel threshold of 0.12 and permits at most 0.001 of the image pixels to differ. Larger differences and any dimension change fail and produce a diff PNG. These values live in `config.mjs`.

A reference records appearance, not functional approval. Recording its image does not waive clipping, unreachable controls, audio failures or other behavioral assertions. Do not refresh references merely to hide a regression.

For each differing screenshot, a local run invokes `codex exec --sandbox read-only` with the reference and actual image attached. The prompt asks about clipping, overlap, colours, missing scope and whether the change looks broken, plausibly intended or uncertain. Its verdict is advisory and cannot turn a failing pixel check into a pass. The configured Codex model is inherited. No additional model or API key is selected by this suite.

The review skips cleanly in CI, with `--no-ai`, when Codex is unavailable, or when images match. Authentication errors and timeouts are recorded as unavailable reviews, not invented verdicts. `npm run review` reruns this advisory step from `out/differences.json`. The report is `out/ai-review.md`.

## Adapting another plugin

Product names and versions come from `config.cmake`; `config.mjs` supplies processor header/class, dependency pins, viewports, visual tolerance and product-specific expectations. The build, server, inspection, visual comparison and advisory review code is shared with Tertiary. When adding an effect, replace the passthrough adapter with meaningful audio assertions, declare its control inventory, add gestures for its controls and review new references. Tertiary's scope inspection header and its references are deliberately absent here. A generic template build is not evidence that a new plugin's controls or sound are covered.
