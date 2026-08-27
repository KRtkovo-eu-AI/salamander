#!/usr/bin/env python3
"""Translate untranslated SLT resource lines through a supported LLM provider."""
from __future__ import annotations
import argparse, json, os, re, shutil, socket, subprocess, sys, tempfile, time, urllib.error, urllib.request
from dataclasses import dataclass
from pathlib import Path

DEFAULT_CURSOR_MODEL = "grok-4.5"
DEFAULT_OPENAI_MODEL = "gpt-5-mini"
DEFAULT_OPENROUTER_MODEL = "openai/gpt-5.4-nano"

TRANSLATIONS_SCHEMA = {
    "type": "object",
    "properties": {
        "translations": {
            "type": "array",
            "items": {
                "type": "object",
                "properties": {"id": {"type": "string"}, "text": {"type": "string"}},
                "required": ["id", "text"],
                "additionalProperties": False,
            },
        }
    },
    "required": ["translations"],
    "additionalProperties": False,
}

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
    "italian": {"name": "Italian", "locale": "it-IT", "langid": 1040, "script": "Italian Latin with accents"},
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

def write_slt(path: Path, text: str) -> None:
    """Write an SLT as UTF-8 with BOM without normalizing its line endings."""
    with path.open("w", encoding="utf-8-sig", newline="") as handle:
        handle.write(text)

@dataclass
class Item:
    index: int; key: str; section: str; text: str; prefix: str; ending: str; source_text: str | None = None

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

def item_map_by_resource(lines: list[str]) -> dict[tuple[str, str], Item]:
    return {(item.section, item.prefix.split(',', 1)[0]): item for item in parse_items(lines, True)}

def attach_source_text(items: list[Item], source_lines: list[str] | None) -> list[Item]:
    if source_lines is None:
        for item in items:
            item.source_text = item.text
        return items
    source_items = item_map_by_resource(source_lines)
    for item in items:
        source_item = source_items.get((item.section, item.prefix.split(',', 1)[0]))
        item.source_text = source_item.text if source_item else item.text
    return items

def translation_context(lines: list[str], source_lines: list[str] | None, selected: set[str], limit: int = 80) -> list[dict[str, str]]:
    if source_lines is None or limit <= 0:
        return []
    source_items = item_map_by_resource(source_lines)
    examples = []
    for item in parse_items(lines, True):
        if item.key in selected:
            continue
        match = LINE_RE.match(lines[item.index])
        if not match or match.group("state") != "1":
            continue
        source_item = source_items.get((item.section, item.prefix.split(',', 1)[0]))
        if not source_item or source_item.text == item.text:
            continue
        examples.append({"source": source_item.text, "translation": item.text})
        if len(examples) >= limit:
            break
    return examples

def trim_items(lines: list[str], source_lines: list[str]) -> list[Item]:
    source_items = item_map_by_resource(source_lines)
    items = []
    for item in parse_items(lines, True):
        match = LINE_RE.match(lines[item.index])
        if not match or match.group("state") != "1":
            continue
        source_item = source_items.get((item.section, item.prefix.split(',', 1)[0]))
        if not source_item:
            continue
        item.source_text = source_item.text
        if len(item.text) > len(source_item.text):
            items.append(item)
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

def translation_instructions(payload: dict) -> str:
    language_name = payload.get("target_language", "the target language")
    language_script = payload.get("target_script", "the native script for the language")
    instructions = (
        "Translate Windows UI resources. Return only valid JSON with a translations array containing id and text. "
        "Do not use tools, edit files, or add markdown fences or commentary. "
        "Preserve placeholders, escapes, accelerators (&), markup, paths, and technical tokens exactly. "
        "Use the supplied existing_translations as translation memory for consistent terminology. "
        f"Use natural {language_name} in {language_script}; do not transliterate, strip accents/diacritics, "
        "or replace unrepresentable characters with '?', boxes, replacement characters, or mojibake. "
        "When max_length_chars is present, make the text concise and no longer than that limit if possible."
    )
    if payload.get("retry_instructions"):
        instructions += " " + payload["retry_instructions"]
    return instructions

