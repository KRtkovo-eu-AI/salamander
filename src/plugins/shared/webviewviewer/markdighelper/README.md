# Markdig Renderer helper

`MarkdigRenderer` preserves the WebView2 Render Viewer Markdown dialect while
keeping the Salamander process free of the CLR. It is published as a
self-contained NativeAOT executable and therefore does not require an installed
.NET runtime on the target computer.

The process accepts repeated length-prefixed UTF-8 Markdown requests on standard
input. Each response contains a success byte, a 32-bit little-endian UTF-8 byte
length, and either an HTML fragment or an error message. The pipeline matches the
legacy managed viewer: `UseAdvancedExtensions()` with generic attributes removed.
