# Overview

Basic description of the plug-in goes here.

# Features

- Feature 1 Name: Feature 1 Description
- Feature 2 Name: Feature 2 Description
- Feature 3 Name: Feature 3 Description

# Tests and the release pipeline

Ported from Tertiary, 2026-10-29. A plugin made from this template gets the whole pipeline and test harness by filling in `config.cmake`; nothing in the workflows or test scripts names a product.

## What config.cmake drives

| Key | Used by |
|---|---|
| `wPLUGIN_NAME` | Every target, file and installer name: `<name>_VST3`, `<name>Tests`, `<name>MacInstaller_<version>.pkg`, `<name>WinInstaller_<version>.exe`, the web build, the test scripts |
| `wCOMPANY_NAME`, `wCOMPANY_ID` | Bundle ID (`com.<id>.<name>`), the Mac package identifiers, the Windows install folder |
| `wPROJECT_VERSION` | The plugin version; a `v*` tag must match it |
| `wPLUGIN_CODE`, `wPLUGIN_MANUFACTURER_CODE`, `wIS_SYNTH` | The AU's identity: auval (`aufx` or `aumu`) and HostTest find the AU by these codes |
| `wFORMATS` | Which bundles are built, signed, packaged, verified and tested (VST3, AU, Standalone). The Mac installer's choices are generated from it (`cmake/installer_formats.cmake`) |
| `wMACOS_MIN_VERSION` | The oldest macOS (set before `project()`); the Mac CI checks every binary against it |
| `wWOO_PRODUCT_ID` | The WooCommerce product. Required for a release: its key source is checked before publishing, and its Software version is set after |

Write one `set(wKEY value)` per line with no comment after it: the workflows read the file line by line.

## Branches

A **work branch** (any name), then **staging**, then **production**, then a **v\* tag** on production, which publishes.

| Workflow | Runs on | Does |
|---|---|---|
| `test.yml` | every push to a work branch; by hand | Windows, Apple Silicon and Intel Macs: build, unit tests, host test, auval, pluginval, golden renders, renders compared across the three machines, the web demo compiles. Never signs or deploys |
| `workflow.yml` | push to staging; v* tags | The release pipeline: preflight, signed builds and installers, tests on the release build, verify, publish (tags only). See below |
| `installer-test.yml` | after each staging pipeline run | Installs the signed installers on fresh Windows and Mac machines: signatures, Gatekeeper, files in place, auval, pluginval, host test, standalone starts, uninstall clean |

## The release pipeline (workflow.yml)

1. **preflight**: on a tag, the tag must be on `production` (a GitHub fetch problem keeps the tag and asks for a re-run, it never deletes it), match `wPROJECT_VERSION` and be newer than every tag. Every release secret must be set: Azure signing, Apple signing, notarization, WordPress, the web demo, plus `wWOO_PRODUCT_ID`. The store product must have a working key source (a "Generator rule" with no generator fails). On staging a missing secret is a warning and that step is skipped, so staging builds can be unsigned.
2. **build** (Mac and Windows): the build, then the unit tests and pluginval on that very build, sign, package, notarize, staple, then **verify** the final installers: the Mac pkg's signature, stapled ticket and Gatekeeper, and every bundle inside it; Authenticode on the Windows installer and binaries. A `.sha256` travels with each installer.
3. **publish** (tag only, after both builds): both installers or neither. Hash check, check the store product again, upload, re-download and compare, point the download redirects at the new files, remove older copies, and last of all set the product's **Software version** (what the plugin's version check reports) and read it back.
4. **web-build**: the Wonderlab demo; on a tag it goes live only after publish.
5. **cleanup-failed-tag**: a failed release deletes its tag so it can be pushed again, except when the tag check never reached a verdict, when publishing stopped part-way, or when the tag was moved.

Secrets (repository settings): `AZURE_TENANT_ID`, `AZURE_CLIENT_ID`, `AZURE_CLIENT_SECRET`, `AZURE_ENDPOINT`, `AZURE_CODE_SIGNING_NAME`, `AZURE_CERT_PROFILE_NAME`, `MAC_APPLICATION_CERTIFICATE_BASE64`, `MAC_INSTALLER_CERTIFICATE_BASE64`, `MAC_CERTIFICATE_PASSWORD`, `KEYCHAIN_PASSWORD`, `APPLE_ID`, `APPLE_TEAM_ID`, `APPLE_APP_SPECIFIC_PASSWORD`, `WORDPRESS_USERNAME`, `WORDPRESS_APPLICATION_PASSWORD`, `WORDPRESS_SITE`, `WORDPRESS_SFTP_PASS`, `CE_GITHUB_PAT`; optional `SSH_ID` (CaulfieldEngineering.com copies).

## The whole process, stage by stage

Each stage runs its own tests, and nothing moves to the next stage until the current one is green. A failure stops the line: fix it, then re-run that stage from the top.

| # | Stage | Triggered by | Tests | Who | Where | If it fails |
|---|---|---|---|---|---|---|
| 1 | **Local tests** | A change on the work branch. A bug found gets its regression check written first | `plugin/tests/run_all.ps1`: build, unit tests, host test (including golden renders), product regression scripts, renders compared with the last release, pluginval. UI robot tests once ported | auto | Dev PC | Fixed on the spot, then stage 1 again |
| 2 | **Feel check** | A passing build goes to Joe (the hidden build number, Ctrl+Shift+click, confirms which) | Joe checks feel on what changed. Feel only: anything repeatable moves to stage 1 | Joe | Dev PC or laptop | What he finds becomes a new regression check, then back to stage 1 |
| 3 | **Push** to the work branch | The push | `test.yml` on Windows and both Macs: the stage 1 tests plus auval, the AU, and renders compared across the three machines | auto | GitHub | Usually a Mac-only problem: back to stage 1 |
| | *Stages 1 to 3 repeat for every change until the release's work is done* | | | | | |
| 4 | **Code lock**, then **merge into staging** | The work is done | `workflow.yml` on staging: the real signed builds and installers, tests on the release build, the verify steps; then `installer-test.yml` installs each installer on fresh machines | auto | GitHub | Fix on the work branch (stages 1 to 3), merge again |
| 5 | **Release-candidate tests** on the staging build | Stage 4 green | Performance, smoke test, the DAW harnesses (REAPER harness in `plugin/tests/reaper`) | auto | Dev PC | As stage 4 |
| 6 | **Mac check** | Stage 5 green | The signed installer on a real Mac, auval, a DAW on Mac (a rented cloud Mac when there is no Mac at hand) | auto | Cloud Mac | Fix and re-run |
| 7 | **Joe's feel pass** on the staging build | Stage 6 green | Window sizes, the controls by ear, anything visual | Joe | His DAW | New regression check, fix, back to stage 4 |
| 8 | **Test sale** | Stage 7 done | Buy, the key arrives, it activates, a refund revokes it | Joe + auto | Live store | Fix, then stage 8 again |
| 9 | **Sign-off**, then **merge staging into production** | Everything above green | Sign-off with the build id and every result. The merge runs no tests: production is exactly the staging code that passed | auto | Local, GitHub | n/a |
| 10 | **Release: v\* tag on production** | **Joe says go** | `workflow.yml` on the tag: preflight, build, tests, sign and notarize, verify, publish, the Software version last | auto | GitHub | Nothing is published (or, if publishing stopped part-way, the tag is kept and the run says what to check). Fix, then back to stage 4 |
| 11 | **Live check** | Stage 10 done | Download from the live site and install; the build id matches the signed-off one | auto | Dev PC, cloud Mac | Pull the download links, then fix |

Details of each test folder: `plugin/tests/README.md`.