def parse_json_object(text: str) -> dict:
    cleaned = (text or "").strip()
    if cleaned.startswith("```"):
        lines = cleaned.splitlines()
        if lines and lines[0].startswith("```"):
            lines = lines[1:]
        if lines and lines[-1].strip() == "```":
            lines = lines[:-1]
        cleaned = "\n".join(lines).strip()
    decoder = json.JSONDecoder()
    data = None
    for index, ch in enumerate(cleaned):
        if ch in "{[":
            data, _ = decoder.raw_decode(cleaned[index:])
            break
    if data is None:
        raise ValueError("response is not JSON")
    if isinstance(data, list):
        return {"translations": data}
    if not isinstance(data, dict):
        raise ValueError("response is not a JSON object")
    nested = data.get("result")
    if isinstance(nested, str) and "translations" not in data:
        return parse_json_object(nested)
    return data

def request_openai(payload: dict, api_key: str, model: str, attempts: int = 5, sleep=time.sleep) -> dict:
    instructions = translation_instructions(payload)
    body = json.dumps({"model": model, "input": [{"role":"system","content":[{"type":"input_text","text":instructions}]},{"role":"user","content":[{"type":"input_text","text":json.dumps(payload, ensure_ascii=False)}]}], "text":{"format":{"type":"json_schema","name":"translations","strict":True,"schema":TRANSLATIONS_SCHEMA}}}).encode()
    req = urllib.request.Request("https://api.openai.com/v1/responses", body, {"Authorization": f"Bearer {api_key}", "Content-Type":"application/json"})
    for attempt in range(attempts):
        try:
            with urllib.request.urlopen(req, timeout=300) as response: result=json.load(response)
            text = result.get("output_text")
            if text is None:
                text = next(c["text"] for o in result["output"] for c in o.get("content", []) if c.get("type") == "output_text")
            return parse_json_object(text)
        except (urllib.error.URLError, socket.timeout, TimeoutError) as exc:
            if attempt + 1 == attempts: raise
            sleep(2 ** attempt)
        except urllib.error.HTTPError as exc:
            if exc.code not in (408, 409, 429, 500, 502, 503, 504) or attempt + 1 == attempts: raise
            sleep(2 ** attempt)
    raise RuntimeError("OpenAI request failed")

def request_openrouter(payload: dict, api_key: str, model: str, attempts: int = 5, sleep=time.sleep) -> dict:
    """Call OpenRouter's OpenAI-compatible Chat Completions endpoint."""
    instructions = translation_instructions(payload)
    body = json.dumps({
        "model": model,
        "messages": [
            {"role": "system", "content": instructions},
            {"role": "user", "content": json.dumps(payload, ensure_ascii=False)},
        ],
        "response_format": {
            "type": "json_schema",
            "json_schema": {"name": "translations", "strict": True, "schema": TRANSLATIONS_SCHEMA},
        },
        "provider": {"require_parameters": True},
    }, ensure_ascii=False).encode("utf-8")
    headers = {
        "Authorization": f"Bearer {api_key}",
        "Content-Type": "application/json",
    }
    if os.environ.get("OPENROUTER_HTTP_REFERER"):
        headers["HTTP-Referer"] = os.environ["OPENROUTER_HTTP_REFERER"]
    if os.environ.get("OPENROUTER_APP_TITLE"):
        headers["X-Title"] = os.environ["OPENROUTER_APP_TITLE"]
    req = urllib.request.Request("https://openrouter.ai/api/v1/chat/completions", body, headers)
    for attempt in range(attempts):
        try:
            with urllib.request.urlopen(req, timeout=300) as response:
                result = json.load(response)
            choices = result.get("choices")
            if not choices or not isinstance(choices[0], dict):
                raise ValueError("OpenRouter response has no choices")
            message = choices[0].get("message") or {}
            text = message.get("content")
            if isinstance(text, list):
                text = "".join(part.get("text", "") for part in text if isinstance(part, dict))
            if not isinstance(text, str) or not text:
                raise ValueError("OpenRouter response has no message content")
            return parse_json_object(text)
        except (urllib.error.URLError, socket.timeout, TimeoutError) as exc:
            if attempt + 1 == attempts: raise
            sleep(2 ** attempt)
        except urllib.error.HTTPError as exc:
            if exc.code not in (408, 409, 429, 500, 502, 503, 504) or attempt + 1 == attempts: raise
            sleep(2 ** attempt)
    raise RuntimeError("OpenRouter request failed")

