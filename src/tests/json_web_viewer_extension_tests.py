#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""
| Date | Summary | Reason | Ticket | Model |
| 2026-11-06 | Expect locales for every supported extension language. | JSON Viewer must ship the same locale set as other packages. |  | Composer |
| 2026-11-06 | Expect the Prism Sharp Find bar for document viewers. | JSON Viewer must reuse the same Find chrome as Prism Text. |  | Composer |
| 2026-11-06 | Expect native WebView2 Find instead of a custom bar. | Users want Edge's built-in Find UI for document viewers. |  | Composer |
| 2026-11-06 | Cover host chrome/Find contracts and the json-viewer-nodejs path. | Title, size, icon, and Ctrl+F must not regress after the rename. |  | Composer |
| 2026-11-06 | Expect the JSON Viewer (nodeJS) display name. | The extension was renamed from JSON Web Viewer. |  | Composer |
| 2026-11-05 | Use file:// URLs when importing the session helper on Windows. | Node ESM rejects bare drive-letter paths. |  | Composer |
| 2026-11-05 | Add source-contract checks for JSON Web Viewer. | Guard the Salamatrix WebView2 JSON extension package. |  | Composer |
"""

from __future__ import annotations

import json
import subprocess
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
EXTENSION_ROOT = REPO_ROOT / "src" / "extensions" / "json-viewer-nodejs"
VENDOR_ROOT = EXTENSION_ROOT / "vendor" / "pd4d10-json-viewer"
PACKAGES_CPP = REPO_ROOT / "src" / "plugins" / "salamatrix" / "salamatrix_packages.cpp"
NATIVE_VIEWER_CPP = (
    REPO_ROOT / "src" / "plugins" / "shared" / "webviewviewer" / "native_viewer.cpp"
)
JS_WORKER = (
    REPO_ROOT / "src" / "plugins" / "javascriptruntime" / "runtime" / "salamatrix_worker.mjs"
)


class JsonWebViewerExtensionTests(unittest.TestCase):
    def test_manifest_declares_json_viewer(self) -> None:
        manifest = json.loads((EXTENSION_ROOT / "extension.json").read_text(encoding="utf-8"))
        self.assertEqual(manifest["schema"], 2)
        self.assertEqual(manifest["id"], "JsonViewerNodeJS")
        self.assertEqual(manifest["name"], "JSON Viewer (nodeJS)")
        self.assertEqual(manifest["runtime"], "JavaScript.Node")
        self.assertEqual(manifest["security"]["activeWebContent"], "yes")
        self.assertIn("ui.dialogs", manifest["capabilities"])
        viewers = manifest["viewers"]
        self.assertEqual(len(viewers), 1)
        self.assertEqual(viewers[0]["name"], "JSON Viewer (nodeJS)")
        self.assertEqual(viewers[0]["handler"], "viewJson")
        self.assertIn("*.json", viewers[0]["patterns"])
        expected_locales = {
            "en",
            "cs",
            "de",
            "es",
            "fr",
            "hu",
            "it",
            "nl",
            "ro",
            "ru",
            "sk",
            "zh-CN",
        }
        self.assertEqual(set(manifest["locales"]), expected_locales)

    def test_locale_files_cover_supported_languages(self) -> None:
        required_string_keys = {"invalidFallback", "viewerOpenFailed", "noPath"}
        expected_locales = (
            "en",
            "cs",
            "de",
            "es",
            "fr",
            "hu",
            "it",
            "nl",
            "ro",
            "ru",
            "sk",
            "zh-CN",
        )
        for lang in expected_locales:
            path = EXTENSION_ROOT / "locales" / f"{lang}.json"
            self.assertTrue(path.is_file(), f"missing locale {path}")
            data = json.loads(path.read_text(encoding="utf-8"))
            self.assertIn("name", data)
            self.assertIn("description", data)
            self.assertIn("viewers", data)
            self.assertIn("JSON Viewer (nodeJS)", data["viewers"])
            self.assertTrue(data["description"].strip())
            strings = data.get("strings") or {}
            self.assertEqual(set(strings), required_string_keys)
            for key in required_string_keys:
                self.assertTrue(str(strings[key]).strip(), f"{lang}.{key} empty")

    def test_vendor_bundle_is_present(self) -> None:
        for name in (
            "content-script.js",
            "content-script.js.LICENSE.txt",
            "LICENSE",
            "NOTICE.md",
        ):
            path = VENDOR_ROOT / name
            self.assertTrue(path.is_file(), f"missing {path}")
        self.assertGreater((VENDOR_ROOT / "content-script.js").stat().st_size, 100_000)

    def test_main_passes_title_document_name_and_window(self) -> None:
        main = (EXTENSION_ROOT / "main.mjs").read_text(encoding="utf-8")
        self.assertIn('VIEWER_TITLE = "JSON Viewer"', main)
        self.assertIn("documentName", main)
        self.assertIn("viewerWindowOptions", main)
        self.assertIn("renderer: \"document\"", main)

    def test_bootstrap_does_not_inject_custom_find_ui(self) -> None:
        bootstrap = (EXTENSION_ROOT / "viewer" / "bootstrap.js").read_text(encoding="utf-8")
        self.assertNotIn("__salamanderFindBar", bootstrap)
        self.assertNotIn("salamander-find-button", bootstrap)
        self.assertNotIn("window.find", bootstrap)

    def test_js_worker_forwards_viewer_chrome_options(self) -> None:
        worker = JS_WORKER.read_text(encoding="utf-8")
        self.assertIn("payload.title", worker)
        self.assertIn("payload.documentName", worker)
        self.assertIn('["left", "top", "width", "height", "showCmd"]', worker)

    def test_host_applies_pending_viewfile_placement_and_default_title(self) -> None:
        source = PACKAGES_CPP.read_text(encoding="utf-8", errors="replace")
        self.assertIn("HasPendingViewerPlacement", source)
        self.assertIn("PendingViewerDocumentName", source)
        self.assertIn('title = "JSON Viewer"', source)
        self.assertIn("CreateSVGIcon(preferredPath, 32)", source)
        self.assertIn("request.documentName", source)

    def test_native_viewer_supports_document_find_and_icons(self) -> None:
        source = NATIVE_VIEWER_CPP.read_text(encoding="utf-8", errors="replace")
        self.assertIn("LoadDocumentFindBarScript", source)
        self.assertIn("document-find-bar.js", source)
        self.assertIn("document_find_bar_script.inc", source)
        self.assertIn("__salamanderOpenFind", source)
        self.assertIn("InjectDocumentFindBar", source)
        self.assertIn("Re-executing document-find-bar.js", source)
        self.assertIn("salamander-close", source)
        self.assertRegex(source, r"NativeViewerKind::RenderDocument")
        self.assertTrue(
            (REPO_ROOT / "src" / "plugins" / "shared" / "webviewviewer" / "document_find_bar_script.inc").is_file()
        )
        self.assertIn("openFind", source)
        self.assertNotIn("Input.dispatchKeyEvent", source)
        self.assertNotIn("__salamanderFindBar", source)
        self.assertIn("CreateWin32MenuBar", source)
        self.assertIn("WM_SETICON", source)
        self.assertIn("CopyIcon", source)
        self.assertIn("IDM_NV_FIND", source)
        find_bar = (
            REPO_ROOT
            / "src"
            / "plugins"
            / "shared"
            / "webviewviewer"
            / "prism"
            / "viewer"
            / "document-find-bar.js"
        )
        self.assertTrue(find_bar.is_file())
        find_src = find_bar.read_text(encoding="utf-8")
        self.assertIn("viewer-find-bar", find_src)
        self.assertIn("__salamanderOpenFind", find_src)
        self.assertIn("find-case", find_src)
        self.assertIn("find-word", find_src)
        self.assertIn("2147483647", find_src)
        self.assertIn("restoreFindInputFocus", find_src)
        self.assertIn("viewer-find-match", find_src)
        self.assertIn("CSS.highlights", find_src)
        self.assertIn("paintCurrentMatch", find_src)
        self.assertIn("collectMatchRanges", find_src)
        self.assertIn("__salamanderFindBarRuntime", find_src)
        self.assertIn("scrollIntoView", find_src)
        self.assertIn("scrollParentsFor", find_src)
        self.assertIn("top:58px", find_src)
        self.assertIn("salamander-close", find_src)
        self.assertIn("__salamanderHideFind", find_src)

    def test_prepare_session_copies_renderer_assets(self) -> None:
        sample = {"hello": "world", "items": [1, 2, {"nested": True}]}
        with tempfile.TemporaryDirectory() as temp:
            source = Path(temp) / "sample.json"
            source.write_text(json.dumps(sample), encoding="utf-8")
            sessions = Path(temp) / "sessions"
            helper_uri = (EXTENSION_ROOT / "lib" / "prepare-session.mjs").resolve().as_uri()
            script = f"""
