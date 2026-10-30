# Release checklist (every Wonderland Audio plugin)

Joe, 2026-10-30: "I want to make sure we have this documentation updated in the Tertiary Test Plan, and then ultimately make a comprehensive testing checklist for when each version goes live."

Tick through it for every version of every plugin. Each line is **who** · **where or the command** · **passes when**. Test IDs (T1, T2, R, D, W, PR) point at the product's own test plan (Tertiary's is `HQ/plans/tertiary-test-plan.md`); this page does not repeat them. Stage numbers are the stages 1 to 11 in this repo's README, "The whole process, stage by stage". The live copy of this checklist is `HQ/plans/release-checklist.md`; change it there and copy it here.

- **Who:** Claude; Joe; auto = the pipeline runs it by itself.
- **Fill in:** `<plugin>` (the product name), `<slug>` (its product in wla-site-tests), `<x.y.z>` (this version), `<prev>` (the version before).
- **Order:** A to H, except C's last line (go), which waits for D and E.
- **A failure stops the line.** Fix it on the work branch, then back to the stage the test plan names. Joe-found bugs get an R-xx check first.
- **Mac and Windows release together.** Never one without the other.
- **Nothing ships Joe doesn't understand.** No `v*` tag without Joe's go, in his words.

## A. Before code lock

- [ ] **First release of a new product only** (not updates): training video, written companion under the product's Blocks page, quiz. Claude makes them, Joe takes the quiz · Blocks, product page · Joe passed; the release is blocked until then
- [ ] **Scope frozen.** Joe · the release's project board in Blocks and the day's master list · Joe says "lock"; nothing open for this release except tests and release steps; new ideas parked as tasks, not added
- [ ] **Version set.** Claude · `wPROJECT_VERSION` in `config.cmake` · equals `<x.y.z>`, newer than every tag
- [ ] **Side branches in.** Claude · release branch · every branch meant for this release merged, each green on GitHub first
- [ ] **Pipeline gaps closed.** Claude · test plan, "Gaps in the pipeline" · each one fixed or marked fine by Joe
- [ ] **Claude's changes listed.** Claude · `CLAUDE_CHANGES.md` · every change has file, function, how tested, how to undo
- [ ] **Changelog drafted.** Claude · `node scripts/draft-changelog.mjs` in the plugin repo · `CHANGELOG.md` has `## <x.y.z> (release date)` with the `<!-- draft` line; at most 8 bullets, user words, no internal IDs, no em-dashes, nothing that isn't in the changes
- [ ] **Store and site ready.** Claude · WooCommerce product, Redirection, Elementor · price right; key source auto generated (prefix, limit, lifetime); `wWOO_PRODUCT_ID` set; download redirects `/<plugin>-download-mac` and `-windows` exist; one `wla-version` widget on the home page; exactly one `wla-changelog` widget on the changelog page; `config/products/<slug>.mjs` in wla-site-tests has its `release` section

## B. Release candidate

- [ ] **Local tests (stage 1).** Claude · `plugin/tests/run_all.ps1` on the release branch head · T1-01 to T1-11 and every R-xx pass
- [ ] **GitHub on three machines (stage 3).** auto · Actions, test workflow · T1-12 and T1-13 green on Windows, Apple silicon and Intel
- [ ] **Code lock, merge into staging (stage 4).** Claude, on Joe's "lock" · `workflow.yml` on staging, then `installer-test.yml` · signed builds and installers green; T2-18, T2-19 (first pass), T2-20
- [ ] **RC build id written down.** Claude · the staging commit · every later test names this build; a new commit means a new RC
- [ ] **Release-candidate tests (stage 5).** Claude · mini PC · T2-00 to T2-03 within limits (no worse than the last release by more than 10%); every DAW harness that is built (D-xxx) passes
- [ ] **Web test (stage 5).** Claude · `npm test` in wla-site-tests · exit 0, or 2 with only known failures Joe has accepted
- [ ] **Codex on the web results.** Claude · `codex exec --sandbox read-only` with the screenshots and findings · the report to Joe says what Codex caught and what was done
- [ ] **Any site change for the release** follows the site rules. Claude · Elementor · backup first; `npm test` before and after; CSS kept in Elementor is fine, no PHP or theme files (if one is unavoidable: tiny, named, with how to turn it off); where to adjust it in Elementor written down
- [ ] **RC sign-off table started.** Claude · test plan, "Release candidate N" · build id and each stage's result so far

