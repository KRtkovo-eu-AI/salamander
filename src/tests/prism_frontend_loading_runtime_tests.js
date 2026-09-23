// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

"use strict";

// Execute the real fallback-loader setup without a WebView or network. The fake
// autoloader records its configuration and completes requests explicitly.
const fs = require("fs");
const path = require("path");
const vm = require("vm");
const { execFileSync } = require("child_process");
const root = path.resolve(__dirname, "../..");
const relative = "src/plugins/shared/webviewviewer/prism/viewer/virtual-viewer.js";
const revision = process.argv[2] === "--baseline-ref" ? process.argv[3] : null;
const source = revision
  ? execFileSync("git", ["show", `${revision}:${relative}`], { cwd: root, encoding: "utf8", timeout: 10000 })
  : fs.readFileSync(path.join(root, relative), "utf8");

function extractFunction(name) {
  const start = source.indexOf(`  function ${name}(`);
  if (start < 0) throw new Error(`Production function ${name} was not found`);
  let end = source.indexOf("{", start) + 1;
  let depth = 1;
  while (depth && end < source.length) {
    depth += (source[end] === "{") - (source[end] === "}");
    end += 1;
  }
  if (depth) throw new Error(`Production function ${name} did not close`);
  return source.slice(start, end);
}

const aliasStart = source.indexOf("  const LANGUAGE_ALIASES = {");
const aliasEnd = source.indexOf("\n  };", aliasStart) + "\n  };".length;
if (aliasStart < 0 || aliasEnd <= aliasStart) throw new Error("Production language aliases were not found");
const production = source.slice(aliasStart, aliasEnd) +
  "\nlet languageLoaders = Object.create(null);\n" +
  extractFunction("configureAutoloader") + "\n" + extractFunction("ensureLanguage");
let checks = 0;
function check(condition, message) {
  checks += 1;
  if (!condition) throw new Error(message);
}

async function testBase(baseURI, expectedPath) {
  const requests = [];
  const autoloader = {
    loadLanguages(languages, success, failure) {
      requests.push({ languages: Array.from(languages), success, failure });
    }
  };
  const context = { URL, document: { baseURI }, Prism: { languages: { javascript: {} }, plugins: { autoloader } } };
  context.window = context;
  vm.createContext(context);
  vm.runInContext(production, context, { timeout: 1000 });
  context.configureAutoloader();
  check(autoloader.languages_path === expectedPath, `Fallback lexer path must follow the page URL: ${baseURI}`);
  check(await context.ensureLanguage("plain") === "plain", "plain text resolves without a lexer");
  check(await context.ensureLanguage("javascript") === "javascript", "an existing grammar resolves immediately");
  check(requests.length === 0, "plain text and an existing grammar do not request scripts");
  const xml = context.ensureLanguage("xml");
  check(context.ensureLanguage("markup") === xml, "aliases share the same pending language request");
  check(requests.length === 1 && requests[0].languages.join() === "markup", "XML fallback loads the canonical markup lexer");
  requests[0].success();
  check(await xml === "markup", "loaded fallback language resolves");
  check(context.ensureLanguage("xml") === xml && requests.length === 1, "completed language requests are cached");
  const failed = context.ensureLanguage("ruby");
  requests[1].failure();
  check(await failed === "ruby", "failed syntax loading resolves to allow plain-text fallback");
  delete context.Prism.plugins.autoloader;
  context.configureAutoloader();
  check(await context.ensureLanguage("python") === "python", "missing autoloader does not block text display");
}

async function main() {
  await testBase("https://prism.example/viewer/virtual-viewer.html?g=77", "https://prism.example/components/");
  await testBase("https://test-host.invalid/nested/viewer/index.html?g=9", "https://test-host.invalid/nested/components/");
  console.log(`${checks} Prism frontend loading checks passed.`);
}

main().catch(error => { console.error(error); process.exitCode = 1; });