def _patch_windows_os_blocking() -> None:
    # cursor-sdk calls these POSIX helpers; they are missing on Windows until Python 3.12.
    if not hasattr(os, "get_blocking"):
        os.get_blocking = lambda fd: True  # type: ignore[attr-defined]
    if not hasattr(os, "set_blocking"):
        os.set_blocking = lambda fd, blocking: None  # type: ignore[attr-defined]

_patch_windows_os_blocking()

def find_cursor_cli() -> str | None:
    for name in ("agent", "cursor-agent"):
        path = shutil.which(name)
        if path:
            return path
    local_app = os.environ.get("LOCALAPPDATA")
    home = os.path.expanduser("~")
    candidates = []
    if local_app:
        candidates.append(os.path.join(local_app, "cursor-agent", "agent.exe"))
    candidates.extend([
        os.path.join(home, ".local", "bin", "agent.exe"),
        os.path.join(home, ".local", "bin", "agent"),
    ])
    for path in candidates:
        if os.path.isfile(path):
            return path
    return None

class CursorSession:
    """Reusable Cursor translator: SDK local agent with no tools, or Cursor CLI fallback."""
    def __init__(self, api_key: str, model: str):
        self.api_key = api_key
        self.model = model
        self._workdir = tempfile.mkdtemp(prefix="slt-cursor-")
        self._agent = None
        self._created = None
        self._cli = None
        _patch_windows_os_blocking()
        try:
            from cursor_sdk import Agent, AgentOptions, LocalAgentOptions
        except ImportError:
            self._require_cli()
            return
        options = AgentOptions(api_key=api_key, model=model, tools=[], local=LocalAgentOptions(cwd=self._workdir))
        try:
            created = Agent.create(options)
            self._created = created
            self._agent = created.__enter__() if hasattr(created, "__enter__") else created
        except Exception as exc:
            print(f"cursor-sdk local agent failed ({exc}); falling back to Cursor CLI.", file=sys.stderr)
            self._close_agent()
            self._require_cli(exc)

    def _require_cli(self, cause: BaseException | None = None) -> None:
        self._cli = find_cursor_cli()
        if self._cli:
            return
        shutil.rmtree(self._workdir, ignore_errors=True)
        hint = " Install with `pip install cursor-sdk` or the Cursor CLI, then set CURSOR_API_KEY."
        if cause is None:
            raise RuntimeError("cursor-sdk is not installed and the Cursor CLI (`agent`) was not found." + hint)
        raise RuntimeError(f"cursor-sdk failed ({cause}) and the Cursor CLI (`agent`) was not found." + hint) from cause

    def _close_agent(self) -> None:
        try:
            if self._created is not None and hasattr(self._created, "__exit__"):
                self._created.__exit__(None, None, None)
            elif self._agent is not None and hasattr(self._agent, "close"):
                self._agent.close()
        except Exception:
            pass
        self._created = None
        self._agent = None

    def close(self) -> None:
        try:
            self._close_agent()
        finally:
            shutil.rmtree(self._workdir, ignore_errors=True)

    def __enter__(self): return self
    def __exit__(self, exc_type, exc, tb): self.close(); return False

    def __call__(self, payload: dict, api_key: str, model: str, attempts: int = 5, sleep=time.sleep) -> dict:
        return self.request(payload, attempts=attempts, sleep=sleep)

    def request(self, payload: dict, attempts: int = 5, sleep=time.sleep) -> dict:
        prompt = translation_instructions(payload) + "\n\n" + json.dumps(payload, ensure_ascii=False)
        last_error: Exception | None = None
        for attempt in range(attempts):
            try:
                text = self._complete(prompt)
                return parse_json_object(text)
            except Exception as exc:
                last_error = exc
                if not _is_retryable_cursor_error(exc) or attempt + 1 == attempts:
                    raise
                wait = getattr(exc, "retry_after", None)
                sleep(wait if isinstance(wait, (int, float)) else 2 ** attempt)
        raise last_error or RuntimeError("Cursor request failed")

    def _complete_sdk(self, prompt: str) -> str:
        run = self._agent.send(prompt)
        result = run.wait()
        if getattr(result, "status", None) == "error":
            raise RuntimeError(f"Cursor run failed: {getattr(result, 'id', '')}".strip())
        text = getattr(result, "result", None)
        if not text and hasattr(run, "text"):
            text = run.text()
        if text is None:
            raise ValueError("Cursor run returned no text")
        return text if isinstance(text, str) else str(text)

    def _complete(self, prompt: str) -> str:
        if self._agent is not None:
            try:
                return self._complete_sdk(prompt)
            except AttributeError as exc:
                if "get_blocking" not in str(exc) and "set_blocking" not in str(exc):
                    raise
                print(f"cursor-sdk hit a Windows Python gap ({exc}); falling back to Cursor CLI.", file=sys.stderr)
                self._close_agent()
                self._require_cli(exc)
        env = os.environ.copy()
        env["CURSOR_API_KEY"] = self.api_key
        args = [self._cli, "-p", "--mode", "ask", "--model", self.model, "--output-format", "text", "--trust", "--workspace", self._workdir]
        # Windows command lines cap around 8k characters; keep long JSON payloads on stdin.
        input_text = None if len(prompt) < 4000 else prompt
        if input_text is None:
            args.append(prompt)
        completed = subprocess.run(
            args, input=input_text, capture_output=True, text=True, encoding="utf-8", timeout=300, env=env, check=False,
        )
        if completed.returncode != 0:
            detail = (completed.stderr or completed.stdout or "").strip() or f"exit {completed.returncode}"
            error = RuntimeError(f"Cursor CLI failed: {detail}")
            if _looks_like_rate_limit(detail):
                error.is_retryable = True  # type: ignore[attr-defined]
            raise error
        return completed.stdout

