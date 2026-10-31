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
| `wCHANGELOG_PAGE_ID` | The WordPress page that shows `CHANGELOG.md` (see "The changelog" below). Empty: no changelog page, the step is skipped |
| `wPRODUCT_SLUG` | The product's short lower-case name (empty: `wPLUGIN_NAME` in lower case); `scripts/draft-changelog.mjs` reads `HQ/plans/<slug>-test-plan.md` |
| `wWEB_STAGING_DIR`, `wWEB_LIVE_DIR` | The web demo's folders under `CaulfieldEngineering.com/plug-ins/<repo>/` (empty: `web-staging` and `web`). Staging deploys only to the first; a tag promotes it to the second and to `WonderlandAudio.com/demo/<repo>/` (see "Release flow") |

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
3. **publish** (tag only, after both builds): both installers or neither. Hash check, check the store product again, upload, re-download and compare, point the download redirects at the new files, remove older copies of the same version (older versions stay), publish the changelog page and the home page's version line from `CHANGELOG.md` (below), and last of all set the product's **Software version** (what the plugin's version check reports) and read it back.
4. **web-build**: the Wonderlab demo. Staging deploys only the private preview; a tag, after publish, promotes it to the live demos (see "Release flow" below).
5. **cleanup-failed-tag**: a failed release deletes its tag so it can be pushed again, except when the tag check never reached a verdict, when publishing stopped part-way, or when the tag was moved.

Secrets (repository settings): `AZURE_TENANT_ID`, `AZURE_CLIENT_ID`, `AZURE_CLIENT_SECRET`, `AZURE_ENDPOINT`, `AZURE_CODE_SIGNING_NAME`, `AZURE_CERT_PROFILE_NAME`, `MAC_APPLICATION_CERTIFICATE_BASE64`, `MAC_INSTALLER_CERTIFICATE_BASE64`, `MAC_CERTIFICATE_PASSWORD`, `KEYCHAIN_PASSWORD`, `APPLE_ID`, `APPLE_TEAM_ID`, `APPLE_APP_SPECIFIC_PASSWORD`, `WORDPRESS_USERNAME`, `WORDPRESS_APPLICATION_PASSWORD`, `WORDPRESS_SITE`, `WORDPRESS_SFTP_PASS`, `CE_GITHUB_PAT`; optional `SSH_ID` (CaulfieldEngineering.com copies).

### Release flow

[Claude 2026-10-30] Joe: staging is "a place to view what WILL/SHOULD be customer-facing with one last look before its live". Nothing public changes until the tag.

- **Push to staging**: the builds, tests, signing and verify steps; the installers go to `CaulfieldEngineering.com/plug-ins/<repo>/installers-staging/` (not linked from the store; a tag uses `installers/`). The web demo goes only to the private preview, `https://caulfieldengineering.com/plug-ins/<repo>/web-staging/`, which sends `X-Robots-Tag: noindex` and is linked from nowhere; the run's summary prints its URL and commit. The store, the download redirects, the changelog and home pages, the WooCommerce Software version and both live demos are not touched.
- **v\* tag**: publish (installers to WordPress, redirects, older copies removed, changelog and home pages, Software version). Then, only after publish succeeded, the web demo: if the staging preview was built from the tag's commit, or from a commit with the same source tree (the usual merge of staging into production), and every file matches its checksum (`build-manifest.sha256`, `build-commit.txt`), those exact files go to `WonderlandAudio.com/demo/<repo>/` (the "Try it in your browser" iframe) and `CaulfieldEngineering.com/plug-ins/<repo>/web/` (the folder it replaces is kept as `web.previous`). Otherwise the tag run's own build goes live and the summary says why. Files the previous release listed and this one does not are removed from WordPress. A re-run of an older tag never deploys its demo over a newer release, and an older staging run never replaces a newer preview.

### The changelog

[Claude 2026-10-30] Joe: "Ideally the changelog gets auto-pushed and it's not Claude or Joe updating each time", written "ideally by AI but I'd like user-facing language". `CHANGELOG.md` is the only place it is written; the site's changelog page and the home page's version line are filled from it on every release.

1. **Draft** at the release candidate, on the mini PC: `node scripts/draft-changelog.mjs` (`--dry-run` only prints). It reads what changed since the last `v*` tag (commit messages, `CLAUDE_CHANGES.md` if there is one, the regression list in `HQ/plans/<wPRODUCT_SLUG>-test-plan.md`) and has `claude -p` write the entry for a musician: at most 8 bullets, no internal IDs or code words, no em-dashes, nothing that isn't in the changes. It goes in as `## <version> (release date)` with a `<!-- draft ... -->` line under it.
2. **Approve**: Joe edits the words if he likes and deletes the `<!-- draft` line.
3. **Release**: preflight stops a tag whose entry is missing or still a draft, before anything is built. In publish, after the installers are live, `.github/scripts/changelog.py publish` dates the entry (a written date wins; `(release date)` becomes the tag's date in New York time), finds the previous version's installers in the media library (titled `<wPLUGIN_NAME> Installer v<x.y.z> for Mac` / `for Windows`, exactly as the upload step titles them), writes every entry up to this version into the changelog page's block with "Need the previous version? <x.y.z> for Mac · Windows" under the newest, sets "<version>, released <date>" in the home page's version line, clears Elementor's cache, reads both back and checks that the live pages read exactly what was written. Any mismatch fails the job. Every entry shown must be approved; each page is read fresh right before its own write, so an edit Joe saved meanwhile is kept; if the home page can't be written only the changelog block is put back (never the rest of the page); a re-run keeps the date already published.

On the site the pipeline writes only: on the changelog page, the one Text Editor widget whose CSS Classes include `wla-changelog` (replaced whole; everything around it is Joe's), or on a page not built with Elementor the text between `<!-- wla-changelog:start -->` and `<!-- wla-changelog:end -->`; on the front page, the number and date in the one Text Editor widget with CSS class `wla-version-<wPRODUCT_SLUG>`, if it has one (each product has its own class, so plugins sharing a site never touch each other's version line; none is needed if the front page has no version line for this product). Tests: `python3 .github/scripts/test_changelog.py` (fake site; CI runs it). A dry run against saved page data: `python .github/scripts/changelog.py publish <x.y.z> --dry-run --release-date <YYYY-MM-DD> --page-id <id> --plugin-name <name> --page-json <file> [--home-json <file>] [--media-json <file>] --out <dir>`.

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
| 11 | **Post-release verification** | Stage 10 done | `npm run post-release -- --product <slug> --version <x.y.z>` in wla-site-tests (below): the website pass, both installers downloaded from the live site with SHA-256 against the tag's signed CI artifacts, signatures and notarization, then install and use on Windows and the cloud Mac | auto | Dev PC, cloud Mac | Pull the download links back to the previous version (Redirection), then fix |

### After a release (stage 11)

[Claude 2026-10-30] Joe: "Once its released to the live site, i'd like for you to always one more website pass to make sure that things like version numbers and change log etc got updated. Then download the installer from the website and just do one more install/use pass." The script is not in this repo: it lives in `wla-site-tests` (`post-release.mjs` and `mac-post-release.sh`), which serves every product. For a new plugin, add a `release` section to its `config/products/<slug>.mjs` there (copy Tertiary's), then after each tag:

```
npm run post-release -- --product <slug> --version <x.y.z> [--tag-run <GitHub run id>]
```

It checks the home page's version line (`wla-version`), the changelog page's newest entry and its previous-version links, the download redirects, the store's Software version (`wWOO_PRODUCT_ID`) and `version_check`, then runs the read-only web test; downloads both installers as a customer and compares their SHA-256 with the signed artifacts of the tag's `workflow.yml` run (artifacts are kept 90 days, so run it soon after the release); checks the Authenticode signer (Azure Trusted Signing); installs silently on Windows and leaves it installed, checks the version resources, loads the VST3 in HostTest and runs `HostTest --live-license` with a test key. `mac-post-release.sh` does the Mac half on the cloud Mac: `spctl -a -vv -t install`, `stapler validate`, install, the Info.plist versions, `auval`. A report and a one-paragraph summary; exit 1 on any failure.

Details of each test folder: `plugin/tests/README.md`.


### Independently runnable test layout (Codex, 2026-10-30)

`tests/sequence.json` carries the Blocks IDs, CI flags and suite relations. `tests/win/Run-All.ps1` and
`tests/mac/run-all.sh` call the same row scripts used in both GitHub workflows (`ci` profile). Each calls its
platform's `Run-All-Product-Tests.ps1` / `run-all-product-tests.sh`, which discovers files in its sibling
`product/` folder. Both template product folders contain only a README and report "no product tests yet";
write those tests in each new plugin. The existing generic processor invariants remain at the platform root.
Reports, binary identities, comparison with prior CI counts, exclusions, and product summaries live under
`tests/results/`. Test/staging artifacts expire in seven days; release installer retention is unchanged.
See [the test-script guide](tests/README.md) for commands and the result contract.
