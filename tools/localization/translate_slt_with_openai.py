#!/usr/bin/env python3
"""Translate untranslated SLT resource lines with the OpenAI Responses API."""
from __future__ import annotations
import argparse, json, os, re, socket, sys, time, urllib.error, urllib.request
from dataclasses import dataclass
from pathlib import Path

LINE_RE = re.compile(r'^(?P<prefix>.*?,)(?P<state>[01]),"(?P<text>.*)"(?P<ending>\r?\n)?$')
SECTION_RE = re.compile(r'^\[(?P<kind>DIALOG|MENU|STRINGTABLE)(?:\s+[^]]+)?\]$')
TOKEN_RE = re.compile(r'%(?:\d+\$)?[-+#0 ]*(?:\d+|\*)?(?:\.\d+|\.\*)?[a-zA-Z]|\{\d+(?::[^}]*)?\}|\\[nrt]|</?[A-Za-z][A-Za-z0-9:_-]*(?:\s+[A-Za-z_:][\w:.-]*(?:=(?:\"[^\"]*\"|\'[^\']*\'|[^\s\"\'>]+))?)*\s*/?>|(?:[A-Za-z]:)?(?:\\[^\\\s]+)+')
ACCELERATOR_RE = re.compile(r'(?<!&)&(?!&)')
MOJIBAKE_RE = re.compile(r'[ÃÂÅ][\u0080-\u00BF\u00A0-\u00BF\u0100-\u017F\u2122]')
REPLACEMENT_CHARS = {"\ufffd", "\u25a1", "\u25a0"}
LANGUAGES = {
    "chinesesimplified": {"name": "Simplified Chinese", "locale": "zh-CN", "langid": 2052, "script": "Simplified Chinese characters"},
    "czech": {"name": "Czech", "locale": "cs-CZ", "langid": 1029, "script": "Czech Latin with accents"},
    "dutch": {"name": "Dutch", "locale": "nl-NL", "langid": 1043, "script": "Dutch Latin"},
    "french": {"name": "French", "locale": "fr-FR", "langid": 1036, "script": "French Latin with accents"},
    "german": {"name": "German", "locale": "de-DE", "langid": 1031, "script": "German Latin with umlauts and ß"},
    "hungarian": {"name": "Hungarian", "locale": "hu-HU", "langid": 1038, "script": "Hungarian Latin with accents"},
    "romanian": {"name": "Romanian", "locale": "ro-RO", "langid": 1048, "script": "Romanian Latin with ș/ț diacritics"},
    "russian": {"name": "Russian", "locale": "ru-RU", "langid": 1049, "script": "Cyrillic"},
    "slovak": {"name": "Slovak", "locale": "sk-SK", "langid": 1051, "script": "Slovak Latin with accents"},
    "spanish": {"name": "Spanish", "locale": "es-ES", "langid": 3082, "script": "Spanish Latin with accents"},
}

def language_info(language: str) -> dict:
    return LANGUAGES.get(language.lower(), {"name": language, "locale": language, "langid": None, "script": "the native script for the language"})

def normalize_translation_header(lines: list[str], language: str) -> int:
    """Keep SLT metadata aligned with the target language folder."""
    langid = language_info(language).get("langid")
    if langid is None:
        return 0
    in_translation = False
    for index, line in enumerate(lines):
        stripped = line.strip()
        if stripped == "[TRANSLATION]":
            in_translation = True
            continue
        if in_translation and stripped.startswith("["):
            return 0
        if in_translation and stripped.startswith("LANGID,"):
            desired = f"LANGID,{langid}"
            if stripped == desired:
                return 0
            ending = "\r\n" if line.endswith("\r\n") else "\n" if line.endswith("\n") else ""
            lines[index] = desired + ending
            return 1
    return 0

@dataclass
class Item:
    index: int; key: str; section: str; text: str; prefix: str; ending: str

