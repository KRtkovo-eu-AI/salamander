# Prism Text Viewer Dependencies

The native Prism Text Viewer uses the shared Viewer Frame and relies on:

- **Microsoft.Web.WebView2 1.0.2420.47** — native WebView2 SDK and loader by Microsoft Corporation, distributed under the BSD 3-Clause license (see the package `LICENSE.txt`).
- **Prism.js 1.29.0** — syntax highlighter by Lea Verou and the PrismJS contributors, published at [prismjs.com](https://prismjs.com) under the MIT License.

Salamatrix is the single producer and installer owner of the shared Prism assets. Text Viewer consumes them from `plugins\salamatrix\prism` through Viewer Frame and therefore depends on the Salamatrix plugin.
