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

def tokens(text: str) -> tuple[list[str], int]:
    # The accelerator must remain present, but its target letter normally
    # changes in translation (for example "&File" becomes "&Soubor").
    return sorted(TOKEN_RE.findall(text)), len(ACCELERATOR_RE.findall(text))

def request_openai(payload: dict, api_key: str, model: str, attempts: int = 5, sleep=time.sleep) -> dict:
    instructions = "Translate Windows UI resources. Return only valid JSON with a translations array containing id and text. Preserve placeholders, escapes, accelerators (&), markup, paths, and technical tokens exactly."
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

def validate(items: list[Item], result: dict) -> dict[str,str]:
    rows=result.get("translations")
    if not isinstance(rows,list): raise ValueError("response has no translations array")
    output={}
    expected={i.key:i for i in items}
    for row in rows:
        if not isinstance(row,dict) or set(row) != {"id","text"} or row["id"] in output or row["id"] not in expected: raise ValueError("response contains invalid, duplicate, or unknown item")
        if "\n" in row["text"] or "\r" in row["text"]: raise ValueError(f"translated text contains newline for {row['id']}")
        if '"' in row["text"]: raise ValueError(f"translated text contains unescaped quote for {row['id']}")
        if tokens(row["text"]) != tokens(expected[row["id"]].text): raise ValueError(f"technical tokens changed for {row['id']}")
        output[row["id"]]=row["text"]
    if set(output) != set(expected): raise ValueError("response is incomplete")
    return output

def translate(path: Path, output: Path, language: str, model: str, batch_size: int, dry_run: bool, force: bool, requester=request_openai, trace_file: Path | None = None) -> dict:
    lines=path.read_text(encoding="utf-8-sig").splitlines(keepends=True); items=parse_items(lines, force)
    all_items=parse_items(lines, True)
    report={"found":len(items),"translated":0,"skipped":len(all_items)-len(items),"failed":0,"estimated_input_characters":sum(len(i.text) for i in items)}
    if not items or dry_run: return report
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
        payload={"target_language":language,"items":[{"id":i.key,"resource_id":i.prefix.split(',',1)[0],"type":i.section,"text":i.text} for i in batch]}
        try:
            translated=validate(batch, call_model(payload))
        except ValueError as exc:
            if len(batch) == 1:
                retry_payload=dict(payload)
                retry_payload["retry_instructions"]="Previous output was rejected. Translate this one item again, but keep every placeholder, backslash escape, XML/HTML tag, braced token, filesystem path, and accelerator count exactly as in the source."
                try:
                    translated=validate(batch, call_model(retry_payload))
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
    if not dry_run: output.write_text("".join(changed),encoding="utf-8-sig",newline="")
    return report

def main() -> int:
    p=argparse.ArgumentParser(); p.add_argument("input",type=Path); p.add_argument("output",type=Path); p.add_argument("--language",required=True); p.add_argument("--model",default=os.environ.get("OPENAI_MODEL","gpt-5-mini")); p.add_argument("--batch-size",type=int,default=40); p.add_argument("--dry-run",action="store_true"); p.add_argument("--force-retranslate",action="store_true"); p.add_argument("--trace-file",type=Path)
    a=p.parse_args()
    try: print(json.dumps(translate(a.input,a.output,a.language,a.model,a.batch_size,a.dry_run,a.force_retranslate,trace_file=a.trace_file),ensure_ascii=False)); return 0
    except Exception as exc: print(f"translation failed: {exc}",file=sys.stderr); return 1
if __name__ == "__main__": raise SystemExit(main())
