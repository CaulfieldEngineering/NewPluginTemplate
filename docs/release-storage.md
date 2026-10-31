# Check storage before pushing a release tag

[Codex 2026-10-31] Run this after the staging and installer checks pass, immediately
before any `git push origin v*`. A failed or unverified storage check stops the tag.
Joe must still approve the release itself.

1. Find the successful `workflow.yml` staging run for this release. Run:

   ```text
   python scripts/check-release-storage.py --repo OWNER/REPO --staging-run RUN_ID
   ```

   This read-only command prints the total staging artifact size and the required
   headroom: twice that size plus 64 MiB. Without evidence it exits 1 deliberately.
   The run must contain both unexpired installer artifacts. The estimate includes
   every artifact in the run, not just installers.

2. Review the **repository owner's account-level** GitHub Billing and licensing
   storage/usage and budgets pages. Verify artifact uploads are allowed and there
   is at least the printed headroom under the current storage allowance and any
   spending limit. Include Actions artifacts and shared Packages storage across
   all repositories. Allow for uploads since the last usage update and concurrent
   work. Do not convert GB-hours or billed monthly usage into current free bytes.
   If the UI does not establish the amount, it remains **unverified**; stop here.
   Do not change spending limits or delete artifacts as part of this check.

3. Keep a local screenshot or review note that records the account, current usage,
   limit, applicable budget/block status, pending uploads, and headroom calculation.
   Keep it outside git. Beside it, write `storage-review.json` using actual values:

   ```json
   {
     "account_owner": "OWNER",
     "checked_at": "YYYY-MM-DDTHH:MM:SS+00:00",
     "available_bytes": 0,
     "uploads_blocked": false,
     "recalculation_pending": false,
     "source_url": "https://github.com/settings/billing/usage",
     "reviewed_by": "Person or agent who inspected the evidence",
     "record": "storage-review.md"
   }
   ```

   Replace the URL with the actual owner's billing page. These are placeholders;
   never fill them from assumptions. `available_bytes` is conservatively verified
   headroom, not the account's total allowance. The script validates the record's
   presence and metadata; the reviewer is responsible for its accuracy.

4. Run the check again with `--evidence /path/to/storage-review.json`. Evidence must
   be less than 30 minutes old. Proceed to the separately approved tag push only
   if it exits 0. Repeat the review if other uploads or account limits change.

GitHub's [billing API](https://docs.github.com/en/rest/billing/usage) reports usage;
it does not provide a definitive remaining-artifact-upload-quota field. The
repository artifacts API is used only to estimate this release's needs, never
to prove account headroom. GitHub says [storage usage updates can take 6 to 12
hours](https://docs.github.com/en/billing/concepts/product-billing/github-actions).
After a quota error or cleanup, wait for the account to reflect the change before
recording `recalculation_pending: false`. A month/minutes reset does not itself
prove storage is available.

Diagnostic uploads in `test.yml` are best effort and warn on failure. Its universal
Mac build transfer and cross-platform comparisons still fail if their inputs are
missing. Release/tag installer transfers in `workflow.yml` remain hard failures;
this local check does not weaken or replace them.
