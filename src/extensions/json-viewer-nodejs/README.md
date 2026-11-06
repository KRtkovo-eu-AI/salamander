<!--
| Date | Summary | Reason | Ticket | Model |
| 2026-11-06 | Add locales for all supported extension languages. | Match other Salamatrix extensions (de/es/fr/hu/it/nl/ro/ru/sk/zh-CN). |  | Composer |
| 2026-11-06 | Make package icon curly braces more recognizable. | Keep green tile; use bold typographic "{ }" paths. |  | Composer |
| 2026-11-06 | Note Find bar is injected on Ctrl+F via ExecuteScript. | Staging misses no longer leave document Find broken. |  | Composer |
| 2026-11-06 | Document Prism Sharp Find bar for documents. | JSON Viewer uses the same Find chrome as Prism Text Viewer. |  | Composer |
| 2026-11-06 | Document native WebView2 Find for documents. | Replace the custom find bar with Edge's built-in Find UI. |  | Composer |
| 2026-11-06 | Document host-side placement/title and in-page Find. | Chrome and Find must work even when the worker omits options. |  | Composer |
| 2026-11-06 | Rename to JSON Viewer (nodeJS) and document window chrome. | Match the product name and viewer placement/title/icon behavior. |  | Composer |
| 2026-11-05 | Document the Salamatrix JSON Web Viewer extension. | Lightweight WebView2 alternative to JSON Viewer .NET. |  | Composer |
-->

# JSON Viewer (nodeJS)

Salamatrix extension that opens `*.json` files in the shared WebView2 Viewer
Frame and renders them with the
[pd4d10/json-viewer](https://github.com/pd4d10/json-viewer) port of Firefox's
JSON Viewer (collapsible tree, syntax colors, Raw Data tab).

## Why

[JSON Viewer .NET](../../plugins/jsonviewer/) remains the full-featured WinForms
tree viewer. This package is a lighter alternative: one Node.js Salamatrix
extension, no managed DLL host, reuse of the existing WebView2 stack already
shipped with Salamatrix / Prism Text Viewer / WebView2 Render Viewer.

## Requirements

- Salamatrix Framework (shared Viewer Frame)
- JavaScript Runtime (`javascriptruntime`)
- Microsoft Edge WebView2 Runtime

## How it works

1. The viewer handler reads the selected JSON file as UTF-8.
2. Valid JSON is copied into a temporary session folder together with the
   vendored `content-script.js` renderer and a small HTML bootstrap page.
3. `Salamander.ui.viewer(htmlPath, { renderer: "document", title, documentName, ...window })`
   opens that page through the shared document renderer using the parent
   viewer placement from `Salamander.invocation.window`, the package SVG icon,
   and the title `\<file\> - JSON Viewer`.
4. Invalid JSON falls back to the Prism text renderer so the user still sees
   the file contents.
5. Ctrl+F / Edit → Find open the same Prism Sharp Find bar used by Prism Text
   Viewer (tree and Raw Data).
6. Salamatrix also remembers the ViewFile rectangle and defaults the title to
   `JSON Viewer` on the host, so chrome stays correct even if an older
   JavaScript worker drops `title` / geometry from `ui.viewer`.

Rebuild/reinstall Salamatrix + JavaScript Runtime together with this package
(extension-only updates are not enough for title, icon, size, or Find). Then
restart Open Samandarin so viewer masks are re-registered under
Configuration → Viewers.
