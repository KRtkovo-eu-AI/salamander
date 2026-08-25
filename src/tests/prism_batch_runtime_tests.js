// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

"use strict";

const path = require("path");

const prismRoot = path.resolve(
  __dirname,
  "../plugins/shared/webviewviewer/prism/node_modules/prismjs"
);
const Prism = require(prismRoot);
require(path.join(prismRoot, "components/prism-batch"));
global.self = global;
require(path.resolve(
  __dirname,
  "../plugins/shared/webviewviewer/prism/viewer/prism-patches.js"
));

function tokenText(value) {
  if (typeof value === "string") {
    return value;
  }
  if (Array.isArray(value)) {
    return value.map(tokenText).join("");
  }
  return tokenText(value.content);
}

function collectTokens(value, type, output = []) {
  if (Array.isArray(value)) {
    value.forEach((item) => collectTokens(item, type, output));
  } else if (value && typeof value !== "string") {
    if (value.type === type) {
      output.push(value);
    }
    collectTokens(value.content, type, output);
  }
  return output;
}

SalamanderPrism.patchLanguage(Prism, "batch");
const lines = [
  'copy /Y "%EXT_SRC%\\main.ps1" "%EXT_DST%\\"',
  'copy /Y "%HARDVIEW_LIB%\\Lib\\HardwareMonitor.dll" "%EXT_DST%\\lib\\"',
  'copy /Y "%HARDVIEW_LIB%\\System.Management.dll" "%EXT_DST%\\lib\\"',
  'copy /Y "%HARDVIEW_LIB%\\vcruntime140_1.dll" "%EXT_DST%\\lib\\"'
];
const source = lines.join("\r\n");
const html = Prism.highlight(source, Prism.languages.batch, "batch");

for (const variable of ["%EXT_SRC%", "%EXT_DST%", "%HARDVIEW_LIB%"]) {
  const token = `<span class="token variable">${variable}</span>`;
  if (!html.includes(token)) {
    throw new Error(`Batch variable was not tokenized inside a quoted path: ${variable}`);
  }
}

const fileStrings = collectTokens(Prism.tokenize(source, Prism.languages.batch), "string").map(
  tokenText
);
const expectedFileStrings = Array.from(source.matchAll(/"[^"\r\n]*"/g), (match) => match[0]);
if (JSON.stringify(fileStrings) !== JSON.stringify(expectedFileStrings)) {
  throw new Error(
    `Consecutive copy lines with Windows path endings were merged:\n${JSON.stringify(fileStrings)}`
  );
}

for (const line of lines) {
  const tokens = Prism.tokenize(line, Prism.languages.batch);
  const expectedArguments = Array.from(line.matchAll(/"[^"\r\n]*"/g), (match) => match[0]);
  const stringTokens = collectTokens(tokens, "string");
  const actualArguments = stringTokens.map(tokenText);
  if (JSON.stringify(actualArguments) !== JSON.stringify(expectedArguments)) {
    throw new Error(
      `Quoted batch paths were not preserved as independent string tokens:\n${line}\n${JSON.stringify(actualArguments)}`
    );
  }
  for (const stringToken of stringTokens) {
    const text = tokenText(stringToken);
    const expectedVariables = Array.from(text.matchAll(/%[A-Z_]+%/g), (match) => match[0]);
    const actualVariables = collectTokens(stringToken.content, "variable").map(tokenText);
    if (JSON.stringify(actualVariables) !== JSON.stringify(expectedVariables)) {
      throw new Error(`Batch variable escaped its quoted path token: ${text}`);
    }
  }
}

console.log("Prism batch runtime tests passed.");
