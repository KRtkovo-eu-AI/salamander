# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Small standard-library Salamatrix worker for Python extensions.

The host starts this file instead of the extension entry point for persistent
workers.  It deliberately has no Salamander-specific native dependency: the
same SMX1 messages are usable by every supported language runtime.
"""

from __future__ import annotations

import argparse
import json
import runpy
import sys
from typing import Any, Callable, Dict, Optional


MAX_FRAME_BYTES = 1024 * 1024


class _Transport:
    def __init__(self) -> None:
        self._next_id = 1
        self._event_handlers: Dict[str, list[Callable[[dict], None]]] = {}
        self._subscriptions: Dict[str, str] = {}

    def _write(self, kind: str, request_id: int, payload: dict) -> None:
        body = json.dumps(payload, separators=(",", ":"), ensure_ascii=False)
        frame = f"SMX1\t{kind}\t{request_id}\t{body}\n".encode("utf-8")
        if len(frame) > MAX_FRAME_BYTES:
            raise RuntimeError("SMX1 frame exceeds the 1 MiB limit")
        sys.stdout.buffer.write(frame)
        sys.stdout.buffer.flush()

    def _read(self) -> tuple[str, int, dict]:
        line = sys.stdin.buffer.readline(MAX_FRAME_BYTES + 1)
        if not line:
            raise EOFError("Salamander host closed the worker channel")
        if len(line) > MAX_FRAME_BYTES or not line.endswith(b"\n"):
            raise RuntimeError("invalid or oversized SMX1 frame")
        line = line.rstrip(b"\r\n")
        parts = line.split(b"\t", 3)
        if len(parts) != 4 or parts[0] != b"SMX1":
            raise RuntimeError("invalid SMX1 frame header")
        try:
            request_id = int(parts[2].decode("ascii"), 10)
            payload = json.loads(parts[3].decode("utf-8"))
        except (ValueError, UnicodeDecodeError, json.JSONDecodeError) as exc:
            raise RuntimeError("invalid SMX1 frame payload") from exc
        if not isinstance(payload, dict):
            raise RuntimeError("SMX1 payload must be a JSON object")
        return parts[1].decode("ascii"), request_id, payload

    def _dispatch_event(self, payload: dict) -> None:
        name = payload.get("event")
        for callback in list(self._event_handlers.get(name, [])):
            callback(payload)

    def call(self, method: str, **arguments: Any) -> dict:
        request_id = self._next_id
        self._next_id += 1
        payload = {"method": method}
        payload.update(arguments)
        self._write("call", request_id, payload)
        while True:
            kind, response_id, response = self._read()
            if kind == "event":
                self._dispatch_event(response)
                continue
            if response_id != request_id:
                continue
            if kind == "error":
                raise RuntimeError(response.get("error", "host call failed"))
            if kind != "result":
                raise RuntimeError(f"unexpected SMX1 response: {kind}")
            if response.get("ok") is False:
                raise RuntimeError(response.get("error", "host call failed"))
            return response

    def handshake(self) -> None:
        self._write("hello", 0, {"protocol": 1, "runtime": "python"})
        while True:
            kind, request_id, response = self._read()
            if kind == "event":
                self._dispatch_event(response)
            elif kind == "result" and request_id == 0:
                if response.get("ok") is False:
                    raise RuntimeError("Salamander host rejected the worker")
                return

    def run_event_loop(self) -> None:
        while True:
            kind, _, payload = self._read()
            if kind == "event":
                self._dispatch_event(payload)
            elif kind == "shutdown":
                return


class _Commands:
    def __init__(self, transport: _Transport) -> None:
        self._transport = transport

    def execute(self, command_id: str) -> str:
        return self._transport.call(
            "salamander.commands.execute", commandId=command_id
        ).get("result", "error")

    def register(self, command_id: str, title: str,
                 plugin_menu: bool = True, context_menu: bool = False) -> bool:
        result = self._transport.call(
            "salamander.commands.register", commandId=command_id,
            title=title, pluginMenu=plugin_menu, contextMenu=context_menu
        )
        return bool(result.get("registered", False))

    def unregister(self, command_id: str) -> bool:
        result = self._transport.call(
            "salamander.commands.unregister", commandId=command_id
        )
        return bool(result.get("unregistered", False))


class _Storage:
    def __init__(self, transport: _Transport) -> None:
        self._transport = transport

    def get(self, key: str, default: Optional[str] = None) -> Optional[str]:
        result = self._transport.call("salamander.storage.get", key=key)
        return result.get("value", default) if result.get("type") == "string" else default

    def set(self, key: str, value: str) -> None:
        self._transport.call("salamander.storage.set", key=key, value=value)


class _FileOperations:
    def __init__(self, transport: _Transport) -> None:
        self._transport = transport

    def _run(self, operation: str) -> str:
        return self._transport.call(
            f"salamander.fileOperations.{operation}"
        ).get("result", "error")

    def rename(self) -> str:
        return self._run("rename")

    def copy(self) -> str:
        return self._run("copy")

    def move(self) -> str:
        return self._run("move")

    def delete(self) -> str:
        return self._run("delete")

    def create_directory(self) -> str:
        return self._run("createDirectory")

    def refresh(self) -> str:
        return self._run("refresh")

    def properties(self) -> str:
        return self._run("properties")


class _Sides:
    def __init__(self, transport: _Transport) -> None:
        self._transport = transport

    def active_tab(self, side: str = "source") -> dict:
        return self._transport.call("salamander.sides.activeTab", side=side)


class _UI:
    def __init__(self, transport: _Transport) -> None:
        self._transport = transport

    def message_box(self, message: str, title: str = "Salamander") -> int:
        return int(self._transport.call(
            "salamander.ui.messageBox", message=message, title=title
        ).get("result", 0))

    def input_box(self, prompt: str, title: str = "Salamander",
                  initial: str = "") -> dict:
        return self._transport.call(
            "salamander.ui.inputBox", prompt=prompt, title=title,
            initial=initial
        )

    def dialog(self, title: str = "Salamander") -> "_Dialog":
        result = self._transport.call("salamander.ui.dialog.create", title=title)
        return _Dialog(self._transport, str(result["dialogId"]))


class _Dialog:
    def __init__(self, transport: _Transport, dialog_id: str) -> None:
        self._transport = transport
        self.dialog_id = dialog_id

    def _add(self, kind: str, control_id: str, text: str = "", **kwargs) -> None:
        self._transport.call(
            "salamander.ui.dialog.add", dialogId=self.dialog_id,
            kind=kind, controlId=control_id, text=text, **kwargs
        )

    def add_label(self, control_id: str, text: str) -> None:
        self._add("label", control_id, text)

    def add_textbox(self, control_id: str, text: str = "",
                    read_only: bool = False) -> None:
        self._add("textbox", control_id, text, readOnly=read_only)

    def add_checkbox(self, control_id: str, text: str,
                     checked: bool = False) -> None:
        self._add("checkbox", control_id, text, checked=checked)

    def add_radio_button(self, control_id: str, text: str,
                         checked: bool = False) -> None:
        self._add("radio", control_id, text, checked=checked)

    def add_combo_box(self, control_id: str, text: str = "") -> None:
        self._add("combobox", control_id, text)

    def add_list_view(self, control_id: str) -> None:
        self._add("listview", control_id)

    def add_tree_view(self, control_id: str) -> None:
        self._add("treeview", control_id)

    def add_button(self, control_id: str, text: str,
                   dialog_result: int = 1) -> None:
        self._add("button", control_id, text, dialogResult=dialog_result)

    def show(self) -> int:
        return int(self._transport.call(
            "salamander.ui.dialog.show", dialogId=self.dialog_id
        ).get("result", 0))

    def get(self, control_id: str) -> dict:
        return self._transport.call(
            "salamander.ui.dialog.get", dialogId=self.dialog_id,
            controlId=control_id
        )

    def close(self) -> None:
        self._transport.call(
            "salamander.ui.dialog.destroy", dialogId=self.dialog_id
        )


class _AI:
    def __init__(self, transport: _Transport) -> None:
        self._transport = transport

    def generate(self, prompt: str, context: Optional[dict] = None,
                 provider: Optional[str] = None) -> dict:
        arguments = {"prompt": prompt}
        if context is not None:
            arguments["context"] = context
        if provider is not None:
            arguments["provider"] = provider
        return self._transport.call("salamander.ai.generate", **arguments)


class _Events:
    def __init__(self, transport: _Transport) -> None:
        self._transport = transport

    def subscribe(self, event: str, callback: Callable[[dict], None]) -> str:
        result = self._transport.call("salamander.events.subscribe", event=event)
        subscription_id = str(result["subscriptionId"])
        self._transport._event_handlers.setdefault(event, []).append(callback)
        self._transport._subscriptions[subscription_id] = event
        return subscription_id

    def unsubscribe(self, subscription_id: str) -> None:
        self._transport.call(
            "salamander.events.unsubscribe", subscriptionId=subscription_id
        )
        event = self._transport._subscriptions.pop(subscription_id, None)
        if event is not None and event in self._transport._event_handlers:
            callbacks = self._transport._event_handlers[event]
            if callbacks:
                callbacks.pop(0)
            if not callbacks:
                self._transport._event_handlers.pop(event, None)


class _Salamander:
    def __init__(self, transport: _Transport) -> None:
        self.commands = _Commands(transport)
        self.storage = _Storage(transport)
        self.file_operations = _FileOperations(transport)
        self.sides = _Sides(transport)
        self.ui = _UI(transport)
        self.ai = _AI(transport)
        self.events = _Events(transport)
        self.left_side = self.sides
        self.right_side = self.sides
        self.source_side = self.sides
        self.target_side = self.sides


def main() -> int:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--entry", required=True)
    args = parser.parse_args()
    transport = _Transport()
    transport.handshake()
    globals_for_script = {"Salamander": _Salamander(transport)}
    runpy.run_path(args.entry, init_globals=globals_for_script, run_name="__main__")
    transport.run_event_loop()
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (BrokenPipeError, EOFError):
        raise SystemExit(0)
    except Exception as exc:  # keep worker failures visible to a CLI caller
        print(f"Salamatrix worker failed: {exc}", file=sys.stderr)
        raise SystemExit(1)