def parse_items(lines: list[str], force: bool = False) -> list[Item]:
    section = ""
    items = []
    for index, line in enumerate(lines):
        header = SECTION_RE.match(line.rstrip("\r\n"))
        if header: section = line.strip(); continue
        if not section: continue
        match = LINE_RE.match(line)
        if not match or (match.group("state") != "0" and not force): continue
        key = f"{section}:{match.group('prefix').split(',', 1)[0]}:{index + 1}"
        items.append(Item(index, key, section, match.group("text"), match.group("prefix"), match.group("ending") or ""))
    return items

def accelerator_count(text: str) -> int:
    # A bare ampersand normally marks a Windows UI accelerator, but some
    # resource strings also contain prose/navigation text such as
    # "Time & Language" where the ampersand is a literal conjunction.
    # Treat only an ampersand with a non-space neighbor as an accelerator so
    # translations are not rejected merely for localizing that conjunction.
    count = 0
    for match in ACCELERATOR_RE.finditer(text):
        index = match.start()
        previous_char = text[index - 1] if index > 0 else ""
        next_char = text[index + 1] if index + 1 < len(text) else ""
        if previous_char.isspace() and next_char.isspace():
            continue
        count += 1
    return count

def tokens(text: str) -> tuple[list[str], int]:
    # The accelerator must remain present, but its target letter normally
    # changes in translation (for example "&File" becomes "&Soubor").
    return sorted(TOKEN_RE.findall(text)), accelerator_count(text)

def request_openai(payload: dict, api_key: str, model: str, attempts: int = 5, sleep=time.sleep) -> dict:
    language_name = payload.get("target_language", "the target language")
    language_script = payload.get("target_script", "the native script for the language")
    instructions = (
        "Translate Windows UI resources. Return only valid JSON with a translations array containing id and text. "
        "Preserve placeholders, escapes, accelerators (&), markup, paths, and technical tokens exactly. "
        f"Use natural {language_name} in {language_script}; do not transliterate, strip accents/diacritics, "
        "or replace unrepresentable characters with '?', boxes, replacement characters, or mojibake."
    )
    if payload.get("retry_instructions"):
        instructions += " " + payload["retry_instructions"]
    body = json.dumps({"model": model, "input": [{"role":"system","content":[{"type":"input_text","text":instructions}]},{"role":"user","content":[{"type":"input_text","text":json.dumps(payload, ensure_ascii=False)}]}], "text":{"format":{"type":"json_schema","name":"translations","strict":True,"schema":{"type":"object","properties":{"translations":{"type":"array","items":{"type":"object","properties":{"id":{"type":"string"},"text":{"type":"string"}},"required":["id","text"],"additionalProperties":False}}},"required":["translations"],"additionalProperties":False}}}}).encode()
    req = urllib.request.Request("https://api.openai.com/v1/responses", body, {"Authorization": f"Bearer {api_key}", "Content-Type":"application/json"})
    for attempt in range(attempts):
        try:
            with urllib.request.urlopen(req, timeout=300) as response: result=json.load(response)
            text = result.get("output_text")
            if text is None:
                text = next(c["text"] for o in result["output"] for c in o.get("content", []) if c.get("type") == "output_text")
            return json.loads(text)
        except (urllib.error.URLError, socket.timeout, TimeoutError) as exc:
            if attempt + 1 == attempts: raise
            sleep(2 ** attempt)
        except urllib.error.HTTPError as exc:
            if exc.code not in (408, 409, 429, 500, 502, 503, 504) or attempt + 1 == attempts: raise
            sleep(2 ** attempt)
    raise RuntimeError("OpenAI request failed")