## C. Joe's sign-off

- [ ] **Feel pass (stage 7).** Joe · his DAW on the RC build (a ready test project that loads the latest build); Ctrl+Shift+click the version to confirm the build id · his feel rows (T2) look, sound and feel right. Anything off: new R-xx, fix, back to stage 4
- [ ] **Harness listen.** Joe · the DAW harness reference recording (`C:\Users\Joe\.wla-test\goldens\<slug>\<daw>\harness.wav`) · sounds right once; it stays the reference
- [ ] **Changelog skim.** Joe · `CHANGELOG.md` entry (Claude sends it to him) · he changes any words and deletes the `<!-- draft` line, or tells Claude to; Claude commits it to the release branch. The tag refuses to build while the draft line is there
- [ ] **Go.** Joe · in the session · after D and E are green, Joe says go for `v<x.y.z>`; Claude logs his words and the date in the sign-off

## D. Mac day

- [ ] **Book it on the release day if possible,** so one rental covers stage 6 and the Mac half of G. Claude · plan · rental start time noted (24 h minimum)
- [ ] **Rent the cloud Mac.** Claude, on Joe's go for the spend · Scaleway, Mac M4 (about €5 to €7) · SSH works
- [ ] **App Store sign-in.** Joe · the cloud Mac, first Mac day only · Logic trial installed
- [ ] **Signed installer on a real Mac (T2-19).** Claude · `spctl -a -vv -t install`, `xcrun stapler validate`, install · accepted, notarized, no Gatekeeper warning
- [ ] **auval.** Claude · `auval` on the installed AU · passes
- [ ] **Mac DAWs.** Claude · REAPER on Mac (D-RPR Mac rows), Logic harness (D-LGC) · pass
- [ ] **Mac-only items.** Claude · anything on the master list marked for Mac day (for example the standalone's microphone permission) · checked and passing
- [ ] **Keep it** if the tag is within the rental; otherwise delete it now and book one more Mac day for G. Claude · Scaleway · nothing left running by accident

## E. Real purchase and refund

- [ ] **Card field loads.** Claude · `npm test -- --read-only`, W-31 · passes (the paid run stops otherwise)
- [ ] **Buy (stage 8).** Claude starts it, Joe pays · `npm test -- --paid` opens Chrome and stops at payment; Joe enters his card and places the order · W-49: key arrives, thank-you page shows it, order email (W-60) from support@, not in spam; order number printed
- [ ] **Activate and reopen.** Claude · the RC build with that key; close and reopen the plugin · still activated (T2-21, R-09)
- [ ] **Refund.** Joe · WooCommerce > Orders > the order > Refund · status Refunded
- [ ] **Refund checked.** Claude · `npm test -- --check-refund <order>` · W-62 refund email arrived; W-65 key refused
- [ ] **Sign-off and production (stage 9).** Claude · T2-22 written in the RC table; merge staging into production · production is exactly the RC commit

## F. Tag and publish

- [ ] **Tag.** Claude, only after Joe's go · `git tag -a v<x.y.z> <production commit> -m "<plugin> <x.y.z>"` then `git push origin v<x.y.z>` (annotated, so the changelog gets the tag's date) · the tag run starts in Actions
- [ ] **Watch it.** Claude · `gh run watch` (or `gh run view <id>`) · every job green

What the pipeline does by itself (auto), and where to see it worked:

- [ ] **Preflight.** Tag on production, matches `wPROJECT_VERSION`, newer than every tag; every signing, notarization and WordPress secret set; the store's key source works; `CHANGELOG.md` entry approved · preflight job green. Red: nothing was built and the tag is deleted, so fix and tag again
- [ ] **Build, Mac and Windows.** Build, unit tests and pluginval on that build, sign, package, notarize, staple, then verify (pkg signature, stapled ticket, Gatekeeper, every bundle; Authenticode on the installer and binaries) · both build jobs green; installers and their `.sha256` on the run's artifacts
- [ ] **Publish, both or neither.** Hash check, store check, upload to the media library, download again and compare, repoint the two download redirects, remove older copies of this version only · publish job green
- [ ] **Changelog and version line.** Writes the changelog page block and the home page's "<x.y.z>, released <date>", clears Elementor's cache, checks the live pages · step "Publish the Changelog Page and the Home Page Version Line" green
- [ ] **Software version last.** The product's Software version set to `<x.y.z>` and read back (this is what tells existing users to update) · last publish step green
- [ ] **Web demo.** The browser demo goes live after publish · web-build job green
- [ ] **If it fails.** Before publish: nothing is public; fix, back to stage 4. Publish stopped part-way: the tag is kept and the run says what to check; go straight to G and use the rollback if anything is live and wrong

