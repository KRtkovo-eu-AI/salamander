# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools" / "codeql"))

import filter_sarif_to_pr_diff as filter_sarif  # noqa: E402


def _result(uri: str, start_line: int) -> dict:
    return {
        "ruleId": "cpp/unbounded-write",
        "locations": [
            {
                "physicalLocation": {
                    "artifactLocation": {"uri": uri},
                    "region": {"startLine": start_line},
                }
            }
        ],
    }


def test_added_lines_from_github_patch() -> None:
    patch = """@@ -10,3 +10,5 @@ void example()
     int a = 1;
+    int b = 2;
     int c = 3;
+    int d = 4;
"""
    assert filter_sarif.added_lines_from_patch(patch) == {11, 13}


def test_filter_keeps_only_alerts_on_added_lines(tmp_path: Path) -> None:
    sarif = {
        "version": "2.1.0",
        "runs": [
            {
                "results": [
                    _result("src/dialogs5.cpp", 11),
                    _result("src/dialogs5.cpp", 12),
                    _result("file:///D:/a/salamander/salamander/src/fileswn3.cpp", 80),
                ]
            }
        ],
    }
    files = [
        {
            "filename": "src/dialogs5.cpp",
            "patch": """@@ -10,3 +10,4 @@
     int a = 1;
+    int b = 2;
     int c = 3;
""",
        }
    ]
    filtered, kept, dropped = filter_sarif.filter_sarif(
        sarif, filter_sarif.changed_lines_by_file(files)
    )
    assert kept == 1
    assert dropped == 2
    assert filtered["runs"][0]["results"][0]["locations"][0]["physicalLocation"][
        "region"
    ]["startLine"] == 11


def test_cli_filters_sarif_from_changed_files_json(tmp_path: Path) -> None:
    sarif_path = tmp_path / "input.sarif"
    output_path = tmp_path / "output.sarif"
    files_path = tmp_path / "files.json"
    sarif_path.write_text(
        json.dumps(
            {
                "runs": [
                    {
                        "results": [
                            _result("src/salspawn/salspawn.cpp", 82),
                            _result("src/callstk.cpp", 588),
                        ]
                    }
                ]
            }
        ),
        encoding="utf-8",
    )
    files_path.write_text(
        json.dumps(
            [
                {
                    "filename": "src/salspawn/salspawn.cpp",
                    "patch": """@@ -81,1 +81,2 @@
     int retBase = 10000;
+    const int exeNameChars = 32768;
""",
                }
            ]
        ),
        encoding="utf-8",
    )
    assert (
        filter_sarif.main(
            [
                "--sarif",
                str(sarif_path),
                "--output",
                str(output_path),
                "--changed-files-json",
                str(files_path),
            ]
        )
        == 0
    )
    output = json.loads(output_path.read_text(encoding="utf-8"))
    assert len(output["runs"][0]["results"]) == 1
    assert output["runs"][0]["results"][0]["locations"][0]["physicalLocation"][
        "region"
    ]["startLine"] == 82