def validate(items: list[Item], result: dict, language: str | None = None) -> dict[str,str]:
    rows=result.get("translations")
    if not isinstance(rows,list): raise ValueError("response has no translations array")
    output={}
    expected={i.key:i for i in items}
    for row in rows:
        if not isinstance(row,dict) or set(row) != {"id","text"} or row["id"] in output or row["id"] not in expected: raise ValueError("response contains invalid, duplicate, or unknown item")
        if "\n" in row["text"] or "\r" in row["text"]: raise ValueError(f"translated text contains newline for {row['id']}")
        if "\x00" in row["text"]: raise ValueError(f"translated text contains NUL byte for {row['id']}")
        if '"' in row["text"]: raise ValueError(f"translated text contains unescaped quote for {row['id']}")
        if any(ch in row["text"] for ch in REPLACEMENT_CHARS): raise ValueError(f"translated text contains replacement glyph for {row['id']}")
        if MOJIBAKE_RE.search(row["text"]): raise ValueError(f"translated text looks mojibaked for {row['id']}")
        if "??" in row["text"]: raise ValueError(f"translated text contains repeated question marks for {row['id']}")
        if tokens(row["text"]) != tokens(expected[row["id"]].text): raise ValueError(f"technical tokens changed for {row['id']}")
        output[row["id"]]=row["text"]
    if set(output) != set(expected): raise ValueError("response is incomplete")
    return output

