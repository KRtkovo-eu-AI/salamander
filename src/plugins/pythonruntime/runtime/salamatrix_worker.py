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
                 plugin_menu: bool = True, context_menu: bool = False,
                 hot_key: int = 0, toolbar: bool = False,
                 handler: str = "") -> bool:
        result = self._transport.call(
            "salamander.commands.register", commandId=command_id,
            title=title, pluginMenu=plugin_menu, contextMenu=context_menu,
            hotKey=int(hot_key), toolbar=toolbar, handler=handler
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

    def remove(self, key: str) -> bool:
        return bool(self._transport.call(
            "salamander.storage.remove", key=key
        ).get("removed", False))

    def clear(self) -> bool:
        return bool(self._transport.call(
            "salamander.storage.clear"
        ).get("ok", False))


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

    def context(self, side: str = "source") -> dict:
        return self._transport.call("salamander.sides.context", side=side)

    def tabs(self, side: str = "source") -> list[dict]:
        return self._transport.call("salamander.sides.tabs", side=side).get(
            "tabs", []
        )

    def activate_tab(self, tab_id: str, focus: bool = True) -> bool:
        return bool(self._transport.call(
            "salamander.sides.activateTab", tabId=str(tab_id), focus=focus
        ).get("activated", False))

    def change_path(self, path: str, side: str = "source") -> dict:
        return self._transport.call(
            "salamander.sides.changePath", side=side, path=path
        )

    def refresh(self, side: str = "source", force: bool = False,
                focus_first_new_item: bool = False) -> bool:
        return bool(self._transport.call(
            "salamander.sides.refresh", side=side, force=force,
            focusFirstNewItem=focus_first_new_item
        ).get("ok", False))

    def select_item(self, index: int, select: bool = True,
                    side: str = "source", repaint: bool = True) -> bool:
        return bool(self._transport.call(
            "salamander.sides.selectItem", side=side, index=int(index),
            select=select, repaint=repaint
        ).get("changed", False))

    def select_all(self, select: bool = True, side: str = "source",
                   repaint: bool = True) -> bool:
        return bool(self._transport.call(
            "salamander.sides.selectAll", side=side, select=select,
            repaint=repaint
        ).get("changed", False))

    def focus_item(self, index: int, side: str = "source",
                   part_visible: bool = True) -> bool:
        return bool(self._transport.call(
            "salamander.sides.focusItem", side=side, index=int(index),
            partVisible=part_visible
        ).get("changed", False))


class _Side:
    def __init__(self, sides: _Sides, name: str) -> None:
        self._sides = sides
        self._name = name

    def active_tab(self, side: Optional[str] = None) -> dict:
        return self._sides.active_tab(self._name if side is None else side)

    def context(self, side: Optional[str] = None) -> dict:
        return self._sides.context(self._name if side is None else side)

    def tabs(self) -> list[dict]:
        return self._sides.tabs(self._name)

    def activate_tab(self, tab_id: str, focus: bool = True) -> bool:
        return self._sides.activate_tab(tab_id, focus)

    def change_path(self, path: str) -> dict:
        return self._sides.change_path(path, self._name)

    def refresh(self, force: bool = False,
                focus_first_new_item: bool = False) -> bool:
        return self._sides.refresh(
            self._name, force, focus_first_new_item
        )

    def select_item(self, index: int, select: bool = True,
                    repaint: bool = True) -> bool:
        return self._sides.select_item(index, select, self._name, repaint)

    def select_all(self, select: bool = True, repaint: bool = True) -> bool:
        return self._sides.select_all(select, self._name, repaint)

    def focus_item(self, index: int, part_visible: bool = True) -> bool:
        return self._sides.focus_item(index, self._name, part_visible)


class _UI:
    def __init__(self, transport: _Transport) -> None:
        self._transport = transport

    def message_box(self, message: str, title: str = "Salamander") -> int:
        return int(self._transport.call(
            "salamander.ui.messageBox", message=message, title=title
        ).get("result", 0))

    def notify(self, message: str, title: str = "Salamander",
               timeout_ms: int = 5000) -> bool:
        return bool(self._transport.call(
            "salamander.ui.notify", message=message, title=title,
            timeoutMs=max(0, int(timeout_ms))
        ).get("shown", False))

    def input_box(self, prompt: str, title: str = "Salamander",
                  initial: str = "") -> dict:
        return self._transport.call(
            "salamander.ui.inputBox", prompt=prompt, title=title,
            initial=initial
        )

    def pick_file(self, save: bool = False, title: str = "",
                  filter: str = "", initial: str = "") -> dict:
        return self._transport.call(
            "salamander.ui.pickFile", save=save, title=title,
            filter=filter, initial=initial
        )

    def pick_folder(self, title: str = "", initial: str = "") -> dict:
        return self._transport.call(
            "salamander.ui.pickFolder", title=title, initial=initial
        )

    def progress(self, title: str = "Salamatrix", total: int = 0,
                 two_progress_bars: bool = False,
                 file_progress: bool = False,
                 cancel_enabled: bool = True,
                 total2: Optional[int] = None) -> "_Progress":
        arguments: dict = {
            "title": title, "total": int(total),
            "twoProgressBars": two_progress_bars,
            "fileProgress": file_progress,
            "cancelEnabled": cancel_enabled,
        }
        if total2 is not None:
            arguments["total2"] = int(total2)
        result = self._transport.call(
            "salamander.ui.progress.create", **arguments
        )
        return _Progress(self._transport, str(result["progressId"]))

    def dialog(self, title: str = "Salamander", width: int = 320,
               height: int = 180) -> "_Dialog":
        result = self._transport.call(
            "salamander.ui.dialog.create", title=title,
            width=int(width), height=int(height)
        )
        return _Dialog(self._transport, str(result["dialogId"]))


class _Progress:
    def __init__(self, transport: _Transport, progress_id: str) -> None:
        self._transport = transport
        self.progress_id = progress_id
        self._closed = False

    def update(self, position: int, total: Optional[int] = None,
               text: str = "", delayed_paint: bool = True,
               position2: Optional[int] = None,
               total2: Optional[int] = None) -> bool:
        arguments: dict = {
            "progressId": self.progress_id,
            "position": int(position),
            "text": text,
            "delayedPaint": delayed_paint,
        }
        if total is not None:
            arguments["total"] = int(total)
        if position2 is not None:
            arguments["position2"] = int(position2)
        if total2 is not None:
            arguments["total2"] = int(total2)
        return bool(self._transport.call(
            "salamander.ui.progress.update", **arguments
        ).get("continued", True))

    def set_totals(self, total: int, total2: int) -> None:
        self._transport.call(
            "salamander.ui.progress.setTotals", progressId=self.progress_id,
            total=int(total), total2=int(total2)
        )

    def set_positions(self, position: int, position2: int,
                      delayed_paint: bool = True) -> bool:
        return bool(self._transport.call(
            "salamander.ui.progress.setPositions", progressId=self.progress_id,
            position=int(position), position2=int(position2),
            delayedPaint=delayed_paint
        ).get("continued", True))

    def set_title(self, title: str) -> None:
        self._transport.call(
            "salamander.ui.progress.setTitle", progressId=self.progress_id,
            title=title
        )

    def set_cancel_enabled(self, enabled: bool) -> None:
        self._transport.call(
            "salamander.ui.progress.setCancelEnabled",
            progressId=self.progress_id, enabled=enabled
        )

    def step(self, amount: int = 1, delayed_paint: bool = True) -> bool:
        return bool(self._transport.call(
            "salamander.ui.progress.step", progressId=self.progress_id,
            amount=int(amount), delayedPaint=delayed_paint
        ).get("continued", True))

    def is_cancelled(self) -> bool:
        return bool(self._transport.call(
            "salamander.ui.progress.cancelled", progressId=self.progress_id
        ).get("cancelled", False))

    def close(self) -> None:
        if not self._closed:
            self._transport.call(
                "salamander.ui.progress.close", progressId=self.progress_id
            )
            self._closed = True

    def __enter__(self) -> "_Progress":
        return self

    def __exit__(self, exc_type, exc_value, traceback) -> None:
        self.close()


class _Clipboard:
    def __init__(self, transport: _Transport) -> None:
        self._transport = transport

    def copy_text(self, text: str, show_echo: bool = False) -> bool:
        return bool(self._transport.call(
            "salamander.clipboard.copyText", text=text,
            showEcho=show_echo
        ).get("copied", False))


class _Dialog:
    def __init__(self, transport: _Transport, dialog_id: str) -> None:
        self._transport = transport
        self.dialog_id = dialog_id

    def _add(self, kind: str, control_id: str, text: str = "", **kwargs) -> None:
        self._transport.call(
            "salamander.ui.dialog.add", dialogId=self.dialog_id,
            kind=kind, controlId=control_id, text=text, **kwargs
        )

    def add_control(self, kind: str, control_id: str, text: str = "",
                    read_only: bool = False, checked: bool = False,
                    dialog_result: int = 0,
                    layout: Optional[dict] = None,
                    keep_open: bool = False,
                    multiline: bool = False) -> None:
        arguments: dict = {
            "readOnly": read_only,
            "checked": checked,
            "dialogResult": dialog_result,
            "keepOpen": keep_open,
            "multiline": multiline,
        }
        if layout is not None:
            for name in ("x", "y", "width", "height"):
                if name in layout:
                    arguments[name] = int(layout[name])
        self._add(kind, control_id, text, **arguments)

    def set_validation(self, control_id: str, required: bool = False,
                       message: str = "") -> None:
        self._transport.call(
            "salamander.ui.dialog.validation", dialogId=self.dialog_id,
            controlId=control_id, required=required, message=message
        )

    def on_change(self, callback: Callable[[dict], None]) -> str:
        event_name = f"salamander.ui.dialog.{self.dialog_id}.changed"
        self._transport.call(
            "salamander.ui.dialog.events", dialogId=self.dialog_id,
            enabled=True, event=event_name
        )
        self._transport._event_handlers.setdefault(event_name, []).append(callback)
        return event_name

    def off_change(self, event_name: str = "") -> None:
        name = event_name or f"salamander.ui.dialog.{self.dialog_id}.changed"
        self._transport.call(
            "salamander.ui.dialog.events", dialogId=self.dialog_id,
            enabled=False, event=name
        )
        self._transport._event_handlers.pop(name, None)

    def add_label(self, control_id: str, text: str) -> None:
        self._add("label", control_id, text)

    def add_textbox(self, control_id: str, text: str = "",
                    read_only: bool = False, multiline: bool = False) -> None:
        self._add("textbox", control_id, text, readOnly=read_only,
                  multiline=multiline)

    def add_folder_picker(self, control_id: str, path: str = "") -> None:
        self._add("folderpicker", control_id, path)

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

    def add_tab_control(self, control_id: str) -> None:
        self._add("tabcontrol", control_id)

    def add_item(self, control_id: str, text: str,
                 parent_index: int = -1) -> int:
        result = self._transport.call(
            "salamander.ui.dialog.item", dialogId=self.dialog_id,
            controlId=control_id, text=text, parentIndex=parent_index
        )
        return int(result.get("itemCount", 0))

    def add_column(self, control_id: str, title: str,
                   width: int = 180) -> None:
        self._transport.call(
            "salamander.ui.dialog.column", dialogId=self.dialog_id,
            controlId=control_id, title=title, width=int(width)
        )

    def set_selected_index(self, control_id: str, index: int) -> int:
        result = self._transport.call(
            "salamander.ui.dialog.selection", dialogId=self.dialog_id,
            controlId=control_id, index=int(index)
        )
        return int(result.get("selectedIndex", -1))

    def clear_items(self, control_id: str) -> None:
        self._transport.call(
            "salamander.ui.dialog.clearItems", dialogId=self.dialog_id,
            controlId=control_id
        )

    def add_button(self, control_id: str, text: str,
                   dialog_result: int = 1, keep_open: bool = False) -> None:
        self._add("button", control_id, text, dialogResult=dialog_result,
                  keepOpen=keep_open)

    def show(self) -> int:
        return int(self._transport.call(
            "salamander.ui.dialog.show", dialogId=self.dialog_id
        ).get("result", 0))

    def get(self, control_id: str) -> dict:
        return self._transport.call(
            "salamander.ui.dialog.get", dialogId=self.dialog_id,
            controlId=control_id
        )

    def set(self, control_id: str, value: str) -> None:
        self._transport.call(
            "salamander.ui.dialog.set", dialogId=self.dialog_id,
            controlId=control_id, value=value
        )

    def close(self) -> None:
        self._transport.call(
            "salamander.ui.dialog.destroy", dialogId=self.dialog_id
        )


class _AI:
    def __init__(self, transport: _Transport) -> None:
        self._transport = transport

    def generate(self, prompt: str, context: Optional[dict] = None,
                 provider: Optional[str] = None, runtime: Optional[str] = None,
                 existing_script: Optional[str] = None,
                 feedback: Optional[str] = None) -> dict:
        arguments = {"prompt": prompt}
        if context is not None:
            arguments["context"] = context
        if provider is not None:
            arguments["provider"] = provider
        if runtime is not None:
            arguments["runtime"] = runtime
        if existing_script is not None:
            arguments["existingScript"] = existing_script
        if feedback is not None:
            arguments["feedback"] = feedback
        return self._transport.call("salamander.ai.generate", **arguments)

    def api(self, topic: Optional[str] = None) -> dict:
        arguments = {} if topic is None else {"topic": topic}
        return self._transport.call("salamander.ai.api", **arguments)

    def api_description(self, topic: Optional[str] = None) -> dict:
        return self.api(topic)

    def preview(self, prompt: str, context: Optional[dict] = None,
                provider: Optional[str] = None, runtime: Optional[str] = None,
                existing_script: Optional[str] = None,
                feedback: Optional[str] = None) -> dict:
        arguments = {"prompt": prompt}
        if context is not None:
            arguments["context"] = context
        if provider is not None:
            arguments["provider"] = provider
        if runtime is not None:
            arguments["runtime"] = runtime
        if existing_script is not None:
            arguments["existingScript"] = existing_script
        if feedback is not None:
            arguments["feedback"] = feedback
        return self._transport.call("salamander.ai.preview", **arguments)


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


class _Runtimes:
    def __init__(self, transport: _Transport) -> None:
        self._transport = transport

    def list(self) -> list:
        return self._transport.call("salamander.runtimes.list").get(
            "runtimes", []
        )


class _Salamander:
    def __init__(self, transport: _Transport, command_id: str = "",
                 command_handler: str = "") -> None:
        self.command_id = command_id
        self.command_handler = command_handler
        self.commands = _Commands(transport)
        self.storage = _Storage(transport)
        self.file_operations = _FileOperations(transport)
        self.sides = _Sides(transport)
        self.ui = _UI(transport)
        self.clipboard = _Clipboard(transport)
        self.ai = _AI(transport)
        self.events = _Events(transport)
        self.runtimes = _Runtimes(transport)
        self.left_side = _Side(self.sides, "left")
        self.right_side = _Side(self.sides, "right")
        self.source_side = _Side(self.sides, "source")
        self.target_side = _Side(self.sides, "target")


def main() -> int:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--entry", required=True)
    parser.add_argument("--command-id", default="")
    parser.add_argument("--command-handler", default="")
    parser.add_argument("--one-shot", action="store_true")
    args = parser.parse_args()
    transport = _Transport()
    transport.handshake()
    globals_for_script = {
        "Salamander": _Salamander(
            transport, args.command_id, args.command_handler)
    }
    runpy.run_path(args.entry, init_globals=globals_for_script, run_name="__main__")
    if args.one_shot:
        return 0
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
