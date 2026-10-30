# Use this space to document any future to-do items.

## Pipeline and tests (Claude, 2026-10-29)

- [ ] **UI robot tests and the ui-tests CI work**: port from Tertiary's `ui-tests` branch when it lands (tests that drive the real plugin window; auval and work-branch triggers in its test.yml). Placeholders: the `ui-tests` job in `.github/workflows/test.yml`, the marked step in `plugin/tests/run_all.ps1`, `plugin/tests/ui/README.md`.
- [ ] Rule (Joe, 2026-10-29): when Tertiary's pipeline or tests change, the generic part comes here too, driven by `config.cmake`.
- [ ] Windows installer (`package_config/installer_win.iss.in`) still assumes VST3 and Standalone; make it follow `wFORMATS` like the Mac installer does.
- [ ] Each new plugin: fill in `wWOO_PRODUCT_ID`, write its golden renders (`HostTest --make-golden`), its `render_settings.json`, and its product checks (`ProductTests`, `productChecks()`, `regression/product/`).
- [ ] REAPER harness (`plugin/tests/reaper`) is prepared, not yet run anywhere (see its README).

