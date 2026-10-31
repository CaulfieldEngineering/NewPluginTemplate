"""[Codex 2026-10-31] Offline failure cases for the pre-tag storage gate."""

from contextlib import redirect_stderr, redirect_stdout
from datetime import datetime, timedelta, timezone
import importlib.util
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("gate", Path(__file__).with_name("check-release-storage.py"))
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)


class StorageGateTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.directory = Path(self.temp.name)
        (self.directory / "review.md").write_text("Synthetic fixture, never real billing evidence.")
        self.now = datetime.now(timezone.utc)
        self.evidence = {
            "account_owner": "example", "checked_at": self.now.isoformat(),
            "available_bytes": 100_000_000, "uploads_blocked": False,
            "recalculation_pending": False, "source_url": "https://github.com/settings/billing/usage",
            "reviewed_by": "Fixture reviewer", "record": "review.md",
        }
        self.run = {"head_branch": "staging", "conclusion": "success",
                    "path": ".github/workflows/workflow.yml"}
        self.pages = [{"artifacts": [
            {"name": "macos-latest-installer", "expired": False, "size_in_bytes": 1000},
            {"name": "windows-latest-installer", "expired": False, "size_in_bytes": 2000},
            {"name": "test-results-windows-latest", "expired": False, "size_in_bytes": 3000},
        ]}]

    def check(self):
        return gate.check_evidence(self.evidence, "example", 10_000, self.directory, self.now)

    def test_current_evidence_with_room(self):
        self.assertEqual(self.check(), 100_000_000)

    def test_unknown_blocked_stale_or_incomplete_evidence_fails(self):
        changes = [
            ("account_owner", "someone-else"), ("available_bytes", 9999),
            ("available_bytes", None), ("available_bytes", True), ("uploads_blocked", True),
            ("uploads_blocked", None), ("recalculation_pending", True),
            ("checked_at", (self.now - timedelta(minutes=31)).isoformat()),
            ("checked_at", (self.now + timedelta(seconds=1)).isoformat()),
            ("checked_at", self.now.replace(tzinfo=None).isoformat()),
            ("source_url", "https://example.com/settings/billing"),
            ("reviewed_by", ""), ("record", "missing.md"),
        ]
        for key, value in changes:
            with self.subTest(key=key, value=value):
                with patch.dict(self.evidence, {key: value}):
                    with self.assertRaises(ValueError):
                        self.check()

    def test_estimate_includes_diagnostics_and_margin(self):
        self.assertEqual(gate.required_space(self.run, self.pages), (6000, 12_000 + 64 * 1024 * 1024))

    def test_bad_staging_run_fails(self):
        for key, value in (("conclusion", "failure"), ("head_branch", "work"), ("path", "test.yml")):
            with self.subTest(key=key), patch.dict(self.run, {key: value}):
                with self.assertRaises(ValueError):
                    gate.required_space(self.run, self.pages)

    def test_missing_expired_or_empty_installers_fail(self):
        artifact = self.pages[0]["artifacts"][0]
        for key, value in (("name", "other"), ("expired", True), ("size_in_bytes", 0)):
            with self.subTest(key=key), patch.dict(artifact, {key: value}):
                with self.assertRaises(ValueError):
                    gate.required_space(self.run, self.pages)

    def test_cli_requires_evidence_and_then_accepts_valid_review(self):
        evidence_file = self.directory / "evidence.json"
        evidence_file.write_text(json.dumps(self.evidence))
        argv = ["check-release-storage.py", "--repo", "example/plugin", "--staging-run", "42"]
        for extra, expected in (([], 1), (["--evidence", str(evidence_file)], 0)):
            with self.subTest(extra=extra), patch("sys.argv", argv + extra), \
                    patch.object(gate, "gh_json", side_effect=[self.run, self.pages]), \
                    redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
                self.assertEqual(gate.main(), expected)


if __name__ == "__main__":
    unittest.main()
