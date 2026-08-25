# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

"""Keep CodeQL SARIF alerts that sit on lines added or modified in a pull request.

GitHub's compare API stops at 300 files, so large PRs make Code scanning treat
baseline alerts as new. The pull-request files API paginates past that limit.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
import urllib.request

HUNK_HEADER = re.compile(r"^@@ -\d+(?:,\d+)? \+(\d+)(?:,\d+)? @@")


def added_lines_from_patch(patch: str | None) -> set[int]:
    if not patch:
        return set()
    added: set[int] = set()
    new_line = 0
    for raw in patch.splitlines():
        match = HUNK_HEADER.match(raw)
        if match:
            new_line = int(match.group(1))
            continue
        if raw.startswith("+++") or raw.startswith("---"):
            continue
        if raw.startswith("+"):
            added.add(new_line)
            new_line += 1
            continue
        if raw.startswith("-"):
            continue
        if raw.startswith("\\"):
            continue
        new_line += 1
    return added


def changed_lines_by_file(files: list[dict]) -> dict[str, set[int]]:
    mapping: dict[str, set[int]] = {}
    for entry in files:
        filename = (entry.get("filename") or "").replace("\\", "/")
        if not filename:
            continue
        mapping[filename] = added_lines_from_patch(entry.get("patch"))
    return mapping


def normalize_uri(uri: str) -> str:
    path = uri.replace("\\", "/")
    if path.startswith("file://"):
        path = path[7:]
        if path.startswith("/") and len(path) > 2 and path[2] == ":":
            path = path[1:]
    if len(path) > 1 and path[1] == ":":
        path = path[2:]
        if path.startswith("/"):
            path = path[1:]
    return path.lstrip("/")


def result_location(result: dict) -> tuple[str | None, int | None]:
    locations = result.get("locations") or []
    if not locations:
        return None, None
    physical = locations[0].get("physicalLocation") or {}
    uri = (physical.get("artifactLocation") or {}).get("uri") or ""
    start_line = (physical.get("region") or {}).get("startLine")
    if not isinstance(start_line, int):
        return normalize_uri(uri) or None, None
    return normalize_uri(uri) or None, start_line


def uri_matches_changed_file(uri: str, changed_files: dict[str, set[int]]) -> str | None:
    for filename in changed_files:
        if uri == filename or uri.endswith("/" + filename):
            return filename
    return None


def filter_sarif(sarif: dict, changed_files: dict[str, set[int]]) -> tuple[dict, int, int]:
    kept = 0
    dropped = 0
    for run in sarif.get("runs") or []:
        filtered = []
        for result in run.get("results") or []:
            uri, start_line = result_location(result)
            filename = uri_matches_changed_file(uri or "", changed_files) if uri else None
            if filename is None or start_line is None:
                dropped += 1
                continue
            if start_line in changed_files[filename]:
                filtered.append(result)
                kept += 1
            else:
                dropped += 1
        run["results"] = filtered
    return sarif, kept, dropped


def github_pull_files(repo: str, pr_number: int, token: str) -> list[dict]:
    files: list[dict] = []
    url = (
        f"https://api.github.com/repos/{repo}/pulls/{pr_number}/files?per_page=100"
    )
    headers = {
        "Accept": "application/vnd.github+json",
        "Authorization": f"Bearer {token}",
        "User-Agent": "salamander-codeql-filter",
        "X-GitHub-Api-Version": "2022-11-28",
    }
    while url:
        request = urllib.request.Request(url, headers=headers)
        with urllib.request.urlopen(request) as response:
            payload = json.loads(response.read().decode("utf-8"))
            if not isinstance(payload, list):
                raise RuntimeError(f"Unexpected GitHub files response for {url}")
            files.extend(payload)
            link = response.headers.get("Link") or ""
        next_url = None
        for part in link.split(","):
            if 'rel="next"' in part:
                next_url = part.split(";")[0].strip().strip("<>")
                break
        url = next_url
    return files


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--sarif", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--changed-files-json")
    parser.add_argument("--repo")
    parser.add_argument("--pr", type=int)
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    if args.changed_files_json:
        with open(args.changed_files_json, encoding="utf-8") as handle:
            files = json.load(handle)
    else:
        token = os.environ.get("GITHUB_TOKEN") or os.environ.get("GH_TOKEN")
        if not args.repo or not args.pr or not token:
            print(
                "Provide --changed-files-json, or --repo, --pr, and GITHUB_TOKEN.",
                file=sys.stderr,
            )
            return 2
        files = github_pull_files(args.repo, args.pr, token)

    changed_files = changed_lines_by_file(files)
    with open(args.sarif, encoding="utf-8") as handle:
        sarif = json.load(handle)
    filtered, kept, dropped = filter_sarif(sarif, changed_files)
    with open(args.output, "w", encoding="utf-8", newline="\n") as handle:
        json.dump(filtered, handle)
        handle.write("\n")
    print(
        f"Kept {kept} CodeQL results on PR-changed lines; dropped {dropped} "
        f"results outside the pull request diff ({len(changed_files)} files)."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
