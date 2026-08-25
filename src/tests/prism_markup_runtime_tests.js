// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

"use strict";

const path = require("path");

const prismRoot = path.resolve(
  __dirname,
  "../plugins/shared/webviewviewer/prism/node_modules/prismjs"
);
const Prism = require(prismRoot);
global.self = global;
require(path.resolve(
  __dirname,
  "../plugins/shared/webviewviewer/prism/viewer/prism-patches.js"
));

SalamanderPrism.patchLanguage(Prism, "markup");

const xml = [
  '<PropertyGroup Condition="true">',
  "  <BaseIntermediateOutputPath>$(Temp)</BaseIntermediateOutputPath>",
  '  <Import Project="$(MSBuildProjectExtensionsHigh)"/>',
  "</PropertyGroup>"
].join("\n");

const html = Prism.highlight(xml, Prism.languages.markup, "markup");

function requireToken(fragment, message) {
  if (!html.includes(fragment)) {
    throw new Error(`${message}\n${html}`);
  }
}

requireToken('<span class="token tag"><span class="token punctuation">&lt;</span>PropertyGroup</span>', "XML tag names must stay tag tokens");
requireToken('<span class="token attr-name">Condition</span>', "XML attributes must stay attr-name tokens");
requireToken('<span class="token attr-value">', "XML attribute values must stay attr-value tokens");
requireToken(
  '<span class="token msbuild-property variable">$(Temp)</span>',
  "MSBuild $(property) text must be pale blue like Visual Studio"
);
requireToken(
  '<span class="token msbuild-property variable">$(MSBuildProjectExtensionsHigh)</span>',
  "MSBuild $(property) attributes must be pale blue like Visual Studio"
);

const splitComment = [
  "  <!--",
  "====================================================================",
  "  </Import>",
  "",
  "D:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\MSBuild\\Current\\Imports\\Microsoft.Common.props\\ImportBefore\\Microsoft.NuGet.ImportBefore.props",
  "====================================================================",
  "-->"
].join("\n");

function requireNoTagToken(html, message) {
  if (html.includes('class="token tag"')) {
    throw new Error(`${message}\n${html}`);
  }
}

function requireComment(html, snippet, message) {
  if (!html.includes('class="token comment"') || !html.includes(snippet)) {
    throw new Error(`${message}\n${html}`);
  }
}

const wholeCommentHtml = SalamanderPrism.highlightMarkupChunk(
  Prism,
  splitComment,
  Prism.languages.markup,
  "markup",
  false
);
requireNoTagToken(wholeCommentHtml, "a complete XML comment must not retokenize inner tags");
requireComment(wholeCommentHtml, "&lt;/Import", "a complete XML comment must keep &lt;/Import as comment text");

const startChunk = splitComment.split("\n").slice(0, 3).join("\n");
const startHtml = SalamanderPrism.highlightMarkupChunk(
  Prism,
  startChunk,
  Prism.languages.markup,
  "markup",
  false
);
requireNoTagToken(startHtml, "an XML comment split after </Import> must not paint that tag as live markup");
requireComment(startHtml, "&lt;/Import", "the opening comment chunk must keep </Import> inside a comment token");

const endChunk = splitComment.split("\n").slice(3).join("\n");
const endHtml = SalamanderPrism.highlightMarkupChunk(
  Prism,
  endChunk,
  Prism.languages.markup,
  "markup",
  true
);
requireNoTagToken(endHtml, "the second virtual chunk of an XML comment must stay a comment");
requireComment(endHtml, "-->", "the continuation chunk must keep the comment closer");

const afterClose = SalamanderPrism.highlightMarkupChunk(
  Prism,
  "  </Import>\n-->\n<Import Project=\"x\"/>",
  Prism.languages.markup,
  "markup",
  true
);
requireComment(afterClose, "&lt;/Import", "comment text before --> must stay a comment");
if (!afterClose.includes('class="token tag"') || !afterClose.includes("Import")) {
  throw new Error(`XML after a closed comment must become a live tag again\n${afterClose}`);
}

console.log("Prism XML Visual Studio token mapping tests passed.");
