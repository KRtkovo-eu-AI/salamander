# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-07 | Verify Find phrase and flags round-trip through every runtime worker. | Shared viewer invocations must preserve Find context for issue #758. | https://github.com/KRtkovo-eu-AI/salamander/issues/758 | openai/gpt-5.6-luna-pro |
# | 2026-11-06 | Round-trip UTF-8 paths under legacy PowerShell console encodings. | Reproduce the CI Unicode failure without changing the caller console or weakening payload parity. | https://github.com/KRtkovo-eu-AI/salamander/pull/769 | GPT-6 (Codex) |
# | 2026-11-06 | Compare real five-runtime viewer calls over SMX1. | Assert shared optional-field types, omission, defaults, return values and host errors. | https://github.com/KRtkovo-eu-AI/salamander/pull/769 | GPT-6 (Codex) |
"""Run actual isolated workers against a fake host; never open a viewer or touch source files."""
import argparse
import base64
import json
import os
from pathlib import Path
import queue
import shutil
import subprocess
import sys
import tempfile
import threading
import time

ROOT = Path(__file__).resolve().parents[2]
PATH = "C:/Unicode-\u65e5\u010d/" + "\u65e5" * 120 + "/sample.json"
CASES = [
    ({}, {}),
    ("prism", {"renderer": "prism"}),
    ({"renderer": "document", "title": "0", "documentName": "\u65e5\U0001f600.json", "window": {"left": -23, "top": 0, "width": 640, "height": 480, "showCmd": 3, "alwaysOnTop": False}}, {"renderer": "document", "title": "0", "documentName": "\u65e5\U0001f600.json", "left": -23, "top": 0, "width": 640, "height": 480, "showCmd": 3, "alwaysOnTop": False}),
    ({"renderer": None, "title": True, "documentName": 0, "left": True, "top": "bad", "width": "640", "height": 1.25, "showCmd": 2147483648, "alwaysOnTop": "false"}, {}),
    ({"renderer": "", "title": " ", "documentName": "0", "left": -2147483648, "top": 2147483647, "width": 0, "height": -1, "showCmd": 1.0, "alwaysOnTop": True}, {"title": " ", "documentName": "0", "left": -2147483648, "top": 2147483647, "width": 0, "height": -1, "showCmd": 1, "alwaysOnTop": True}),
    ({"width": 800, "height": 600, "window": {"width": 320, "height": 240, "left": None, "top": -2147483649, "alwaysOnTop": None}}, {"width": 320, "height": 240}),
    ({"width": 800, "height": 600, "window": "bad"}, {"width": 800, "height": 600}),
    (None, {}),
]
PROGRAMS = {
    "javascript": (".mjs", """const path = await Salamander.commands.execute('viewer-options-path');
if (path !== Salamander.invocation.path) throw new Error('UTF-8 host response');
for (const item of Salamander.invocation.cases) {
  let failed = false;
  let opened;
  try { opened = await Salamander.ui.viewer(path, item.options); }
  catch { failed = true; }
  if (failed !== item.expectError || (!failed && opened !== item.expectedOpen)) throw new Error('viewer return/error parity');
}
await Salamander.commands.execute('viewer-options-complete');
"""),
    "python": (".py", """path = Salamander.commands.execute('viewer-options-path')
if path != Salamander.invocation['path']:
    raise RuntimeError('UTF-8 host response')
for item in Salamander.invocation['cases']:
    failed = False
    opened = None
    try:
        opened = Salamander.ui.viewer(path, item['options'])
    except Exception:
        failed = True
    if failed != item['expectError'] or (not failed and opened is not item['expectedOpen']):
        raise RuntimeError('viewer return/error parity')
Salamander.commands.execute('viewer-options-complete')
"""),
    "powershell": (".ps1", """$path = $Salamander.commands.Execute('viewer-options-path')
if ($path -cne $Salamander.invocation.path) { throw 'UTF-8 host response' }
foreach ($item in $Salamander.invocation.cases) {
    $failed = $false
    $opened = $null
    try { $opened = $Salamander.ui.Viewer($path, $item.options) }
    catch { $failed = $true }
    if ($failed -ne $item.expectError -or (!$failed -and ($opened -isnot [bool] -or $opened -ne $item.expectedOpen))) { throw 'viewer return/error parity' }
}
[void]$Salamander.commands.Execute('viewer-options-complete')
"""),
    "php": (".php", """<?php
$path = $Salamander->commands->execute('viewer-options-path');
if ($path !== $Salamander->invocation['path']) throw new RuntimeException('UTF-8 host response');
foreach ($Salamander->invocation['cases'] as $item) {
    $failed = false;
    $opened = null;
    try { $opened = $Salamander->ui->viewer($path, $item['options']); }
    catch (Throwable $error) { $failed = true; }
    if ($failed !== $item['expectError'] || (!$failed && $opened !== $item['expectedOpen'])) throw new RuntimeException('viewer return/error parity');
}
$Salamander->commands->execute('viewer-options-complete');
"""),
    "lua": (".lua", """local path = Salamander.commands.execute('viewer-options-path')
if path ~= Salamander.invocation.path then error('UTF-8 host response') end
for _, item in ipairs(Salamander.invocation.cases) do
    local ok, opened = pcall(Salamander.ui.viewer, path, item.options)
    if (not ok) ~= item.expectError or (ok and opened ~= item.expectedOpen) then error('viewer return/error parity') end
end
Salamander.commands.execute('viewer-options-complete')
"""),
}
WORKERS = {
    "javascript": "javascriptruntime/runtime/salamatrix_worker.mjs",
    "python": "pythonruntime/runtime/salamatrix_worker.py",
    "powershell": "powershellruntime/runtime/salamatrix_worker.ps1",
    "php": "phpruntime/runtime/salamatrix_worker.php",
    "lua": "luaruntime/runtime/salamatrix_worker.lua",
}

