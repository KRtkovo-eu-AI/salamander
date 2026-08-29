// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

"use strict";

const fs = require("fs");
const path = require("path");
const vm = require("vm");

const prismRoot = path.resolve(
  __dirname,
  "../plugins/shared/webviewviewer/prism/node_modules/prismjs"
);
const viewerRoot = path.resolve(
  __dirname,
  "../plugins/shared/webviewviewer/prism/viewer"
);
const workerSource = fs.readFileSync(path.join(viewerRoot, "prism-worker.js"), "utf8");
let messageHandler = null;
let resolveResponse = null;

const context = {
  console,
  Set,
  Promise,
  fetch(url) {
    if (url !== "../components.json") {
      return Promise.reject(new Error(`Unexpected worker fetch: ${url}`));
    }
    return Promise.resolve({
      ok: true,
      json() {
        return Promise.resolve(
          JSON.parse(fs.readFileSync(path.join(prismRoot, "components.json"), "utf8"))
        );
      }
    });
  },
  importScripts(...urls) {
    for (const url of urls) {
      const name = String(url).split("/").pop().split("?")[0];
      const resolved = String(url).split("?")[0].replace(/^\.\.\//, "");
      const file =
        name === "prism-patches.js" || name === "prism-language-graph.js"
          ? path.join(viewerRoot, name)
          : path.join(prismRoot, resolved);
      vm.runInContext(fs.readFileSync(file, "utf8"), sandbox, { filename: file });
    }
  },
  addEventListener(type, handler) {
    if (type === "message") {
      messageHandler = handler;
    }
  },
  postMessage(message) {
    if (resolveResponse) {
      resolveResponse(message);
    }
  }
};
context.self = context;
const sandbox = vm.createContext(context);
vm.runInContext(workerSource, sandbox, {
  filename: path.join(viewerRoot, "prism-worker.js")
});

if (typeof messageHandler !== "function") {
  throw new Error("Prism worker did not install its message handler.");
}

function highlight(request) {
  return new Promise((resolve, reject) => {
    const timeout = setTimeout(
      () => reject(new Error(`Prism worker did not finish ${request.language} highlighting.`)),
      30000
    );
    resolveResponse = (message) => {
      clearTimeout(timeout);
      resolveResponse = null;
      resolve(message);
    };
    messageHandler({ data: request });
  });
}

async function main() {
  const element = '<device id="42" enabled="true">temperature</device>';
  const xml = `<?xml version="1.0"?><devices>${element.repeat(24000)}</devices>`;
  if (Buffer.byteLength(xml, "utf8") < 1024 * 1024) {
    throw new Error("Large XML fixture did not cross the virtual-viewer threshold.");
  }

  const xmlResult = await highlight({
    type: "highlight",
    generation: 1,
    requestId: 1,
    startLine: 0,
    lineCount: 1,
    language: "markup",
    showWhitespace: false,
    text: xml
  });
  if (!xmlResult.ok) {
    throw new Error(`Large XML highlighting failed: ${xmlResult.error}`);
  }
  for (const tokenClass of ["token tag", "token attr-name", "token attr-value"]) {
    if (!xmlResult.html.includes(`class="${tokenClass}"`)) {
      throw new Error(`Large XML output is missing ${tokenClass}.`);
    }
  }
  const xmlByAlias = await highlight({
    type: "highlight",
    generation: 1,
    requestId: 3,
    startLine: 0,
    lineCount: 1,
    language: "xml",
    showWhitespace: false,
    text: '<?xml version="1.0"?><device id="42">temperature</device>'
  });
  if (!xmlByAlias.ok || !xmlByAlias.html.includes('class="token tag"')) {
    throw new Error(`XML alias highlighting failed: ${xmlByAlias.error}`);
  }

  const batchResult = await highlight({
    type: "highlight",
    generation: 1,
    requestId: 2,
    startLine: 0,
    lineCount: 1,
    language: "batch",
    showWhitespace: false,
    text: 'copy /Y "%HARDVIEW_LIB%\\hostpolicy.dll" "%EXT_DST%\\lib\\"'
  });
  if (
    !batchResult.ok ||
    !batchResult.html.includes(
      '<span class="token string">"<span class="token variable">%HARDVIEW_LIB%</span>\\hostpolicy.dll"</span>'
    )
  ) {
    throw new Error("Worker did not apply the batch quoted-path grammar patch.");
  }

  const powershell = 'Write-Host "hello"; if ($true) { Get-ChildItem -Path C:\\temp }'
  const powershellResult = await highlight({
    type: "highlight",
    generation: 1,
    requestId: 4,
    startLine: 0,
    lineCount: 1,
    language: "powershell",
    showWhitespace: false,
    text: powershell
  });
  if (!powershellResult.ok || !powershellResult.html.includes("token")) {
    throw new Error(`PowerShell highlighting failed: ${powershellResult.error}`);
  }

  const cmdResult = await highlight({
    type: "highlight",
    generation: 1,
    requestId: 5,
    startLine: 0,
    lineCount: 1,
    language: "cmd",
    showWhitespace: false,
    text: 'copy /Y "%HARDVIEW_LIB%\\hostpolicy.dll" "%EXT_DST%\\lib\\"'
  });
  if (
    !cmdResult.ok ||
    !cmdResult.html.includes(
      '<span class="token string">"<span class="token variable">%HARDVIEW_LIB%</span>\\hostpolicy.dll"</span>'
    )
  ) {
    throw new Error(`CMD alias highlighting failed: ${cmdResult.error}`);
  }

  const configXml = '<?xml version="1.0"?><configuration><appSettings/></configuration>';
  for (const language of ["config", "targets", "xml"]) {
    const result = await highlight({
      type: "highlight",
      generation: 1,
      requestId: 6,
      startLine: 0,
      lineCount: 1,
      language,
      showWhitespace: false,
      text: configXml
    });
    if (!result.ok || !result.html.includes('class="token tag"')) {
      throw new Error(`${language} must highlight as XML markup: ${result.error}`);
    }
  }

  const splitCommentStart = [
    "  <!--",
    "====================================================================",
    "  </Import>"
  ].join("\n");
  const splitStart = await highlight({
    type: "highlight",
    generation: 1,
    requestId: 7,
    startLine: 0,
    lineCount: 3,
    language: "xml",
    showWhitespace: false,
    insideMarkupComment: false,
    text: splitCommentStart
  });
  if (!splitStart.ok || splitStart.html.includes('class="token tag"')) {
    throw new Error(
      `XML comments split across virtual chunks must not retokenize inner tags: ${splitStart.error || splitStart.html}`
    );
  }

  const splitCommentEnd = [
    "",
    "D:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\MSBuild\\Current\\Imports\\Microsoft.Common.props\\ImportBefore\\Microsoft.NuGet.ImportBefore.props",
    "====================================================================",
    "-->"
  ].join("\n");
  const splitEnd = await highlight({
    type: "highlight",
    generation: 1,
    requestId: 8,
    startLine: 3,
    lineCount: 4,
    language: "xml",
    showWhitespace: false,
    insideMarkupComment: true,
    text: splitCommentEnd
  });
  if (!splitEnd.ok || !splitEnd.html.includes('class="token comment"') || splitEnd.html.includes('class="token tag"')) {
    throw new Error(
      `XML comment continuation chunks must stay comments: ${splitEnd.error || splitEnd.html}`
    );
  }

  const whitespaceResult = await highlight({
    type: "highlight",
    generation: 1,
    requestId: 9,
    startLine: 0,
    lineCount: 2,
    language: "plaintext",
    showWhitespace: true,
    text: "  \ttext\r\n"
  });
  if (
    !whitespaceResult.ok ||
    !whitespaceResult.html.includes('class="token space"') ||
    !whitespaceResult.html.includes('class="token tab"') ||
    !whitespaceResult.html.includes('<span class="token crlf">\r\n</span>')
  ) {
    throw new Error(`Whitespace highlighting failed: ${whitespaceResult.error || whitespaceResult.html}`);
  }

  console.log("Prism worker runtime tests passed.");
}

main().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
