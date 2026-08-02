#!/usr/bin/env python3
"""Compare source-code metrics with the latest Samandarin release tag."""

from __future__ import annotations

import argparse
import io
import json
import re
import subprocess
import tarfile
from collections import defaultdict
from pathlib import Path
from typing import Any, Iterable


SOURCE_EXTENSIONS = {
    ".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".inl",
    ".cs", ".java", ".js", ".mjs", ".ts", ".py", ".php", ".lua", ".rb",
    ".ps1", ".rc", ".rc2", ".idl",
}
EXCLUDED_PARTS = {".git", "build", "node_modules", "packages", "__pycache__"}


def git(repository: Path, *args: str, text: bool = True) -> str | bytes:
    result = subprocess.run(
        ["git", "-C", str(repository), *args],
        check=True,
        capture_output=True,
        text=text,
        encoding="utf-8" if text else None,
        errors="replace" if text else None,
    )
    return result.stdout


def resolve_baseline(repository: Path, prefix: str) -> dict[str, str]:
    best: tuple[tuple[int, ...], str] | None = None
    pattern = re.compile(rf"^{re.escape(prefix)}(?P<version>\d+(?:\.\d+)*)$")
    for tag in str(git(repository, "tag", "--list", f"{prefix}*")).splitlines():
        match = pattern.fullmatch(tag.strip())
        if not match:
            continue
        parts = tuple(int(part) for part in match.group("version").split("."))
        candidate = (parts, tag)
        if best is None or candidate > best:
            best = candidate
    if best is None:
        raise RuntimeError(f"No numeric release tag matching {prefix}* was found")
    commit = str(git(repository, "rev-list", "-n", "1", best[1])).strip().lower()
    return {"tag": best[1], "version": ".".join(map(str, best[0])), "commit": commit}


def is_source(path: str) -> bool:
    candidate = Path(path)
    return candidate.suffix.lower() in SOURCE_EXTENSIONS and not any(
        part.lower() in EXCLUDED_PARTS for part in candidate.parts
    )


def decode_source(data: bytes) -> str:
    for encoding in ("utf-8-sig", "utf-16", "cp1252"):
        try:
            return data.decode(encoding)
        except UnicodeDecodeError:
            pass
    return data.decode("utf-8", errors="replace")


def baseline_sources(repository: Path, commit: str) -> dict[str, str]:
    result: dict[str, str] = {}
    archive = bytes(git(repository, "archive", "--format=tar", commit, text=False))
    with tarfile.open(fileobj=io.BytesIO(archive), mode="r:") as tar:
        for member in tar:
            normalized = member.name.replace("\\", "/")
            if not member.isfile() or not is_source(normalized):
                continue
            extracted = tar.extractfile(member)
            if extracted is not None:
                result[normalized] = decode_source(extracted.read())
    return result


def current_sources(repository: Path) -> dict[str, str]:
    output = str(git(repository, "ls-files", "--cached", "--others", "--exclude-standard", "-z"))
    result: dict[str, str] = {}
    for path in output.split("\0"):
        normalized = path.replace("\\", "/")
        if not normalized or not is_source(normalized):
            continue
        absolute = repository / Path(normalized)
        if absolute.is_file():
            result[normalized] = decode_source(absolute.read_bytes())
    return result


def source_diff_summary(repository: Path, baseline_commit: str) -> dict[str, int]:
    added = 0
    deleted = 0
    changed_files = 0
    tracked_paths: set[str] = set()
    output = str(git(repository, "diff", "--numstat", baseline_commit, "--"))
    for line in output.splitlines():
        parts = line.split("\t", 2)
        if len(parts) != 3 or not is_source(parts[2]) or not parts[0].isdigit() or not parts[1].isdigit():
            continue
        added += int(parts[0])
        deleted += int(parts[1])
        changed_files += 1
        tracked_paths.add(parts[2].replace("\\", "/"))

    untracked = str(git(repository, "ls-files", "--others", "--exclude-standard", "-z"))
    for path in untracked.split("\0"):
        normalized = path.replace("\\", "/")
        if not normalized or normalized in tracked_paths or not is_source(normalized):
            continue
        absolute = repository / Path(normalized)
        if absolute.is_file():
            added += len(decode_source(absolute.read_bytes()).splitlines())
            changed_files += 1
    return {"added": added, "deleted": deleted, "net": added - deleted, "changedFiles": changed_files}


def fallback_nloc(source: str) -> int:
    in_block_comment = False
    count = 0
    for raw_line in source.splitlines():
        line = raw_line.strip()
        if in_block_comment:
            if "*/" in line:
                line = line.split("*/", 1)[1].strip()
                in_block_comment = False
            else:
                continue
        while line.startswith("/*"):
            if "*/" in line[2:]:
                line = line.split("*/", 1)[1].strip()
            else:
                in_block_comment = True
                line = ""
                break
        if line and not line.startswith("//"):
            count += 1
    return count


