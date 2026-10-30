# NewPluginTemplate changelog

<!-- How this file works (Claude, 2026-10-30):
     This is the one source of the plugin's public changelog page (wCHANGELOG_PAGE_ID in config.cmake). On a v* release
     tag the pipeline turns every entry up to that version into the page's changelog block (newest first), sets the
     home page's version line, and checks both live.
     Format: "## x.y.z (YYYY-MM-DD)" or "## x.y.z (release date)" (the release tag's date, New York time), then "- "
     bullets; an optional "**Group**" line starts a group.
     A new entry is drafted by scripts/draft-changelog.mjs with a "<!-- draft" line under its heading. Joe approves it by
     deleting that line; the pipeline refuses to release a version whose entry still has it.
     Comments like this one are never published. -->

## 0.1.0 (release date)
<!-- draft: the first entry. Write it with scripts/draft-changelog.mjs at the release candidate; Joe approves it by deleting this line. -->
- First release.