def _looks_like_rate_limit(text: str) -> bool:
    lowered = text.lower()
    return any(token in lowered for token in ("429", "rate limit", "too many requests", "overloaded", "temporar"))

def _is_retryable_cursor_error(exc: BaseException) -> bool:
    if getattr(exc, "is_retryable", False):
        return True
    if isinstance(exc, (socket.timeout, TimeoutError, subprocess.TimeoutExpired)):
        return True
    name = type(exc).__name__
    if name == "CursorAgentError":
        return bool(getattr(exc, "is_retryable", False))
    return False

def request_cursor(payload: dict, api_key: str, model: str, attempts: int = 5, sleep=time.sleep) -> dict:
    with CursorSession(api_key, model) as session:
        return session.request(payload, attempts=attempts, sleep=sleep)

def resolve_api_key(provider: str) -> str | None:
    if provider == "openai":
        return os.environ.get("OPENAI_API_KEY")
    if provider == "openrouter":
        return os.environ.get("OPENROUTER_API_KEY")
    return os.environ.get("CURSOR_API_KEY")

def default_model(provider: str) -> str:
    if provider == "openai":
        return os.environ.get("OPENAI_MODEL") or DEFAULT_OPENAI_MODEL
    if provider == "openrouter":
        return os.environ.get("OPENROUTER_MODEL") or DEFAULT_OPENROUTER_MODEL
    return os.environ.get("CURSOR_MODEL") or os.environ.get("OPENAI_MODEL") or DEFAULT_CURSOR_MODEL

def validate(items: list[Item], result: dict, language: str | None = None, enforce_max_length: bool = False) -> dict[str,str]:
    rows=result.get("translations")
    if not isinstance(rows,list): raise ValueError("response has no translations array")
    output={}
    expected={i.key:i for i in items}
    for row in rows:
        if not isinstance(row,dict) or set(row) != {"id","text"} or row["id"] in output or row["id"] not in expected: raise ValueError("response contains invalid, duplicate, or unknown item")
        if "\n" in row["text"] or "\r" in row["text"]: raise ValueError(f"translated text contains newline for {row['id']}")
        if "\x00" in row["text"]: raise ValueError(f"translated text contains NUL byte for {row['id']}")
        # SLT text fields are delimited by the final quote on the line; literal
        # quotes inside the field are valid and are present in the English resources.
        if any(ch in row["text"] for ch in REPLACEMENT_CHARS): raise ValueError(f"translated text contains replacement glyph for {row['id']}")
        if MOJIBAKE_RE.search(row["text"]): raise ValueError(f"translated text looks mojibaked for {row['id']}")
        if "??" in row["text"]: raise ValueError(f"translated text contains repeated question marks for {row['id']}")
        if tokens(row["text"]) != tokens(expected[row["id"]].text): raise ValueError(f"technical tokens changed for {row['id']}")
        if enforce_max_length and expected[row["id"]].source_text is not None and len(row["text"]) > len(expected[row["id"]].source_text): raise ValueError(f"translated text is longer than source for {row['id']}")
        output[row["id"]]=row["text"]
    if set(output) != set(expected): raise ValueError("response is incomplete")
    return output

