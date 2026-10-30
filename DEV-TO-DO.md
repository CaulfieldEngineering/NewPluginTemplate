# Use this space to document any future to-do items.

## DAW harnesses (Claude, 2026-10-29)

- [ ] **Cubase**: make the Export Audio step work by automation (the synthetic click on Export Audio does nothing; see `plugin/tests/daw/README.md` and CUBASE-PLAYBOOK.md), then run `assemble_cubase_harness.ps1` once with someone watching.
- [ ] **Other DAWs**, one folder each under `plugin/tests/daw/`, same input, tempo map and automation, goldens in `%USERPROFILE%\.wla-test\goldens\<name>\<daw>\` (approach per DAW in HQ `plans/tertiary-daw-test-plan.md`):
  - [ ] REAPER (`daw/reaper/`): Lua script builds the project and renders from the command line; fold in the parked `plugin/tests/reaper`.
  - [ ] FL Studio (`daw/fl/`): `FL64.exe /R /Ewav` command-line render; needs a licence (the trial cannot reopen a saved project).
  - [ ] Ableton Live (`daw/ableton/`): remote script (AbletonOSC) for transport and parameters; export over screen.
  - [ ] Bitwig (`daw/bitwig/`): screen; check whether the demo can save and export.
  - [ ] Logic Pro (`daw/logic/`, Mac): AU, UI scripting for the bounce.
- [ ] Give the example plugin its own codes: WLAD/EXPL are Tertiary's too, so both plugins have the same VST3 class ID.