import {{ join }} from "node:path";
import {{ prepareViewerSession }} from {json.dumps(helper_uri)};
import {{ readFile }} from "node:fs/promises";
const result = await prepareViewerSession({{
  jsonText: await readFile({json.dumps(str(source))}, "utf8"),
  sourcePath: {json.dumps(str(source))},
  packageRoot: {json.dumps(str(EXTENSION_ROOT))},
  sessionsRoot: {json.dumps(str(sessions))},
}});
const html = await readFile(result.htmlPath, "utf8");
const data = await readFile(join(result.sessionDir, "data.json"), "utf8");
const bootstrap = await readFile(join(result.sessionDir, "bootstrap.js"), "utf8");
console.log(JSON.stringify({{
  titleOk: html.includes("<title>sample.json</title>"),
  hasBootstrap: html.includes("bootstrap.js"),
  data,
  hasContentScript: (await readFile(join(result.sessionDir, "content-script.js"), "utf8")).length > 1000,
  hasShim: (await readFile(join(result.sessionDir, "chrome-shim.js"), "utf8")).includes("chrome.storage"),
  hasCustomFind: bootstrap.includes("__salamanderFindBar"),
}}));
"""
            completed = subprocess.run(
                ["node", "--input-type=module", "-e", script],
                check=False,
                capture_output=True,
                text=True,
                cwd=str(EXTENSION_ROOT),
            )
            self.assertEqual(
                completed.returncode,
                0,
                completed.stderr or completed.stdout,
            )
            payload = json.loads(completed.stdout.strip().splitlines()[-1])
            self.assertTrue(payload["titleOk"])
            self.assertTrue(payload["hasBootstrap"])
            self.assertTrue(payload["hasContentScript"])
            self.assertTrue(payload["hasShim"])
            self.assertFalse(payload["hasCustomFind"])
            self.assertEqual(json.loads(payload["data"]), sample)


if __name__ == "__main__":
    unittest.main()
