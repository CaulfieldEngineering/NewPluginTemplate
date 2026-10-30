# UI robot tests (placeholder)

Arriving from Tertiary's `ui-tests` branch (being built 2026-10-29): tests that drive the real plugin window, every control, the menus, the About window and its build number, the licence box, presets, the bugs Joe found in the UI, and opening and closing the window many times. That branch also adds auval and work-branch triggers to Tertiary's test.yml.

When it lands in Tertiary, port it here the same way as the rest of `plugin/tests`: names from `config.cmake`, Tertiary's own scenarios as marked examples, and:

- a job in `.github/workflows/test.yml` (the `ui-tests` placeholder job is there now)
- a step in `plugin/tests/run_all.ps1` (marked there)
- a row in the README's stage table (stages 1 and 3)

Tracked in `DEV-TO-DO.md`.