def translate(path: Path, output: Path, language: str, model: str, batch_size: int, dry_run: bool, force: bool, requester=request_openai, trace_file: Path | None = None) -> dict:
    lang = language_info(language)
    lines=path.read_text(encoding="utf-8-sig").splitlines(keepends=True)
    header_updates = 0 if dry_run else normalize_translation_header(lines, language)
    items=parse_items(lines, force)
    all_items=parse_items(lines, True)
    report={"found":len(items),"translated":0,"skipped":len(all_items)-len(items),"failed":0,"estimated_input_characters":sum(len(i.text) for i in items)}
    if dry_run: return report
    if not items:
        if header_updates:
            output.write_text("".join(lines),encoding="utf-8-sig",newline="")
        return report
    key=os.environ.get("OPENAI_API_KEY")
    if not key: raise RuntimeError("OPENAI_API_KEY is not set")
    changed=list(lines)
    trace_handle = trace_file.open("a", encoding="utf-8") if trace_file else None

    def call_model(payload: dict) -> dict:
        if trace_handle:
            trace_handle.write(json.dumps({"event":"request","language":language,"item_count":len(payload["items"]),"ids":[i["id"] for i in payload["items"]]}, ensure_ascii=False)+"\n")
            trace_handle.flush()
        result = requester(payload,key,model)
        if trace_handle:
            trace_handle.write(json.dumps({"event":"response","language":language,"item_count":len(result.get("translations",[]))}, ensure_ascii=False)+"\n")
            trace_handle.flush()
        return result

    def translate_batch(batch: list[Item]) -> None:
        payload={"target_language":lang["name"],"target_language_key":language,"target_locale":lang["locale"],"target_langid":lang["langid"],"target_script":lang["script"],"items":[{"id":i.key,"resource_id":i.prefix.split(',',1)[0],"type":i.section,"text":i.text} for i in batch]}
        try:
            translated=validate(batch, call_model(payload), language)
        except ValueError as exc:
            if len(batch) == 1:
                retry_payload=dict(payload)
                retry_payload["retry_instructions"]="Previous output was rejected. Translate this one item again in the target language's native script. Keep every placeholder, backslash escape, XML/HTML tag, braced token, filesystem path, and accelerator count exactly as in the source. Do not use '?', replacement glyphs, stripped accents, transliteration, or mojibake for target-language characters."
                try:
                    translated=validate(batch, call_model(retry_payload), language)
                except ValueError as retry_exc:
                    report["failed"] += 1
                    print(f"translation skipped: {batch[0].key}: {retry_exc}", file=sys.stderr)
                    return
                except Exception:
                    report["failed"] += 1
                    raise
                for item in batch:
                    changed[item.index]=f'{item.prefix}1,"{translated[item.key]}"{item.ending}'
                    report["translated"] += 1
                return
            midpoint=max(1, len(batch)//2)
            translate_batch(batch[:midpoint])
            translate_batch(batch[midpoint:])
            return
        except (urllib.error.URLError, socket.timeout, TimeoutError) as exc:
            report["failed"] += len(batch)
            print(f"batch translation failed (network error, will continue): {exc}", file=sys.stderr)
            return
        except Exception:
            report["failed"] += len(batch)
            raise
        for item in batch:
            changed[item.index]=f'{item.prefix}1,"{translated[item.key]}"{item.ending}'
            report["translated"] += 1

    try:
        for start in range(0,len(items),batch_size):
            translate_batch(items[start:start+batch_size])
    finally:
        if trace_handle: trace_handle.close()
    expanded = expand_widths(changed, items)
    if expanded: report["widths_expanded"] = expanded
    output.write_text("".join(changed),encoding="utf-8-sig",newline="")
    return report

def expand_widths(changed: list[str], items: list[Item]) -> int:
    """Expand control and dialog widths for translated text longer than original."""
    item_map = {item.index: item for item in items}
    in_dialog = False
    dialog_title_idx = -1
    max_right = 0
    modified = 0

    def flush_dialog():
        nonlocal modified, dialog_title_idx, max_right
        if dialog_title_idx < 0 or max_right <= 0:
            return
        m = LINE_RE.match(changed[dialog_title_idx])
        if not m:
            return
        parts = m.group("prefix").rstrip(",").split(",")
        if not parts:
            return
        try:
            old_w = int(parts[0])
        except ValueError:
            return
        if max_right > old_w:
            ending = m.group("ending") or ""
            changed[dialog_title_idx] = f'{max_right},{parts[1]},{m.group("state")},"{m.group("text")}"{ending}'
            modified += 1

    for i, line in enumerate(changed):
        stripped = line.rstrip("\r\n")
        sm = SECTION_RE.match(stripped)
        if sm:
            if in_dialog:
                flush_dialog()
            in_dialog = sm.group("kind") == "DIALOG"
            dialog_title_idx = -1
            max_right = 0
            continue
        if not in_dialog:
            continue
        if i not in item_map:
            if dialog_title_idx < 0:
                dialog_title_idx = i
            continue
        item = item_map[i]
        parts = item.prefix.rstrip(",").split(",")
        if len(parts) < 5:
            continue
        try:
            x = int(parts[1])
            old_w = int(parts[3])
        except (ValueError, IndexError):
            continue
        m = LINE_RE.match(changed[i])
        if not m or m.group("state") != "1":
            max_right = max(max_right, x + old_w)
            continue
        trans_text = m.group("text")
        orig_len = len(item.text)
        trans_len = len(trans_text)
        if orig_len == 0 or trans_len <= orig_len:
            max_right = max(max_right, x + old_w)
            continue
        ratio = trans_len / orig_len
        new_w = int(old_w * ratio * 1.15)
        if new_w <= old_w:
            new_w = old_w + 1
        new_prefix = f"{parts[0]},{parts[1]},{parts[2]},{new_w},{parts[4]},"
        ending = m.group("ending") or ""
        changed[i] = f'{new_prefix}{m.group("state")},"{trans_text}"{ending}'
        modified += 1
        max_right = max(max_right, x + new_w)

    if in_dialog:
        flush_dialog()

    return modified

def main() -> int:
    p=argparse.ArgumentParser(); p.add_argument("input",type=Path); p.add_argument("output",type=Path); p.add_argument("--language",required=True); p.add_argument("--model",default=os.environ.get("OPENAI_MODEL","gpt-5-mini")); p.add_argument("--batch-size",type=int,default=40); p.add_argument("--dry-run",action="store_true"); p.add_argument("--force-retranslate",action="store_true"); p.add_argument("--trace-file",type=Path)
    a=p.parse_args()
    try: print(json.dumps(translate(a.input,a.output,a.language,a.model,a.batch_size,a.dry_run,a.force_retranslate,trace_file=a.trace_file),ensure_ascii=False)); return 0
    except Exception as exc: print(f"translation failed: {exc}",file=sys.stderr); return 1
if __name__ == "__main__": raise SystemExit(main())