def translate(path: Path, output: Path, language: str, model: str, batch_size: int, dry_run: bool, force: bool, requester=None, trace_file: Path | None = None, source_archive: Path | None = None, trim_translations: bool = False, provider: str | None = None) -> dict:
    lang = language_info(language)
    lines=path.read_text(encoding="utf-8-sig").splitlines(keepends=True)
    source_lines=source_archive.read_text(encoding="utf-8-sig").splitlines(keepends=True) if source_archive else None
    header_updates = 0 if dry_run else normalize_translation_header(lines, language)
    if trim_translations:
        if source_lines is None:
            raise RuntimeError("--trim-translations requires --source-archive")
        items=trim_items(lines, source_lines)
    else:
        items=parse_items(lines, force)
        attach_source_text(items, source_lines)
    all_items=parse_items(lines, True)
    report={"found":len(items),"translated":0,"skipped":len(all_items)-len(items),"failed":0,"estimated_input_characters":sum(len(i.text) for i in items)}
    if dry_run: return report
    if not items:
        if header_updates:
            write_slt(output, "".join(lines))
        return report
    provider = (provider or os.environ.get("TRANSLATION_PROVIDER") or "openrouter").lower()
    owns_requester = requester is None
    if owns_requester:
        key=resolve_api_key(provider)
        if provider == "openai":
            if not key: raise RuntimeError("OPENAI_API_KEY is not set")
            requester = request_openai
        elif provider == "openrouter":
            if not key: raise RuntimeError("OPENROUTER_API_KEY is not set")
            requester = request_openrouter
        else:
            if not key: raise RuntimeError("CURSOR_API_KEY is not set")
            requester = CursorSession(key, model)
    else:
        key=resolve_api_key(provider) or os.environ.get("OPENAI_API_KEY") or os.environ.get("CURSOR_API_KEY") or ""
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
        selected={i.key for i in batch}
        payload={"target_language":lang["name"],"target_language_key":language,"target_locale":lang["locale"],"target_langid":lang["langid"],"target_script":lang["script"],"mode":"trim" if trim_translations else "translate","existing_translations":translation_context(lines, source_lines, selected),"items":[{"id":i.key,"resource_id":i.prefix.split(',',1)[0],"type":i.section,"text":i.text,"source_text":i.source_text or i.text,"max_length_chars":len(i.source_text or i.text)} for i in batch]}
        try:
            translated=validate(batch, call_model(payload), language, enforce_max_length=trim_translations)
        except ValueError as exc:
            if len(batch) == 1:
                retry_payload=dict(payload)
                retry_payload["retry_instructions"]="Previous output was rejected. Translate this one item again in the target language's native script. Keep every placeholder, backslash escape, XML/HTML tag, braced token, filesystem path, and accelerator count exactly as in the source. Do not use '?', replacement glyphs, stripped accents, transliteration, or mojibake for target-language characters. If max_length_chars is present, shorten the translation to fit that length."
                try:
                    translated=validate(batch, call_model(retry_payload), language, enforce_max_length=trim_translations)
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
        if owns_requester and hasattr(requester, "close"):
            requester.close()
    expanded = expand_widths(changed, items)
    if expanded: report["widths_expanded"] = expanded
    write_slt(output, "".join(changed))
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
    p=argparse.ArgumentParser(); p.add_argument("input",type=Path); p.add_argument("output",type=Path); p.add_argument("--language",required=True); p.add_argument("--provider",choices=("cursor","openai","openrouter"),default=os.environ.get("TRANSLATION_PROVIDER","openrouter")); p.add_argument("--model",default=None); p.add_argument("--batch-size",type=int,default=40); p.add_argument("--dry-run",action="store_true"); p.add_argument("--force-retranslate",action="store_true"); p.add_argument("--trace-file",type=Path); p.add_argument("--source-archive",type=Path); p.add_argument("--trim-translations",action="store_true")
    a=p.parse_args(); model=a.model or default_model(a.provider)
    try: print(json.dumps(translate(a.input,a.output,a.language,model,a.batch_size,a.dry_run,a.force_retranslate,trace_file=a.trace_file,source_archive=a.source_archive,trim_translations=a.trim_translations,provider=a.provider),ensure_ascii=False)); return 0
    except Exception as exc: print(f"translation failed: {exc}",file=sys.stderr); return 1
if __name__ == "__main__": raise SystemExit(main())