def load_lizard() -> Any | None:
    try:
        import lizard  # type: ignore
    except ImportError:
        return None
    return lizard


def analyze_snapshot(sources: dict[str, str], label: str, lizard_module: Any | None) -> dict[str, Any]:
    files: list[dict[str, Any]] = []
    functions: list[dict[str, Any]] = []
    for path in sorted(sources):
        source = sources[path]
        nloc = fallback_nloc(source)
        file_functions: list[Any] = []
        if lizard_module is not None:
            try:
                info = lizard_module.analyze_file.analyze_source_code(path, source)
                nloc = int(info.nloc)
                file_functions = list(info.function_list)
            except Exception:
                file_functions = []
        files.append({"path": path, "nloc": nloc})
        for function in file_functions:
            functions.append(
                {
                    "path": path,
                    "name": str(function.name),
                    "longName": str(function.long_name),
                    "startLine": int(function.start_line),
                    "endLine": int(function.end_line),
                    "nloc": int(function.nloc),
                    "cyclomaticComplexity": int(function.cyclomatic_complexity),
                    "tokenCount": int(function.token_count),
                }
            )
    return {
        "label": label,
        "fileCount": len(files),
        "nloc": sum(item["nloc"] for item in files),
        "functionCount": len(functions),
        "cyclomaticComplexity": sum(item["cyclomaticComplexity"] for item in functions),
        "files": files,
        "functions": functions,
    }


def aggregate(items: Iterable[dict[str, Any]], key_name: str) -> dict[str, dict[str, int]]:
    totals: dict[str, dict[str, int]] = defaultdict(lambda: {"nloc": 0, "count": 0, "complexity": 0})
    for item in items:
        key = str(item[key_name])
        totals[key]["nloc"] += int(item["nloc"])
        totals[key]["count"] += 1
        totals[key]["complexity"] += int(item.get("cyclomaticComplexity", 0))
    return dict(totals)


def module_for(path: str) -> str:
    parts = path.split("/")
    if len(parts) >= 3 and parts[:2] == ["src", "plugins"]:
        return "/".join(parts[:3])
    if len(parts) == 2 and parts[0] == "src":
        return "src/core"
    return "/".join(parts[:2]) if len(parts) >= 2 else parts[0]


def scope_for(function: dict[str, Any]) -> str:
    name = str(function["name"])
    return name.rsplit("::", 1)[0] if "::" in name else "<global functions>"


def comparison_rows(
    before: dict[str, dict[str, int]], after: dict[str, dict[str, int]], key_label: str
) -> list[dict[str, Any]]:
    rows = []
    for key in sorted(set(before) | set(after)):
        old = before.get(key, {"nloc": 0, "count": 0, "complexity": 0})
        new = after.get(key, {"nloc": 0, "count": 0, "complexity": 0})
        rows.append(
            {
                key_label: key,
                "baselineNloc": old["nloc"],
                "currentNloc": new["nloc"],
                "deltaNloc": new["nloc"] - old["nloc"],
                "baselineCount": old["count"],
                "currentCount": new["count"],
                "deltaCount": new["count"] - old["count"],
                "deltaComplexity": new["complexity"] - old["complexity"],
            }
        )
    return sorted(rows, key=lambda row: (abs(row["deltaNloc"]), row[key_label]), reverse=True)


def compare(baseline: dict[str, Any], current: dict[str, Any]) -> dict[str, Any]:
    before_files = aggregate(baseline["files"], "path")
    after_files = aggregate(current["files"], "path")

    before_modules = aggregate(
        ({**item, "module": module_for(item["path"])} for item in baseline["files"]), "module"
    )
    after_modules = aggregate(
        ({**item, "module": module_for(item["path"])} for item in current["files"]), "module"
    )

    before_scopes = aggregate(
        ({**item, "scope": scope_for(item)} for item in baseline["functions"]), "scope"
    )
    after_scopes = aggregate(
        ({**item, "scope": scope_for(item)} for item in current["functions"]), "scope"
    )

    def function_key(item: dict[str, Any]) -> str:
        return f"{item['path']}::{item['name']}"

    before_functions = aggregate(
        ({**item, "identity": function_key(item)} for item in baseline["functions"]), "identity"
    )
    after_functions = aggregate(
        ({**item, "identity": function_key(item)} for item in current["functions"]), "identity"
    )

    return {
        "summary": {
            "baselineNloc": baseline["nloc"],
            "currentNloc": current["nloc"],
            "deltaNloc": current["nloc"] - baseline["nloc"],
            "baselineFiles": baseline["fileCount"],
            "currentFiles": current["fileCount"],
            "deltaFiles": current["fileCount"] - baseline["fileCount"],
            "baselineFunctions": baseline["functionCount"],
            "currentFunctions": current["functionCount"],
            "deltaFunctions": current["functionCount"] - baseline["functionCount"],
            "deltaComplexity": current["cyclomaticComplexity"] - baseline["cyclomaticComplexity"],
        },
        "modules": comparison_rows(before_modules, after_modules, "module"),
        "files": comparison_rows(before_files, after_files, "path"),
        "scopes": comparison_rows(before_scopes, after_scopes, "scope"),
        "functions": comparison_rows(before_functions, after_functions, "function"),
    }


