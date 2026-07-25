"""Optional local Salamatrix AI command provider.

The Automation bridge intentionally knows only about a command that accepts one
JSON request on stdin and emits one JSON response on stdout.  This small
adapter makes a local Ollama server (or another compatible endpoint) usable
without adding a model SDK or a provider-specific dependency to Salamander.

Configure it explicitly, for example on Windows:

  set SALAMATRIX_AI_COMMAND=python.exe "...\\salamatrix_ai_local.py"

The endpoint, model, and protocol are opt-in environment settings.  The
default endpoint is Ollama's local /api/generate API.  For llama.cpp's
OpenAI-compatible server use SALAMATRIX_AI_PROTOCOL=chat-completions.
"""

from __future__ import annotations

import json
import os
import sys
import urllib.error
import urllib.request


MAX_REQUEST_BYTES = 1024 * 1024
MAX_RESPONSE_CHARS = 1024 * 1024
DEFAULT_TIMEOUT_SECONDS = 110


def _error(message: str) -> None:
    print(json.dumps({"error": message}, ensure_ascii=False), flush=True)


def _prompt(request: dict) -> str:
    prompt = request.get("prompt") or request.get("instruction") or ""
    runtime = request.get("runtime") or ""
    existing = request.get("existingScript") or ""
    feedback = request.get("feedback") or ""
    context = request.get("context")
    parts = [
        "Return only one JSON object with keys title, description, capabilities, estimatedEffects, script, and optional runtime.",
        "Do not wrap the JSON in Markdown fences.",
        "Generate a Salamander extension script, not prose.",
        f"Requested runtime: {runtime or 'choose a registered runtime'}",
        f"Task: {prompt}",
    ]
    system_prompt = os.environ.get("SALAMATRIX_AI_SYSTEM", "").strip()
    if system_prompt:
        parts.insert(0, "System policy:\n" + system_prompt)
    if existing:
        parts.append("Existing script to repair or improve:\n" + str(existing))
    if feedback:
        parts.append("Repair feedback:\n" + str(feedback))
    if context:
        parts.append("Host context:\n" + json.dumps(context, ensure_ascii=False))
    return "\n\n".join(parts)


def _decode_model_json(value):
    if isinstance(value, dict):
        return value
    if not isinstance(value, str):
        raise ValueError("local model returned a non-JSON response")
    text = value.strip()
    if text.startswith("```"):
        lines = text.splitlines()
        if lines and lines[0].startswith("```"):
            lines = lines[1:]
        if lines and lines[-1].strip() == "```":
            lines = lines[:-1]
        text = "\n".join(lines).strip()
    result = json.loads(text)
    if not isinstance(result, dict):
        raise ValueError("local model returned a JSON value instead of an object")
    return result


def _request_ollama(endpoint: str, model: str, prompt: str, timeout: float):
    body = {"model": model, "prompt": prompt, "stream": False, "format": "json"}
    request = urllib.request.Request(
        endpoint,
        data=json.dumps(body, ensure_ascii=False).encode("utf-8"),
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    with urllib.request.urlopen(request, timeout=timeout) as response:
        payload = response.read(MAX_RESPONSE_CHARS + 1)
    if len(payload) > MAX_RESPONSE_CHARS:
        raise ValueError("local model response exceeded the provider limit")
    decoded = json.loads(payload.decode("utf-8"))
    return _decode_model_json(decoded.get("response"))


def _request_chat_completions(endpoint: str, model: str, prompt: str, timeout: float):
    body = {
        "model": model,
        "messages": [{"role": "user", "content": prompt}],
        "temperature": 0.2,
        "response_format": {"type": "json_object"},
    }
    request = urllib.request.Request(
        endpoint,
        data=json.dumps(body, ensure_ascii=False).encode("utf-8"),
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    with urllib.request.urlopen(request, timeout=timeout) as response:
        payload = response.read(MAX_RESPONSE_CHARS + 1)
    if len(payload) > MAX_RESPONSE_CHARS:
        raise ValueError("local model response exceeded the provider limit")
    decoded = json.loads(payload.decode("utf-8"))
    choices = decoded.get("choices") or []
    if not choices or not isinstance(choices[0], dict):
        raise ValueError("chat-completions response has no choices")
    message = choices[0].get("message") or {}
    return _decode_model_json(message.get("content"))


def main() -> int:
    raw = sys.stdin.buffer.read(MAX_REQUEST_BYTES + 1)
    if len(raw) > MAX_REQUEST_BYTES:
        _error("AI request exceeded the provider limit")
        return 1
    try:
        request = json.loads(raw.decode("utf-8"))
        if not isinstance(request, dict):
            raise ValueError("AI request must be a JSON object")
        endpoint = os.environ.get(
            "SALAMATRIX_AI_ENDPOINT", "http://127.0.0.1:11434/api/generate"
        )
        model = os.environ.get("SALAMATRIX_AI_MODEL", "llama3.2")
        protocol = os.environ.get("SALAMATRIX_AI_PROTOCOL", "ollama").lower()
        configured_timeout = float(
            os.environ.get("SALAMATRIX_AI_LOCAL_TIMEOUT", DEFAULT_TIMEOUT_SECONDS)
        )
        timeout = max(1.0, min(configured_timeout, DEFAULT_TIMEOUT_SECONDS))
        prompt = _prompt(request)
        if protocol in ("chat", "chat-completions", "openai"):
            result = _request_chat_completions(endpoint, model, prompt, timeout)
        else:
            result = _request_ollama(endpoint, model, prompt, timeout)
        print(json.dumps(result, ensure_ascii=False), flush=True)
        return 0
    except (ValueError, TypeError, KeyError, UnicodeError) as exc:
        _error(str(exc))
        return 1
    except (urllib.error.URLError, TimeoutError, OSError) as exc:
        _error("local AI endpoint unavailable: " + str(exc))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
