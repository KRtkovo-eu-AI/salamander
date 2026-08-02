# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

import json
import shutil
import struct
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
REPORT = ROOT / "tools" / "binary_size" / "report_binary_sizes.ps1"
RESOLVER = ROOT / "tools" / "binary_size" / "resolve_release_baseline.ps1"
WORKFLOW = ROOT / ".github" / "workflows" / "binary-size.yml"
DIRECTORY_TARGETS = ROOT / "Directory.Build.targets"
PWSH = shutil.which("pwsh") or shutil.which("powershell")


def write_pe(path: Path, text_size: int, resource_size: int) -> None:
    pe_offset = 0x80
    optional_size = 0xF0
    section_table = pe_offset + 24 + optional_size
    data = bytearray(section_table + 80)
    struct.pack_into("<H", data, 0, 0x5A4D)
    struct.pack_into("<I", data, 0x3C, pe_offset)
    struct.pack_into("<IHH", data, pe_offset, 0x00004550, 0x8664, 2)
    struct.pack_into("<H", data, pe_offset + 20, optional_size)
    struct.pack_into("<H", data, pe_offset + 24, 0x20B)
    struct.pack_into("<I", data, pe_offset + 24 + 56, 0x5000)
    for index, (name, raw_size, virtual_size) in enumerate(
        ((b".text", text_size, text_size - 1), (b".rsrc", resource_size, resource_size - 1))
    ):
        offset = section_table + index * 40
        data[offset : offset + len(name)] = name
        struct.pack_into("<III", data, offset + 8, virtual_size, 0x1000 * (index + 1), raw_size)
    path.write_bytes(data)


def run_ps1(script: Path, *args: str) -> subprocess.CompletedProcess[str]:
    assert PWSH, "PowerShell is required"
    return subprocess.run(
        [PWSH, "-NoProfile", "-NonInteractive", "-File", str(script), *args],
        check=True,
        text=True,
        capture_output=True,
    )


def test_report_compares_files_and_pe_sections(tmp_path: Path) -> None:
    baseline = tmp_path / "baseline"
    current = tmp_path / "current"
    baseline.mkdir()
    current.mkdir()
    write_pe(baseline / "salamand.exe", 0x200, 0x100)
    write_pe(current / "salamand.exe", 0x400, 0x180)
    write_pe(current / "new-plugin.spl", 0x100, 0x80)
    output_json = tmp_path / "report.json"
    output_md = tmp_path / "report.md"

    run_ps1(
        REPORT,
        "-BaselineRoot", str(baseline),
        "-CurrentRoot", str(current),
        "-BaselineTag", "5.0-samandarin-0.14",
        "-BaselineCommit", "418595a821e82bf05d71acffeb95231d1689a9c3",
        "-OutputJson", str(output_json),
        "-OutputMarkdown", str(output_md),
    )

    report = json.loads(output_json.read_text(encoding="utf-8"))
    assert report["baselineTag"] == "5.0-samandarin-0.14"
    assert report["summary"]["changedArtifactCount"] == 2
    rows = {row["path"]: row for row in report["comparison"]}
    assert rows["salamand.exe"]["status"] == "changed"
    assert rows["new-plugin.spl"]["status"] == "added"
    sections = {section["name"]: section for section in rows["salamand.exe"]["sectionDeltas"]}
    assert sections[".text"]["deltaRawSizeBytes"] == 0x200
    assert sections[".rsrc"]["deltaRawSizeBytes"] == 0x80
    assert "PE section changes" in output_md.read_text(encoding="utf-8")


def test_resolver_uses_numeric_tag_order_and_peels_annotated_tag(tmp_path: Path) -> None:
    repository = tmp_path / "repo"
    repository.mkdir()
    subprocess.run(["git", "init", "-q", str(repository)], check=True)
    subprocess.run(["git", "-C", str(repository), "config", "user.name", "Binary Size Test"], check=True)
    subprocess.run(["git", "-C", str(repository), "config", "user.email", "test@example.invalid"], check=True)
    (repository / "file.txt").write_text("baseline", encoding="utf-8")
    subprocess.run(["git", "-C", str(repository), "add", "file.txt"], check=True)
    subprocess.run(["git", "-C", str(repository), "commit", "-q", "-m", "baseline"], check=True)
    subprocess.run(["git", "-C", str(repository), "tag", "5.0-samandarin-0.9"], check=True)
    subprocess.run(["git", "-C", str(repository), "tag", "5.0-samandarin-0.10"], check=True)
    subprocess.run(["git", "-C", str(repository), "tag", "-a", "5.0-samandarin-0.14", "-m", "release"], check=True)
    subprocess.run(["git", "-C", str(repository), "tag", "5.0-samandarin-preview"], check=True)

    result = run_ps1(RESOLVER, "-RepositoryPath", str(repository))
    resolved = json.loads(result.stdout)
    expected_commit = subprocess.check_output(
        ["git", "-C", str(repository), "rev-parse", "HEAD"], text=True
    ).strip()
    assert resolved == {
        "tag": "5.0-samandarin-0.14",
        "version": "0.14",
        "commit": expected_commit,
    }


def test_msbuild_and_workflow_keep_release_clean_out_of_reporting() -> None:
    targets = DIRECTORY_TARGETS.read_text(encoding="utf-8")
    workflow = WORKFLOW.read_text(encoding="utf-8")
    assert "'$(Configuration)|$(Platform)' == 'Release|x64'" in targets
    assert "[binary-size]" in targets
    assert "Release clean|x64" not in targets.split('OpenSalamanderReportBinarySize', 1)[1]
    assert "resolve_release_baseline.ps1" in workflow
    assert "/p:Configuration=Release" in workflow
    assert "/p:Platform=x64" in workflow
    assert "job.check_run_id" in workflow
