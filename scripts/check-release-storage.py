#!/usr/bin/env python3
"""[Codex 2026-10-31] Read-only storage gate to run before pushing a v* tag.

GitHub's billing usage API does not report authoritative remaining upload space.
The operator must review current account-level billing/storage and supply the
local evidence described in docs/release-storage.md. This script checks that
evidence, its freshness, and room for the measured staging artifacts; it never
pushes, deletes artifacts, or changes billing settings.
"""

import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import re
import subprocess
import sys
from urllib.parse import urlparse


def gh_json(endpoint, paginate=False):
    command = ["gh", "api", endpoint]
    if paginate:
        command += ["--paginate", "--slurp"]
    result = subprocess.run(command, capture_output=True, text=True, timeout=60)
    if result.returncode:
        raise ValueError("GitHub read failed; check gh authentication/access. Storage is unverified.")
    return json.loads(result.stdout)


def required_space(run, pages):
    if (run.get("head_branch") != "staging" or run.get("conclusion") != "success"
            or run.get("path") != ".github/workflows/workflow.yml"):
        raise ValueError("Use a successful staging workflow.yml run for the artifact estimate.")
    artifacts = [item for page in pages for item in page["artifacts"]]
    names = {item["name"] for item in artifacts if not item.get("expired")}
    if not {"macos-latest-installer", "windows-latest-installer"} <= names:
        raise ValueError("The staging run must still contain both verified installer artifacts.")
    if any(item.get("expired") or type(item.get("size_in_bytes")) is not int
           or item["size_in_bytes"] <= 0 for item in artifacts):
        raise ValueError("An expired, missing-size or empty staging artifact prevents an estimate.")
    # Allow for larger release logs/installers and another upload attempt. This
    # estimates one release; it is not an account quota or a reservation.
    footprint = sum(item["size_in_bytes"] for item in artifacts)
    return footprint, footprint * 2 + 64 * 1024 * 1024


def check_evidence(evidence, owner, required, directory, now):
    if str(evidence.get("account_owner", "")).casefold() != owner.casefold():
        raise ValueError("Evidence must cover the repository owner's whole billing account.")
    checked = datetime.fromisoformat(evidence.get("checked_at", "").replace("Z", "+00:00"))
    if checked.tzinfo is None or not 0 <= (now - checked).total_seconds() <= 1800:
        raise ValueError("Storage evidence must have a timezone and be no more than 30 minutes old.")
    if evidence.get("uploads_blocked") is not False:
        raise ValueError("Uploads are blocked or their status is unverified.")
    if evidence.get("recalculation_pending") is not False:
        raise ValueError("Wait for GitHub's storage recalculation before checking again.")
    available = evidence.get("available_bytes")
    if type(available) is not int or available < required:
        raise ValueError(f"Verified account headroom must be at least {required} bytes.")
    source = urlparse(evidence.get("source_url", ""))
    if source.scheme != "https" or source.hostname != "github.com" or "billing" not in source.path:
        raise ValueError("Record the GitHub account billing page used to verify headroom.")
    if not str(evidence.get("reviewed_by", "")).strip():
        raise ValueError("Name the person or agent who actually reviewed the billing evidence.")
    record = directory / evidence.get("record", "")
    if not record.is_file() or record.stat().st_size == 0:
        raise ValueError("Keep a nonempty local review record/screenshot beside the evidence JSON.")
    return available


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", required=True, help="OWNER/REPO")
    parser.add_argument("--staging-run", required=True, type=int)
    parser.add_argument("--evidence", type=Path, help="Local account storage review JSON; never commit it")
    args = parser.parse_args()
    if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", args.repo) or args.staging_run <= 0:
        parser.error("Supply OWNER/REPO and a positive staging run ID.")
    try:
        base = f"repos/{args.repo}/actions/runs/{args.staging_run}"
        run = gh_json(base)
        pages = gh_json(base + "/artifacts?per_page=100", paginate=True)
        footprint, required = required_space(run, pages)
        print(f"Staging artifacts: {footprint} bytes. Required release headroom: {required} bytes.")
        if args.evidence is None:
            raise ValueError("Storage is UNVERIFIED. Review account billing and supply --evidence; do not push a tag.")
        evidence = json.loads(args.evidence.read_text(encoding="utf-8-sig"))
        available = check_evidence(evidence, args.repo.split("/")[0], required,
                                   args.evidence.parent, datetime.now(timezone.utc))
        print(f"PASS: reviewed account headroom {available} bytes >= {required} bytes required.")
        print("This is an operator-verified check, not an API quota guarantee or storage reservation.")
        print("Recheck if other uploads or account limits change. Joe's release approval is still required.")
        return 0
    except (ValueError, KeyError, TypeError, OSError, subprocess.TimeoutExpired) as error:
        print(f"BLOCKED: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
