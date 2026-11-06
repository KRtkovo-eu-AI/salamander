import { readFile } from "node:fs/promises";
import { basename, dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import {
  cleanupOldSessions,
  isValidJson,
  prepareViewerSession,
  stripBom,
} from "./lib/prepare-session.mjs";

const packageRoot = dirname(fileURLToPath(import.meta.url));
const handler = Salamander.commandHandler || Salamander.command_handler;
const VIEWER_TITLE = "JSON Viewer";

async function loadStrings() {
  const locale = String(Salamander.locale || Salamander.uiLocale || "en");
  const candidates = [locale, locale.split("-")[0], "en"];
  for (const candidate of candidates) {
    if (!candidate) {
      continue;
    }
    try {
      const raw = await readFile(join(packageRoot, "locales", `${candidate}.json`), "utf8");
      const parsed = JSON.parse(raw);
      return parsed.strings || {};
    } catch {
      // Try the next candidate.
    }
  }
  return {};
}

function viewerWindowOptions() {
  const window = Salamander.invocation?.window;
  if (!window || typeof window !== "object") {
    return {};
  }
  const options = {};
  for (const key of ["left", "top", "width", "height", "showCmd"]) {
    if (window[key] != null && Number.isFinite(Number(window[key]))) {
      options[key] = Number(window[key]);
    }
  }
  if (window.alwaysOnTop != null) {
    options.alwaysOnTop = Boolean(window.alwaysOnTop);
  }
  return options;
}

if (handler === "viewJson") {
  const path = String(Salamander.invocation?.path || "");
  const strings = await loadStrings();
  const windowOptions = viewerWindowOptions();
  try {
    if (!path) {
      throw new Error(strings.noPath || "No file path was provided to JSON Viewer (nodeJS).");
    }

    const raw = stripBom(await readFile(path, "utf8"));
    const documentName = basename(path) || "JSON";
    if (!isValidJson(raw)) {
      // Invalid or JSON-with-comments: keep a readable fallback in Prism.
      const opened = await Salamander.ui.viewer(path, {
        renderer: "prism",
        title: VIEWER_TITLE,
        documentName,
        ...windowOptions,
      });
      if (!opened) {
        await Salamander.ui.messageBox(
          strings.invalidFallback
            || "The file is not valid JSON and the Prism text viewer could not be opened.",
          "JSON Viewer (nodeJS)",
          "OK",
          "Warning",
        );
      }
    } else {
      await cleanupOldSessions();
      const { htmlPath } = await prepareViewerSession({
        jsonText: raw,
        sourcePath: path,
        packageRoot,
      });
      const opened = await Salamander.ui.viewer(htmlPath, {
        renderer: "document",
        title: VIEWER_TITLE,
        documentName,
        ...windowOptions,
      });
      if (!opened) {
        await Salamander.ui.messageBox(
          strings.viewerOpenFailed
            || ("The WebView2 document viewer could not be opened.\n"
              + "Install Salamatrix (shared Viewer Frame) and ensure WebView2 Runtime is available."),
          "JSON Viewer (nodeJS)",
          "OK",
          "Error",
        );
      }
    }
  } catch (error) {
    await Salamander.ui.messageBox(
      String(error?.stack || error),
      "JSON Viewer (nodeJS)",
      "OK",
      "Error",
    );
  }
}
