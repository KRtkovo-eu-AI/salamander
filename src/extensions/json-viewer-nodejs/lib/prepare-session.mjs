import { copyFile, mkdir, mkdtemp, readFile, rm, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { basename, join } from "node:path";

export function stripBom(text) {
  return text.charCodeAt(0) === 0xfeff ? text.slice(1) : text;
}

export function isValidJson(text) {
  try {
    JSON.parse(text);
    return true;
  } catch {
    return false;
  }
}

export function escapeHtml(value) {
  return String(value)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;");
}

export async function prepareViewerSession({
  jsonText,
  sourcePath,
  packageRoot,
  sessionsRoot = join(tmpdir(), "salamatrix-json-viewer-nodejs"),
}) {
  const vendorRoot = join(packageRoot, "vendor", "pd4d10-json-viewer");
  const viewerRoot = join(packageRoot, "viewer");
  await mkdir(sessionsRoot, { recursive: true });
  const sessionDir = await mkdtemp(join(sessionsRoot, "view-"));
  const title = basename(sourcePath) || "JSON";

  const html = (await readFile(join(viewerRoot, "viewer.html"), "utf8"))
    .replace("<title>JSON Viewer</title>", `<title>${escapeHtml(title)}</title>`);

  await Promise.all([
    writeFile(join(sessionDir, "viewer.html"), html, "utf8"),
    writeFile(join(sessionDir, "data.json"), jsonText, "utf8"),
    copyFile(join(viewerRoot, "bootstrap.js"), join(sessionDir, "bootstrap.js")),
    copyFile(join(viewerRoot, "chrome-shim.js"), join(sessionDir, "chrome-shim.js")),
    copyFile(
      join(vendorRoot, "content-script.js"),
      join(sessionDir, "content-script.js"),
    ),
  ]);

  return { sessionDir, htmlPath: join(sessionDir, "viewer.html") };
}

export async function cleanupOldSessions(
  sessionsRoot = join(tmpdir(), "salamatrix-json-viewer-nodejs"),
  maxAgeMs = 24 * 60 * 60 * 1000,
) {
  try {
    const { readdir, stat } = await import("node:fs/promises");
    const entries = await readdir(sessionsRoot, { withFileTypes: true });
    const cutoff = Date.now() - maxAgeMs;
    await Promise.all(entries.map(async (entry) => {
      if (!entry.isDirectory()) {
        return;
      }
      const fullPath = join(sessionsRoot, entry.name);
      try {
        const info = await stat(fullPath);
        if (info.mtimeMs < cutoff) {
          await rm(fullPath, { recursive: true, force: true });
        }
      } catch {
        // Ignore races with other viewer sessions.
      }
    }));
  } catch {
    // Missing root is fine on first run.
  }
}
