# Advanced combined example: TabuleSprZel

When available, the installed package at `C:\Program Files\Open Salamander Samandarin\extensions\TabuleSprZel\` is a useful real-world reference for a single extension that combines several roles. Treat it as read-only inspiration and verify its behavior against the current checkout's docs, parser, runtime contract, and tests; an installed package can be newer or older than repository sources.

## What it demonstrates

Its `extension.json` uses schema 2 and combines:

- a toolbar command that navigates to `salamatrix:TabuleSprZel!departures`, plus configuration and favorite-station commands; the entry point also registers data-dependent favorite commands through `Salamander.commands.register()` during startup and closes that registration window on host shutdown;
- a `viewers[]` association for `*.departure` detail records;
- a `fileSystems[]` provider with hierarchical roots, many typed columns, per-item action filtering, and separate list/open handlers;
- Czech and English locales, package light/dark icons, explicit runtime capabilities, storage, and host lifecycle events.

The `main.mjs` coordinates listing and actions through one handler dispatcher. It combines short-lived per-extension `Salamander.storage` values with a bounded disk cache, stable item IDs, asynchronous network refresh, timeout/cancellation, periodic refresh teardown, and a separate richer details/configuration UI. Related HTML, CSS, and JS files show how the package splits larger UI surfaces and localizes their text.

The package's security declaration discloses network access, external processes, script execution and active web content. It fetches external data and uses Node child processes and local HTTP endpoints to manage custom WebView windows. This is a deliberate advanced architecture with additional dependencies and security obligations, not the baseline pattern for ordinary extensions. Prefer `Salamatrix.UI` and the shared `ui.viewer` surface for ordinary dialogs and document viewing; only reproduce a custom process/window bridge when the requested behavior needs it and the current host contract cannot provide it.

## Lessons to apply selectively

- Keep each contribution aligned: command paths must match provider IDs; manifest handlers must match entry-point dispatch; action IDs/filters must match returned item IDs.
- Keep listing responsive: publish snapshots, avoid allowing an older asynchronous request to overwrite a newer panel request, and use cached data deliberately when refresh fails.
- Persist only the state that must survive invocations. Use extension-scoped storage for ordinary preferences; add local caches only when they solve a real data-volume or latency problem.
- Stop timers and background work when leaving the panel or when the host shuts down. Bound fetch duration, response sizes, retries and temporary data.
- Sanitize and constrain remote HTML, URLs, and resources before rendering them in active web content. Do not trust the remote page or interpolate raw values into HTML.
- Include locale strings for every user-facing role and keep locale dictionaries in sync.
- Treat custom windows, child processes and local HTTP bridges as independent lifecycles: clean temporary state, validate the IPC token/request, handle startup failure, and avoid orphaned processes.

Do not transplant the package wholesale into a simpler extension. Its value is the integration example and the explicit tradeoffs, not a required architecture.

