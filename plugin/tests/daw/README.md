# DAW harnesses

This folder holds a test project per DAW: the real DAW opens a saved project with the plugin on a track, renders it,
and compares the render with that machine's golden. It catches what HostTest can't, because it runs inside the actual
host: the host's tempo map and ramps, automation, loops, its scan and its saved state.

Ported from Tertiary on 2026-10-29. Tertiary's working copy is in `plugin/tests/daw/cubase/` of the Tertiary repo, and
its per-DAW plan is in HQ `plans/tertiary-daw-test-plan.md`. Joe: "the plug-in specific test harnesses should probably be
stored in their respective Plug-In repo project folders", "likewise, copied to the NewPluginTemplate folder".

## Layout

```
plugin/tests/daw/
  README.md                         this file
  <daw>/                            one folder per DAW (cubase/ now; reaper, ableton, bitwig, fl, logic later)
    <generator>                     writes the harness's parts from config.cmake: no product name in the scripts
    <run script>                    one unattended run: install the build, open, render, quit, compare
    .gitignore
    <wPLUGIN_NAME> harness/         the DAW project, one per plugin repo (not in this template: built per plugin)
      <project file>                in git
      Misc/                         small build parts, in git (tempo map, track archive)
      Audio/ Runs/ Mixdown/ ...     NOT in git: rebuilt, or this machine's output
```

The rules for every DAW:

- **Names come from `config.cmake`** (`wPLUGIN_NAME`, `wPLUGIN_MANUFACTURER_CODE`, `wPLUGIN_CODE`). Plugin-specific
  inputs live in data files next to the scripts, such as `harness_automation.json`.
- **The same input everywhere.** It is HostTest's golden-render input (`HostTest --write-input`: noise, a sweep, a
  click every 0.25 s, 48 kHz stereo float), repeated to 64 s. The song is 4/4, bars 1 to 13 (about 24.8 s): 120 BPM,
  a ramp to 90, then a jump to 140.
- **Large or generated files are not committed:** input audio, peak files, exports, run folders.
- **Goldens are per machine** (that DAW, that PC), so they live outside the repo at
  `%USERPROFILE%\.wla-test\goldens\<plugin name, lower case>\<daw>\` (on a Mac, `~/.wla-test/goldens/...`).
- **Screen-control rules** (HQ `agent/README.md`): check the front window after every step that can raise a window.
  An unknown window means a screenshot and a stop, with nothing clicked. Save, overwrite and install prompts are never
  answered. The DAW is never killed.
- Renders are compared by `plugin/tests/regression/compare_renders.py`, which arrives with the pipeline sync.
  HostTest also comes from there.

## Cubase (`cubase/`)

| File | What | Status |
|---|---|---|
| `make_cubase_harness.py` | `info`, `parts` (input WAV, tempo `.smt`, track archive `.xml`), `template` | Works. The parts for this template's example plugin were generated and checked 2026-10-29, and a round trip through Tertiary's lanes matched Tertiary's own archive |
| `track-template.xml` | Cubase's own Selected Tracks export of one audio track with a JUCE plugin in insert 1. The plugin, its saved state, its lanes and its paths are placeholders | Made from Tertiary's export with `template` |
| `harness_automation.json` | Which parameters get lanes, as JUCE parameter IDs with points | Empty: the example plugin has no parameters |
| `assemble_cubase_harness.ps1` | Imports the parts into a new Cubase project and saves `<name> harness.cpr` | **Not yet run as one script.** Its steps built Tertiary's harness by hand on 2026-10-29. The pixel positions are the mini PC's |
| `run_cubase_harness.ps1` | Installs the build per user, opens the harness, exports, quits with Ctrl+Q, compares with the golden | **Export not working yet** (below) |

To build a harness in a new plugin repo:

```
python plugin\tests\daw\cubase\make_cubase_harness.py info
python plugin\tests\daw\cubase\make_cubase_harness.py parts          # needs a HostTest build (run_all.ps1) or --input <wav>
powershell -File plugin\tests\daw\cubase\assemble_cubase_harness.ps1
powershell -File plugin\tests\daw\cubase\run_cubase_harness.ps1      # later: -MakeGolden after listening to a good render
```

Then commit `<name> harness/<name> harness.cpr` and its `Misc/*.smt` and `Misc/*.xml`. After cloning, `parts` rebuilds the
audio. Moving the project folder is fine: on 2026-10-29 Cubase found `Audio/harness-input.wav` relative to the moved
project, with no prompt.

**What the VST3 class ID is:** for a JUCE plugin it is `ABCDEF01 9182FAEB` + the manufacturer code + the plugin code,
so `make_cubase_harness.py` points the track at any plugin without Cubase. A Cubase lane tag for a parameter of the
plugin in insert 1 is the VST3 parameter ID + 4201. JUCE's VST3 parameter ID is `juce::String::hashCode()` of the
parameter ID, masked to 31 bits.

**Not yet working:**

- **The Export Audio click.** Driven by automation, the Export Audio Mixdown dialog opens with the right settings,
  but the synthetic click on **Export Audio** does nothing: no progress window and no file. This happens in every
  project on the mini PC, including a new empty one, so it is not the plugin. Everything that was tried is listed in
  `CUBASE-PLAYBOOK.md` (Joe's pipeline docs) under "Tertiary harness run, and where it stopped". Until it is solved,
  `run_cubase_harness.ps1` stops at the export and no golden can be made.
- **Codes:** this template's `wPLUGIN_MANUFACTURER_CODE`/`wPLUGIN_CODE` (WLAD/EXPL) are the same as Tertiary's, so its
  example plugin has the same VST3 class ID. Don't install the example plugin on a machine whose Cubase has
  Tertiary. Give every plugin its own code first.

**Depends on Joe's generic Cubase helpers**, which are not in this repo. `cubase_windows.ps1`, `cubase_wait.ps1`,
`cubase_menu.ps1`, `cubase_import.ps1` and `screen.ps1` are in `Dropbox\Joe's Music\_Production\_PIPELINE\tools`.
Override the location with `-Tools <folder>` or `$env:WLA_CUBASE_TOOLS`. The scripts check that the helpers are
there before starting.

## Other DAWs

These are not started yet; see `DEV-TO-DO.md`. The pipeline sync brings `plugin/tests/reaper/`, a REAPER render of the
golden input that is prepared but parked. A full REAPER harness, with a tempo map and automation, would move here as
`daw/reaper/`. Tertiary's test plan already says how each DAW can be automated:
REAPER through a Lua script and a command-line render, FL Studio through a command-line render, Ableton Live
through a remote script, Bitwig and Logic over screen control.