def source_contracts():
    for language, relative in WORKERS.items():
        source = (ROOT / "src/plugins" / relative).read_text(encoding="utf-8-sig")
        for field in ("documentName", "alwaysOnTop", "showCmd", "-2147483648", "2147483647"):
            assert field in source, f"{language} omits {field}"
    for relative in ("doc/salamatrix-ui.md", "doc/salamatrix-ui.html"):
        source = (ROOT / relative).read_text(encoding="utf-8-sig")
        assert "Viewer options" in source and "2147483647" in source
    # ui.dialogs already authorizes viewer.open; this option extension needs no new capability.
    for demo in ("javascript-node", "python", "powershell", "php", "lua"):
        manifest = json.loads((ROOT / "src/extensions/demos" / demo / "extension.json").read_text(encoding="utf-8"))
        assert "ui.dialogs" in manifest["capabilities"] and manifest.get("viewers")

def executable(language, explicit):
    if explicit:
        assert Path(explicit).is_file(), f"Interpreter not found: {explicit}"
        return explicit
    if language == "python":
        return sys.executable
    env_name = {"javascript": "SALAMATRIX_NODE", "powershell": "SALAMATRIX_POWERSHELL", "php": "SALAMATRIX_PHP", "lua": "SALAMATRIX_LUA"}[language]
    configured = os.environ.get(env_name)
    return configured or next((found for name in {"javascript": ["node"], "powershell": ["pwsh", "powershell"], "php": ["php"], "lua": ["lua", "lua54"]}[language] if (found := shutil.which(name))), None)

