#!/usr/bin/env python3
"""Generate the self-contained HTML Salamatrix Automation API reference."""

from __future__ import annotations

import argparse
import html
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "doc" / "salamatrix-automation-api.md"
TARGET = ROOT / "doc" / "salamatrix-automation-api.html"


def inline(text: str) -> str:
    value = html.escape(text, quote=False)
    value = re.sub(r"`([^`]+)`", r"<code>\1</code>", value)
    value = re.sub(r"\*\*([^*]+)\*\*", r"<strong>\1</strong>", value)
    return re.sub(r"\[([^\]]+)\]\(([^)]+)\)", r'<a href="\2">\1</a>', value)


def render(markdown: str) -> str:
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
            title = heading.group(2)
            anchor = re.sub(r"[^a-z0-9]+", "-", title.lower()).strip("-")
            output.append(f'<h{level} id="{anchor}">{inline(title)}</h{level}>')
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
<title>Salamatrix Automation API reference</title>
<style>
:root {{ color-scheme:light dark; --bg:#fff; --fg:#202124; --surface:#f6f8fa;
 --border:#d0d7de; --accent:#c9252d; --code:#f0f2f4; }}
@media (prefers-color-scheme:dark) {{ :root {{ --bg:#1e1f20; --fg:#e8eaed;
 --surface:#27292c; --border:#45484d; --code:#2d3034; }} }}
* {{ box-sizing:border-box; }}
body {{ margin:0; background:var(--bg); color:var(--fg);
 font:15px/1.55 "Segoe UI",Arial,sans-serif; }}
main {{ max-width:1120px; margin:0 auto; padding:32px 28px 64px; }}
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
{body}
</main></body>
</html>
"""


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    generated = render(SOURCE.read_text(encoding="utf-8"))
    if args.check:
        if not TARGET.exists() or TARGET.read_text(encoding="utf-8") != generated:
            print(f"{TARGET} is stale; run {Path(__file__).name}", file=sys.stderr)
            return 1
        return 0
    TARGET.write_text(generated, encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