def markdown(report: dict[str, Any]) -> str:
    summary = report["comparison"]["summary"]
    lines = [
        "# Source code growth report",
        "",
        f"Baseline: **{report['baseline']['tag']}** (`{report['baseline']['commit']}`)  ",
        f"Current: **{report['current']['label']}**  ",
        f"NLOC: **{summary['baselineNloc']:,} -> {summary['currentNloc']:,}** "
        f"(**{summary['deltaNloc']:+,}**)  ",
        f"Files: **{summary['baselineFiles']:,} -> {summary['currentFiles']:,}** "
        f"(**{summary['deltaFiles']:+,}**)  ",
        f"Functions/methods: **{summary['baselineFunctions']:,} -> {summary['currentFunctions']:,}** "
        f"(**{summary['deltaFunctions']:+,}**)",
    ]
    if not report["lizardAvailable"]:
        lines += ["", "> Function and scope/class details are unavailable because the `lizard` package is not installed."]

    def table(title: str, item_label: str, rows: list[dict[str, Any]], key: str, count_label: str) -> None:
        changed = [row for row in rows if row["deltaNloc"] or row["deltaCount"]][:20]
        lines.extend(["", f"## {title}", ""])
        if not changed:
            lines.append("No changes.")
            return
        lines.extend(
            [
                f"| {item_label} | Baseline NLOC | Current NLOC | Delta | {count_label} delta |",
                "| --- | ---: | ---: | ---: | ---: |",
            ]
        )
        for row in changed:
            lines.append(
                f"| `{row[key]}` | {row['baselineNloc']:,} | {row['currentNloc']:,} | "
                f"{row['deltaNloc']:+,} | {row['deltaCount']:+,} |"
            )

    table("Modules", "Module", report["comparison"]["modules"], "module", "Files")
    table("Files", "File", report["comparison"]["files"], "path", "Files")
    if report["lizardAvailable"]:
        table("Scopes/classes", "Scope/class", report["comparison"]["scopes"], "scope", "Functions")
        table("Functions", "Function", report["comparison"]["functions"], "function", "Occurrences")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repository-root", type=Path, default=Path.cwd())
    parser.add_argument("--baseline-commit")
    parser.add_argument("--tag-prefix", default="5.0-samandarin-")
    parser.add_argument("--current-label", default="working tree")
    parser.add_argument("--output-json", type=Path)
    parser.add_argument("--output-markdown", type=Path)
    parser.add_argument("--summary-only", action="store_true")
    args = parser.parse_args()

    repository = args.repository_root.resolve()
    baseline = resolve_baseline(repository, args.tag_prefix)
    if args.baseline_commit:
        baseline["commit"] = str(git(repository, "rev-parse", args.baseline_commit)).strip().lower()

    if args.summary_only:
        summary = source_diff_summary(repository, baseline["commit"])
        print(
            f"[code-growth] {baseline['tag']} -> {args.current_label}: "
            f"{summary['added']:+,} / -{summary['deleted']:,} source lines "
            f"(net {summary['net']:+,}) across {summary['changedFiles']:,} files"
        )
        return 0

    lizard_module = load_lizard()
    baseline_snapshot = analyze_snapshot(
        baseline_sources(repository, baseline["commit"]), baseline["tag"], lizard_module
    )
    current_snapshot = analyze_snapshot(current_sources(repository), args.current_label, lizard_module)
    report = {
        "schemaVersion": 1,
        "metric": "source-nloc",
        "lizardAvailable": lizard_module is not None,
        "baseline": baseline,
        "current": current_snapshot,
        "baselineSnapshot": baseline_snapshot,
    }
    report["comparison"] = compare(baseline_snapshot, current_snapshot)
    rendered = markdown(report)

    if args.output_json:
        args.output_json.parent.mkdir(parents=True, exist_ok=True)
        args.output_json.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    if args.output_markdown:
        args.output_markdown.parent.mkdir(parents=True, exist_ok=True)
        args.output_markdown.write_text(rendered, encoding="utf-8")

    summary = report["comparison"]["summary"]
    print(
        f"[code-growth] {baseline['tag']} -> {args.current_label}: "
        f"{summary['currentNloc']:,} NLOC ({summary['deltaNloc']:+,}), "
        f"{summary['currentFiles']:,} files ({summary['deltaFiles']:+,})"
    )
    print(rendered, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
