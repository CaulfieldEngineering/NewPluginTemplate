# NewPluginTemplate — LLM Orientation

## What this repo is

NewPluginTemplate is the **GitHub template repository** for all new Wonderland Audio plugin projects. It is not a shipping product — it is the standardized starting point that every new plugin repo is cloned from.

The authoritative standards for development conventions live in the **WLA Playbook**, specifically `03-development-pipeline.md`.

## Role in the ecosystem

```
NewPluginTemplate  ──(gh repo create --template)──→  New plugin repo
                                                            ↓
                                                    WizardCore (already wired as dependency)
                                                    CI/CD (pre-configured)
                                                    run_local scripts (ready to use)
```

New repos are created from this template using `NewRepositoryScript/NewRepositoryMac.sh` (or `NewRepositoryWin.bat` on Windows), which also sets up GitHub secrets automatically.

## How to customize for a new plugin

All project-specific values live in **`config.cmake`** at the repo root. This is the only file that needs to change to rename a plugin:

```cmake
set(wPLUGIN_NAME        "MyPlugin")
set(wCOMPANY_NAME       "Wonderland Audio")
set(wCOMPANY_ID         "WonderlandAudio")
set(wPROJECT_VERSION    "0.1.0")
set(wPLUGIN_CODE        "MPLG")   # Unique 4-char code per plugin
```

The run_local scripts, CI/CD pipeline, and installer configs all read from `config.cmake` dynamically — no other files need hardcoded names.

## Directory structure

```
/plugin/source/       ← All plugin C++ source goes here
/plugin/images/       ← Binary assets (images embedded via JUCE BinaryData)
/plugin/sample_audio/ ← Demo audio tracks for the Wonderlab web build
/run_local/           ← Local build scripts (Windows + macOS)
/run_local/QuickScripts/ ← One-click web build + deploy scripts
/cmake/               ← CPM cmake package manager bootstrap
/libs/                ← Dependencies land here (juce, wizardcore — git-ignored)
/package_config/      ← Installer templates (Inno Setup, macOS productbuild)
/.github/workflows/   ← CI/CD pipeline
```

## WizardCore is already wired

WizardCore (shared DSP library) is pulled in automatically via CPM in `CMakeLists.txt` and linked in `plugin/CMakeLists.txt`. To use a module:

```cpp
#include "Chorus/Chorus.h"   // NOT "WizardCore/Chorus/Chorus.h"
#include "LFO/LFO.h"
#include "DelayLine/DelayLine.h"
// WizardCore:: C++ namespace — see WizardCore/CLAUDE.md for full reference
```

The include root is the WizardCore `plugin/source/` directory. Include paths are `Module/Module.h`, not `WizardCore/Module/Module.h`. No CMake changes needed to access existing WizardCore modules.

### Local development vs CI/production

CPM supports a **source override** via the `CPM_WizardCore_SOURCE` environment variable:

| Scenario | `CPM_WizardCore_SOURCE` | WizardCore source | Commit required? |
|----------|------------------------|-------------------|-----------------|
| **Local dev** | Set to your local WizardCore path | Files on disk — uncommitted edits compile immediately | No |
| **CI / production** | Not set | Cloned from GitHub at `GIT_TAG` | Yes |
| **Ship time** | Not set | Cloned from GitHub at a pinned version tag | Yes |

**Set this once on your dev machine** (System Properties → Environment Variables):

```
CPM_WizardCore_SOURCE = C:\Users\jpcfo\Documents\_GitHub\_CaulfieldEngineering\WizardCore
```

Then re-run CMake configure. All plugins will build against your single local WizardCore — one source of truth, no duplicate clones. Edit WizardCore, Ctrl+B in the plugin's VS solution, changes compile instantly.

When ready to push: commit & push WizardCore first, then the plugin. CI runners don't have the env var, so they clone from GitHub automatically.

## Local builds

- **Windows**: `run_local/_runlocalwin.bat` — interactive, prompts for demo/full and web build options
- **macOS**: `run_local/_runlocalmac.sh` — same flow
- **Quick deploy to CE server**: `run_local/QuickScripts/build_deploy_ce.bat`
- **Quick deploy to WordPress**: `run_local/QuickScripts/build_deploy_wp.bat`

Before using QuickScripts, update the deployment configuration block at the top of each script (SSH key path, server address, destination path).

## CI/CD

The GitHub Actions workflow builds on both Mac and Windows automatically on push to `staging`. Code signing credentials are injected via GitHub secrets (set up by NewRepositoryScript).

## Source file registration

Plugin source files are auto-discovered by `plugin/createSourceList.py`, which generates `plugin/sourceListResult.cmake`. You do not need to manually register new `.cpp` files in CMake — just add them to `plugin/source/` and they will be picked up on the next CMake configure.

## Demo / activation system

Every plugin has a built-in demo nag cycle: active for 50 seconds, then bypassed for 10 seconds, repeating. Controlled by:

- `setDemoState(bool)` — called by Wonderlab (`false` = full version) and by serial key activation
- `isEffectEnabled()` — check this before applying DSP; returns `false` during demo bypass windows
- `mIsDemoVersion` defaults to `true` — new plugins start in demo mode

The Wonderlab web build calls `processor->setDemoState(false)` so the web demo always runs as full version. This method **must** exist on the processor or the web build will fail.

## Repo name vs plugin name

The GitHub repository name and `wPLUGIN_NAME` in `config.cmake` can differ (e.g., repo=`concept-fattener`, plugin=`Fattener`). JUCE generates all build artifacts under `wPLUGIN_NAME`. The CI/CD workflow uses `wPLUGIN_NAME` for artifact paths, package names, installer filenames, and web build outputs. It uses the repository name only for server deploy directory paths and URL slugs.

**Never hardcode product names in the workflow, run_local scripts, or package configs.** Everything is driven by `config.cmake`.

## Branch convention

- `staging` — active development branch (default). Pushes here deploy the web app to CaulfieldEngineering.com.
- `main` — production release target. Tags on `main` deploy installers to WordPress and the web app to WonderlandAudio.com.
