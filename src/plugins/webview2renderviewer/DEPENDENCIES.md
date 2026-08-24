# WebView2 Render Viewer Dependencies

The native WebView2 Render Viewer uses the shared Viewer Frame and relies on:

- **Microsoft.Web.WebView2 1.0.2420.47** — native WebView2 SDK and loader by Microsoft Corporation, distributed under the BSD 3-Clause license (see the package `LICENSE.txt`).
- **Markdig 0.36.2** — Markdown processor by Alexandre Mutel and contributors, released under the BSD 2-Clause license. It runs in the NativeAOT `MarkdigRenderer.exe` helper staged by Salamatrix.

The viewer does not load the .NET runtime into Salamander.
