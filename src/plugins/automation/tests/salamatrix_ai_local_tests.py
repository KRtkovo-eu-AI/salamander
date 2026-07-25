"""Offline tests for the optional local Salamatrix AI command wrapper."""

from __future__ import annotations

import importlib.util
import json
import pathlib
import unittest
from unittest import mock


MODULE_PATH = pathlib.Path(__file__).parents[1] / "runtime" / "salamatrix_ai_local.py"
SPEC = importlib.util.spec_from_file_location("salamatrix_ai_local", MODULE_PATH)
assert SPEC is not None and SPEC.loader is not None
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


class _Response:
    def __init__(self, payload: bytes):
        self.payload = payload

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        return False

    def read(self, limit: int) -> bytes:
        return self.payload


class LocalProviderTests(unittest.TestCase):
    def test_prompt_carries_runtime_and_feedback(self):
        with mock.patch.dict(
            MODULE.os.environ,
            {"SALAMATRIX_AI_SYSTEM": "avoid network effects"},
            clear=False,
        ):
            prompt = MODULE._prompt(
                {
                    "prompt": "add a progress dialog",
                    "runtime": "Python.CPython",
                    "existingScript": "old script",
                    "feedback": "use only read-only APIs",
                }
            )
        self.assertIn("avoid network effects", prompt)
        self.assertIn("Python.CPython", prompt)
        self.assertIn("old script", prompt)
        self.assertIn("use only read-only APIs", prompt)

    def test_ollama_response_is_decoded(self):
        payload = json.dumps({"response": '{"script":"print(1)"}'}).encode("utf-8")
        with mock.patch.object(
            MODULE.urllib.request, "urlopen", return_value=_Response(payload)
        ) as urlopen:
            result = MODULE._request_ollama(
                "http://127.0.0.1:11434/api/generate", "test", "prompt", 110
            )
        self.assertEqual(result["script"], "print(1)")
        self.assertEqual(
            urlopen.call_args.args[0].full_url,
            "http://127.0.0.1:11434/api/generate",
        )
        self.assertEqual(urlopen.call_args.kwargs["timeout"], 110)

    def test_chat_response_is_decoded(self):
        payload = json.dumps(
            {"choices": [{"message": {"content": '{"script":"x"}'}}]}
        ).encode("utf-8")
        with mock.patch.object(
            MODULE.urllib.request, "urlopen", return_value=_Response(payload)
        ):
            result = MODULE._request_chat_completions(
                "http://127.0.0.1:8080/v1/chat/completions", "test", "prompt", 110
            )
        self.assertEqual(result, {"script": "x"})

    def test_model_json_fence_is_removed(self):
        self.assertEqual(
            MODULE._decode_model_json("```json\n{\"script\":\"x\"}\n```")['script'],
            "x",
        )


if __name__ == "__main__":
    unittest.main()
