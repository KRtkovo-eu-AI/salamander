# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

import json
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
REPORT = ROOT / "tools" / "code_metrics" / "report_code_growth.py"
WORKFLOW = ROOT / ".github" / "workflows" / "code-growth.yml"
PR_BUILD = ROOT / ".github" / "workflows" / "pr-msbuild.yml"
DIRECTORY_TARGETS = ROOT / "Directory.Build.targets"


def git(repository: Path, *args: str) -> str:
    return subprocess.check_output(["git", "-C", str(repository), *args], text=True).strip()


def create_repository(path: Path) -> None:
    subprocess.run(["git", "init", "-q", str(path)], check=True)
    subprocess.run(["git", "-C", str(path), "config", "user.name", "Code Growth Test"], check=True)
    subprocess.run(["git", "-C", str(path), "config", "user.email", "test@example.invalid"], check=True)


def test_report_compares_source_without_building_release(tmp_path: Path) -> None:
    repository = tmp_path / "repo"
    repository.mkdir()
    create_repository(repository)
    source = repository / "src" / "main.cpp"
    source.parent.mkdir()
    source.write_text("int CExample::Small() { return 1; }\n", encoding="utf-8")
    subprocess.run(["git", "-C", str(repository), "add", "."], check=True)
    subprocess.run(["git", "-C", str(repository), "commit", "-q", "-m", "baseline"], check=True)
    baseline_commit = git(repository, "rev-parse", "HEAD")
    subprocess.run(["git", "-C", str(repository), "tag", "5.0-samandarin-0.9"], check=True)
    subprocess.run(["git", "-C", str(repository), "tag", "-a", "5.0-samandarin-0.14", "-m", "release"], check=True)

    source.write_text(
        "int CExample::Small() { return 1; }\n"
        "int CExample::Larger(int value) {\n"
        "    if (value > 0) {\n"
        "        return value;\n"
        "    }\n"
        "    return 0;\n"
        "}\n",
        encoding="utf-8",
    )
    output_json = tmp_path / "report.json"
    output_markdown = tmp_path / "report.md"
    subprocess.run(
        [
            sys.executable,
            str(REPORT),
            "--repository-root", str(repository),
            "--current-label", "test-head",
            "--output-json", str(output_json),
            "--output-markdown", str(output_markdown),
        ],
        check=True,
    )

    report = json.loads(output_json.read_text(encoding="utf-8"))
    assert report["metric"] == "source-nloc"
    assert report["baseline"] == {
        "tag": "5.0-samandarin-0.14",
        "version": "0.14",
        "commit": baseline_commit,
    }
    assert report["comparison"]["summary"]["deltaNloc"] > 0
    assert report["comparison"]["files"][0]["path"] == "src/main.cpp"
    assert "Source code growth report" in output_markdown.read_text(encoding="utf-8")


def test_workflow_is_source_only_and_pr_build_calls_codeql_after_success() -> None:
    workflow = WORKFLOW.read_text(encoding="utf-8")
    pr_build = PR_BUILD.read_text(encoding="utf-8")
    targets = DIRECTORY_TARGETS.read_text(encoding="utf-8")
    assert "report_code_growth.py" in workflow
    assert "lizard==1.23.0" in workflow
    assert "msbuild" not in workflow.lower()
    assert "binary-size" not in workflow
    assert "configuration: Release" not in pr_build
    assert "uses: ./.github/workflows/binary-size.yml" not in pr_build
    assert "uses: ./.github/workflows/codeql.yml" in pr_build
    assert "needs.build.result == 'success'" in pr_build
    assert "OpenSalamanderReportCodeGrowth" in targets
    assert "report_code_growth.py" in targets
    assert "'$(Configuration)|$(Platform)' == 'Release|x64'" in targets
    assert "OpenSalamanderReportBinarySize" not in targets
