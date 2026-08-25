// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

"use strict";

const path = require("path");

const prismRoot = path.resolve(
  __dirname,
  "../plugins/shared/webviewviewer/prism/node_modules/prismjs"
);
const Prism = require(prismRoot);
require(path.join(prismRoot, "components/prism-clike"));
require(path.join(prismRoot, "components/prism-csharp"));
global.self = global;
require(path.resolve(
  __dirname,
  "../plugins/shared/webviewviewer/prism/viewer/prism-patches.js"
));

const grammarBeforePatch = Prism.languages.csharp;
SalamanderPrism.patchLanguage(Prism, "csharp");
if (grammarBeforePatch !== Prism.languages.csharp) {
  throw new Error("C# grammar patches must mutate the existing Prism language object");
}

const vsZeroLine = "IntPtr parentHandle = IntPtr.Zero;";
const vsZeroHtml = Prism.highlight(vsZeroLine, Prism.languages.csharp, "csharp");
if (
  vsZeroHtml !==
  '<span class="token class-name">IntPtr</span> <span class="token variable">parentHandle</span> <span class="token operator">=</span> <span class="token maybe-class-name class-name">IntPtr</span><span class="token punctuation">.</span><span class="token maybe-member">Zero</span><span class="token punctuation">;</span>'
) {
  throw new Error(`Visual Studio IntPtr.Zero coloring mismatch:\n${vsZeroHtml}`);
}

const vsAliasLine = "using Timer = System.Threading.Timer;";
const vsAliasHtml = Prism.highlight(vsAliasLine, Prism.languages.csharp, "csharp");
if (!vsAliasHtml.includes('<span class="token maybe-namespace namespace">System</span>')) {
  throw new Error(`using-alias namespace prefix System must not be a type:\n${vsAliasHtml}`);
}
if (!vsAliasHtml.includes('<span class="token maybe-namespace namespace">Threading</span>')) {
  throw new Error(`using-alias namespace prefix Threading must not be a type:\n${vsAliasHtml}`);
}
if ((vsAliasHtml.match(/class-name">Timer</g) || []).length < 1) {
  throw new Error(`using-alias Timer must remain a type:\n${vsAliasHtml}`);
}
if (vsAliasHtml.includes('maybe-class-name class-name">System<') || vsAliasHtml.includes('maybe-class-name class-name">Threading<')) {
  throw new Error(`System.Threading must not use the type color:\n${vsAliasHtml}`);
}

const source = [
  "    [STAThread]",
  "    public static int Dispatch(string? argument)",
  "    {",
  "        private static readonly object SyncRoot = new();",
  "        IntPtr parent = IntPtr.Zero;",
  "        var handler = new HttpClientHandler",
  "        {",
  "            AllowAutoRedirect = true,",
  "            Timeout = TimeSpan.FromSeconds(15),",
  "        };",
  "        var n = parts.Length;",
  "        NativeStrings.Get(NativeStringId.UnexpectedException);",
  "        ViewerHost.Launch(parent, payload, asynchronous: true);",
  "        try",
  "        {",
  "            if (parent == IntPtr.Zero)",
  "            {",
  "                return command switch",
  "                {",
  "                    default => 1,",
  "                };",
  "            }",
  "            else",
  "            {",
  "                return 1;",
  "            }",
  "        }",
  "        catch (Exception ex)",
  "        {",
  "            throw;",
  "        }",
  "    }"
].join("\n");

const html = Prism.highlight(source, Prism.languages.csharp, "csharp");

function requireToken(fragment, message) {
  if (!html.includes(fragment)) {
    throw new Error(`${message}\n${html}`);
  }
}

requireToken('<span class="token function">Dispatch</span>', "C# methods must stay function tokens");
requireToken('<span class="token class-name">IntPtr</span>', "C# type names in declarations must stay class-name tokens");
requireToken(
  '<span class="token maybe-class-name class-name">IntPtr</span><span class="token punctuation">.</span><span class="token maybe-member">Zero</span>',
  "IntPtr.Zero must keep Zero in the default foreground like Visual Studio"
);
requireToken(
  '<span class="token variable">parts</span><span class="token punctuation">.</span><span class="token maybe-member">Length</span>',
  "members after a dot such as parts.Length must keep the default foreground"
);
requireToken(
  '<span class="token maybe-class-name class-name">NativeStringId</span><span class="token punctuation">.</span><span class="token maybe-member">UnexpectedException</span>',
  "enum members after a type must keep the default foreground"
);
requireToken(
  '<span class="token maybe-class-name class-name">ViewerHost</span><span class="token punctuation">.</span><span class="token function">Launch</span>',
  "PascalCase receivers such as ViewerHost.Launch must keep yellow methods"
);
requireToken(
  '<span class="token variable">AllowAutoRedirect</span>',
  "object-initializer names stay pale blue like other declared identifiers"
);
requireToken(
  '<span class="token variable">Timeout</span>',
  "object-initializer names stay pale blue like other declared identifiers"
);
requireToken('<span class="token boolean">true</span>', "C# true/false must remain boolean tokens");
requireToken(
  '<span class="token named-parameter punctuation">asynchronous</span>',
  "C# named arguments must keep the named-parameter token"
);
requireToken('<span class="token number">1</span>', "C# numeric literals must remain number tokens");
requireToken('<span class="token keyword">public</span>', "declaration keywords must stay blue keyword tokens");
for (const word of ["argument", "parent", "payload", "command", "ex", "handler", "SyncRoot"]) {
  requireToken(
    `<span class="token variable">${word}</span>`,
    `declared name ${word} must be pale blue like Visual Studio locals and fields`
  );
}
for (const word of ["try", "if", "return", "switch", "default", "else", "catch", "throw"]) {
  requireToken(
    `<span class="token control-keyword">${word}</span>`,
    `C# ${word} must use the Visual Studio pink control-keyword token`
  );
}
if (html.includes('<span class="token keyword">return</span>')) {
  throw new Error("return must not remain a plain keyword after try/switch in the same chunk");
}
if (html.includes('<span class="token keyword">if</span>')) {
  throw new Error("if must not remain a plain keyword next to pink try/catch/switch");
}

if ((html.match(/class="token function">Dispatch</g) || []).length !== 1) {
  throw new Error("Dispatch must not be reclassified as a type name");
}

console.log("Prism C# Visual Studio token mapping tests passed.");
