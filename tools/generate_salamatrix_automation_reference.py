#!/usr/bin/env python3
"""Generate the self-contained HTML Salamatrix authoring references."""

from __future__ import annotations

import argparse
import html
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DOCUMENTS = (
    ("salamatrix-automation-api", "Salamatrix Automation API reference"),
    ("salamatrix-ui", "Salamatrix.UI framework and custom dialog guide"),
    ("salamatrix-platform", "Salamatrix Platform Foundation"),
    ("salamatrix-runtime-providers", "Standalone Salamatrix runtime providers"),
    (
        "salamatrix-runtime-provider-development",
        "Developing a Salamatrix language runtime provider",
    ),
    ("salamatrix-gap-analysis", "Salamatrix GAP analysis"),
)

HTML_DOCUMENT_NAMES = {f"{slug}.md": f"{slug}.html" for slug, _ in DOCUMENTS}


def inline(text: str) -> str:
    value = html.escape(text, quote=False)
    value = re.sub(r"`([^`]+)`", r"<code>\1</code>", value)
    value = re.sub(r"\*\*([^*]+)\*\*", r"<strong>\1</strong>", value)

    def link(match: re.Match[str]) -> str:
        target = match.group(2)
        for markdown_name, html_name in HTML_DOCUMENT_NAMES.items():
            if target == markdown_name or target.startswith(markdown_name + "#"):
                target = html_name + target[len(markdown_name) :]
                break
        return f'<a href="{target}">{match.group(1)}</a>'

    return re.sub(r"\[([^\]]+)\]\(([^)]+)\)", link, value)


def render(markdown: str, document_title: str, current_slug: str) -> str:
    lines = markdown.splitlines()
    output: list[str] = []
    paragraph: list[str] = []
    list_open = False
    in_code = False
    code_language = ""
    code_lines: list[str] = []
    index = 0

    def flush_paragraph() -> None:
        if paragraph:
            output.append("<p>" + inline(" ".join(paragraph)) + "</p>")
            paragraph.clear()

    def close_list() -> None:
        nonlocal list_open
        if list_open:
            output.append("</ul>")
            list_open = False

    while index < len(lines):
        line = lines[index]
        if line.startswith("```"):
            flush_paragraph()
            close_list()
            if in_code:
                output.append(
                    f'<pre><code class="language-{html.escape(code_language)}">'
                    + html.escape("\n".join(code_lines))
                    + "</code></pre>"
                )
                code_lines.clear()
                in_code = False
            else:
                in_code = True
                code_language = line[3:].strip()
            index += 1
            continue
        if in_code:
            code_lines.append(line)
            index += 1
            continue
        if line.startswith("|") and index + 1 < len(lines) and re.match(
            r"^\|\s*:?-+", lines[index + 1]
        ):
            flush_paragraph()
            close_list()
            rows: list[list[str]] = []
            while index < len(lines) and lines[index].startswith("|"):
                rows.append([cell.strip() for cell in lines[index].strip("|").split("|")])
                index += 1
            output.append('<div class="table-wrap"><table><thead><tr>')
            output.extend(f"<th>{inline(cell)}</th>" for cell in rows[0])
            output.append("</tr></thead><tbody>")
            for row in rows[2:]:
                output.append("<tr>")
                output.extend(f"<td>{inline(cell)}</td>" for cell in row)
                output.append("</tr>")
            output.append("</tbody></table></div>")
            continue
        heading = re.match(r"^(#{1,4})\s+(.+)$", line)
        if heading:
            flush_paragraph()
            close_list()
            level = len(heading.group(1))
            heading_title = heading.group(2)
            anchor = re.sub(r"[^a-z0-9]+", "-", heading_title.lower()).strip("-")
            output.append(
                f'<h{level} id="{anchor}">{inline(heading_title)}</h{level}>'
            )
            index += 1
            continue
        item = re.match(r"^\s*[-*]\s+(.+)$", line)
        if item:
            flush_paragraph()
            if not list_open:
                output.append("<ul>")
                list_open = True
            output.append("<li>" + inline(item.group(1)) + "</li>")
            index += 1
            continue
        if not line.strip():
            flush_paragraph()
            close_list()
        else:
            paragraph.append(line.strip())
        index += 1

    flush_paragraph()
    close_list()
    body = "\n".join(output)
    return f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{html.escape(document_title)}</title>
<style>
:root {{ color-scheme:light dark; --bg:#fff; --fg:#202124; --surface:#f6f8fa;
 --border:#d0d7de; --accent:#c9252d; --code:#f0f2f4; }}
@media (prefers-color-scheme:dark) {{ :root {{ --bg:#1e1f20; --fg:#e8eaed;
 --surface:#27292c; --border:#45484d; --code:#2d3034; }} }}
* {{ box-sizing:border-box; }}
body {{ margin:0; background:var(--bg); color:var(--fg);
 font:15px/1.55 "Segoe UI",Arial,sans-serif; }}
main {{ max-width:1120px; margin:0 auto; padding:32px 28px 64px; }}
.doc-nav {{ display:flex; flex-wrap:wrap; gap:.5rem; margin-bottom:1.5rem;
 padding:.75rem; background:var(--surface); border:1px solid var(--border);
 border-radius:6px; }}
.doc-nav a {{ padding:.35rem .6rem; border-radius:4px; text-decoration:none; }}
.doc-nav a:hover {{ background:var(--code); }}
h1 {{ font-size:2rem; border-bottom:3px solid var(--accent); padding-bottom:.3em; }}
h2 {{ font-size:1.45rem; margin-top:2em; border-bottom:1px solid var(--border);
 padding-bottom:.25em; }}
h3 {{ font-size:1.15rem; margin-top:1.6em; }}
p,ul {{ max-width:92ch; }} a {{ color:#2f81f7; }}
code {{ font-family:Consolas,"Cascadia Mono",monospace; background:var(--code);
 border-radius:3px; padding:.08em .3em; }}
pre {{ overflow:auto; padding:14px 16px; background:var(--surface);
 border:1px solid var(--border); border-radius:6px; }}
pre code {{ padding:0; background:transparent; }}
.table-wrap {{ overflow:auto; margin:1em 0; }}
table {{ border-collapse:collapse; min-width:620px; width:100%; }}
th,td {{ border:1px solid var(--border); padding:7px 10px; text-align:left;
 vertical-align:top; }}
th {{ background:var(--surface); }} li {{ margin:.2em 0; }}
</style>
</head>
<body><main>
<nav class="doc-nav" aria-label="Salamatrix documentation">
{' '.join(f'<a href="{slug}.html"' + (' aria-current="page"' if slug == current_slug else '') + f'>{html.escape(nav_title)}</a>' for slug, nav_title in DOCUMENTS)}
</nav>
{body}
</main></body>
</html>
"""


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    stale: list[Path] = []
    generated_documents: list[tuple[Path, str]] = []
    for slug, title in DOCUMENTS:
        source = ROOT / "doc" / f"{slug}.md"
        target = ROOT / "doc" / f"{slug}.html"
        generated = render(source.read_text(encoding="utf-8"), title, slug)
        generated_documents.append((target, generated))
        if not target.exists() or target.read_text(encoding="utf-8") != generated:
            stale.append(target)
    if args.check:
        for target in stale:
            print(f"{target} is stale; run {Path(__file__).name}", file=sys.stderr)
        return 1 if stale else 0
    for target, generated in generated_documents:
        target.write_text(generated, encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