## G. Post-release check

The release is not done until this passes. Run it the same day: the CI artifacts it compares against are kept 90 days, and the cloud Mac is still rented.

- [ ] **Run it.** Claude · in wla-site-tests: `npm run post-release -- --product <slug> --version <x.y.z> --tag-run <run id> --mac-ssh <user@host>` · exit 0; read `runs/<time>-post-release-<slug>-<x.y.z>/report.html`
- [ ] **Website pass.** PR-01 home version line; PR-02 changelog's newest entry, no draft marker; PR-03 "Need the previous version?" links download `<prev>`; PR-04 download links go to installers named `<x.y.z>`, with and without `?utm_source=`; PR-05 product Software version; PR-06 `version_check`; PR-07 read-only web test, no new failures against the last run before release
- [ ] **Download.** PR-10 · both installers from the public links, as a customer would · saved in the run folder
- [ ] **Checksums.** PR-11, PR-12 · SHA-256 of the downloaded files against the tag's signed CI artifacts · identical
- [ ] **Signatures.** PR-13 Windows Authenticode Valid, signer as configured; PR-14 Mac `spctl` and `stapler validate` · pass
- [ ] **Install.** PR-20 to PR-25 Windows silent install, versions and signatures of the VST3 and standalone, HostTest loads it, standalone starts and quits; PR-30 to PR-34 the same on the cloud Mac, plus `auval` · pass. Left installed
- [ ] **Version.** Claude · open it in a host, Ctrl+Shift+click the version in About · shows `<x.y.z>` and the tag's commit, no "+"
- [ ] **Licence.** `HostTest --live-license` with a test key, Windows and Mac · activates, deactivates, key back to 0 activations
- [ ] **Codex, then report.** Claude · Codex over the report and screenshots, then the script's one-paragraph summary to Joe · Joe has it, with what Codex caught
- [ ] **Delete the cloud Mac.** Claude · Scaleway · server gone, billing stopped

**Rollback, if anything above fails:**

- [ ] **Pull the downloads back.** Claude, or Joe from these notes · WordPress > Tools > Redirection, the rules `/<plugin>-download-mac` and `/<plugin>-download-windows` · each points at the `<prev>` installer in the media library ("`<plugin>` Installer v`<prev>` for Mac" / "for Windows"); `curl -I` on both, with and without a query string, gives 301 to `<prev>`
- [ ] **Stop the update prompt.** Claude · WooCommerce product > Serial Numbers > Software version · back to `<prev>`, so existing users aren't told to update to the bad build
- [ ] **Site text.** Claude · Elementor: home > Specs > Version (`wla-version`), changelog page entries widget (`wla-changelog`) · read `<prev>` again, or say on the changelog that `<x.y.z>` is pulled
- [ ] **Tell Joe** what failed, what was pulled, and the fix plan. Fix on the work branch, back to stage 4, release as the next patch version; never move a published tag

## H. After release

- [ ] **Announce: parked.** Joe decides when · the release's announcement tasks (forums, a note to beta testers and demo downloaders, ads) · nothing goes out without his say; no em-dashes in anything public
- [ ] **Clean-up.** Claude · GitHub folder and the shop · side branches merged, extra worktree folders removed, scratch folders deleted; test orders set aside and their keys deactivated (from the ledger `C:\Users\Joe\.held\wla-test\orders.jsonl`)
- [ ] **Template.** Claude · this repo · any pipeline or test change made during the release is ported here, driven by `config.cmake`; pushed, not merged
- [ ] **Board updated.** Claude · Blocks, the release project's tasks (through the Blocks server, with `baseMtime`, read back) · every release task Done; anything left over is a task for the next version
- [ ] **Sign-off table filled.** Claude · test plan, "Release candidate N" table · build id, tag, date, who passed each stage, the post-release result; then the Blocks copy of the test plan synced
- [ ] **Session note.** Claude · Blocks, `AI/Notes` · one Session note: what shipped, what is still open