def worker_test(language, interpreter, directory, console_code_page=None):
    suffix, program = PROGRAMS[language]
    entry = directory / ("viewer-options" + suffix)
    entry.write_text(program, encoding="utf-8")
    cases = [{"options": options, "expectError": False, "expectedOpen": True} for options, _ in CASES]
    cases += [{"options": {}, "expectError": False, "expectedOpen": False}] * 3
    cases += [{"options": {}, "expectError": True, "expectedOpen": False}]
    invocation = json.dumps({"path": PATH, "cases": cases,
                             "searchPhrase": "Běloučký", "searchFlags": 3},
                            ensure_ascii=True, separators=(",", ":"))
    worker = ROOT / "src/plugins" / WORKERS[language]
    if language == "powershell":
        command = [interpreter, "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", str(worker), "-EntryPoint", str(entry), "-InvocationJson", invocation, "-OneShot"]
    else:
        command = [interpreter] + (["-B"] if language == "python" else []) + [str(worker), "--entry", str(entry), "--invocation-json", invocation, "--one-shot"]
    process_options = {}
    if console_code_page is not None:
        assert language == "powershell" and os.name == "nt"
        def quote(value):
            return "'" + value.replace("'", "''") + "'"
        bootstrap = (
            f"[Console]::InputEncoding = [Text.Encoding]::GetEncoding({console_code_page}); "
            f"[Console]::OutputEncoding = [Text.Encoding]::GetEncoding({console_code_page}); "
            f"& {quote(str(worker))} -EntryPoint {quote(str(entry))} "
            f"-InvocationJson {quote(invocation)} -OneShot"
        )
        command = [interpreter, "-NoProfile", "-ExecutionPolicy", "Bypass", "-EncodedCommand",
                   base64.b64encode(bootstrap.encode("utf-16le")).decode("ascii")]
        # Encoding setters affect only this new hidden console, never the caller's console.
        startupinfo = subprocess.STARTUPINFO()
        startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startupinfo.wShowWindow = 0
        process_options = {"startupinfo": startupinfo, "creationflags": subprocess.CREATE_NEW_CONSOLE}
    frames = queue.Queue()
    process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, encoding="utf-8", cwd=directory, **process_options)
    def read_lines():
        try:
            for line in process.stdout:
                frames.put(line)
        except UnicodeError as error:
            frames.put(error)
        finally:
            frames.put(None)
    reader = threading.Thread(target=read_lines, daemon=True)
    reader.start()
    calls, completed = 0, False
    deadline = time.monotonic() + 25
    try:
        while True:
            line = frames.get(timeout=max(0.1, deadline - time.monotonic()))
            if line is None:
                break
            if isinstance(line, UnicodeError):
                raise AssertionError(f"{language} invalid UTF-8 worker output") from line
            parts = line.rstrip("\r\n").split("\t", 3)
            assert len(parts) == 4 and parts[0] == "SMX1", f"{language} invalid frame {line!r}"
            kind, request_id, payload = parts[1], int(parts[2]), json.loads(parts[3])
            if kind == "error":
                raise AssertionError(f"{language} worker failed: {payload}")
            response = {"ok": True}
            if kind == "call" and payload.get("method") == "salamander.ui.viewer.open":
                expected = {"method": "salamander.ui.viewer.open", "path": PATH, "renderer": "auto",
                            "searchPhrase": "Běloučký", "searchFlags": 3}
                if calls < len(CASES):
                    expected.update(CASES[calls][1])
                assert payload == expected, f"{language} case {calls}: {payload!r} != {expected!r}"
                # Equality alone would treat Python True == 1; compare JSON field types too.
                for key, value in expected.items():
                    actual = payload[key]
                    assert type(actual) is type(value) or (type(value) is int and type(actual) is float and actual.is_integer()), f"{language} wrong wire type for {key}"
                if calls < len(CASES): response["opened"] = True
                elif calls == len(CASES): response["opened"] = False
                elif calls == len(CASES) + 1: response["opened"] = "true"
                elif calls == len(CASES) + 2: pass
                else: response = {"ok": False, "error": "viewer denied"}
                calls += 1
            elif kind == "call" and payload.get("method") == "salamander.commands.execute":
                if payload.get("commandId") == "viewer-options-path":
                    response["result"] = PATH
                else:
                    assert payload.get("commandId") == "viewer-options-complete"
                    completed = True
                    response["result"] = "ok"
            elif kind == "call" and payload.get("method") == "salamander.host.windowIcon":
                response["icon"] = ""
            elif kind != "hello":
                raise AssertionError(f"{language} unexpected request: {payload}")
            process.stdin.write("SMX1\tresult\t" + str(request_id) + "\t" + json.dumps(response, ensure_ascii=False) + "\n")
            process.stdin.flush()
        code = process.wait(timeout=max(0.1, deadline - time.monotonic()))
        error = process.stderr.read()
        assert code == 0 and completed and calls == len(cases), f"{language}: exit {code}, {calls}/{len(cases)} calls, complete={completed}: {error}"
    finally:
        if process.poll() is None:
            process.kill()
        process.wait(timeout=5)
        reader.join(timeout=2)
        process.stdin.close()
        process.stdout.close()
        process.stderr.close()
    encoding_context = f", console CP{console_code_page}" if console_code_page is not None else ""
    print(f"PASS {language}: {calls} real worker calls, Unicode path > MAX_PATH UTF-8 bytes{encoding_context}", flush=True)
    return {"language": language, "interpreter": interpreter, "calls": calls, "status": "PASS", "console_code_page": console_code_page}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for language in WORKERS:
        parser.add_argument("--" + language)
    parser.add_argument("--require-all", action="store_true")
    parser.add_argument("--results", type=Path)
    args = parser.parse_args()
    source_contracts()
    results = []
    with tempfile.TemporaryDirectory(prefix="viewer-worker-options-") as temp:
        directory = Path(temp)
        for language in WORKERS:
            interpreter = executable(language, getattr(args, language))
            if not interpreter:
                print(f"SKIPPED {language}: interpreter unavailable", flush=True)
                results.append({"language": language, "status": "SKIPPED", "reason": "interpreter unavailable"})
                continue
            code_pages = (None, 437, 1252) if language == "powershell" and os.name == "nt" else (None,)
            for code_page in code_pages:
                results.append(worker_test(language, interpreter, directory, code_page))
    if args.results:
        args.results.write_text(json.dumps(results, indent=2) + "\n", encoding="utf-8")
    return int(args.require_all and any(result["status"] != "PASS" for result in results))

if __name__ == "__main__":
    raise SystemExit(main())
