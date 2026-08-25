"use strict";

self.Prism = { manual: true };

function resolveAsset(relative) {
  try {
    if (self.location && /^https?:/i.test(self.location.href)) {
      return new URL(relative, self.location.href).href;
    }
  } catch (error) {
  }
  return relative;
}

importScripts(resolveAsset("../prism.js"));
importScripts(resolveAsset("prism-patches.js") + "?v=vs-tokens-10");
importScripts(resolveAsset("prism-language-graph.js"));

const LANGUAGE_ALIASES = Object.assign(
  {
    xml: "markup",
    html: "markup",
    svg: "markup",
    mathml: "markup",
    ssml: "markup",
    atom: "markup",
    rss: "markup",
    cmd: "batch",
    config: "markup",
    targets: "markup"
  },
  (self.SalamanderPrismLanguageGraph && self.SalamanderPrismLanguageGraph.aliases) || {}
);

const LANGUAGE_REQUIRES =
  (self.SalamanderPrismLanguageGraph && self.SalamanderPrismLanguageGraph.requires) || {};

let invisiblesLoaded = false;

function asArray(value) {
  if (!value) {
    return [];
  }
  return Array.isArray(value) ? value : [value];
}

function loadLanguage(language, visiting) {
  const canonical = LANGUAGE_ALIASES[language] || language;
  if (Prism.languages[canonical]) {
    return canonical;
  }
  if (visiting.has(canonical)) {
    return canonical;
  }

  visiting.add(canonical);
  asArray(LANGUAGE_REQUIRES[canonical]).forEach(function (dependency) {
    loadLanguage(dependency, visiting);
  });
  importScripts(resolveAsset("../components/prism-" + canonical + ".min.js"));
  visiting.delete(canonical);

  if (!Prism.languages[canonical]) {
    throw new Error("Prism component did not register: " + canonical);
  }
  return canonical;
}

function loadShowInvisibles() {
  if (!invisiblesLoaded) {
    importScripts(resolveAsset("../plugins/show-invisibles/prism-show-invisibles.min.js"));
    invisiblesLoaded = true;
  }
}

function encodePlainText(text) {
  return Prism.util.encode(text);
}

function highlight(request) {
  const language = LANGUAGE_ALIASES[String(request.language || "none").toLowerCase()] ||
    String(request.language || "none").toLowerCase();
  if (language === "none" || language === "plain" || language === "plaintext" || language === "text") {
    if (request.showWhitespace) {
      loadShowInvisibles();
      const environment = {
        code: request.text,
        grammar: {},
        language: "plain"
      };
      Prism.hooks.run("before-highlight", environment);
      return Prism.highlight(environment.code, environment.grammar, environment.language);
    }
    return encodePlainText(request.text);
  }

  const canonical = loadLanguage(language, new Set());
  SalamanderPrism.patchLanguage(Prism, canonical);
  if (request.showWhitespace) {
    loadShowInvisibles();
  }

  const environment = {
    code: request.text,
    grammar: Prism.languages[canonical],
    language: canonical
  };
  Prism.hooks.run("before-highlight", environment);
  if (canonical === "markup" && SalamanderPrism.highlightMarkupChunk) {
    return SalamanderPrism.highlightMarkupChunk(
      Prism,
      environment.code,
      environment.grammar,
      environment.language,
      Boolean(request.insideMarkupComment)
    );
  }
  return Prism.highlight(environment.code, environment.grammar, environment.language);
}

self.addEventListener("message", function (event) {
  const request = event.data;
  if (!request || request.type !== "highlight" || typeof request.text !== "string") {
    return;
  }

  try {
    const html = highlight(request);
    self.postMessage({
      type: "highlighted",
      ok: true,
      generation: request.generation,
      requestId: request.requestId,
      startLine: request.startLine,
      lineCount: request.lineCount,
      html: html
    });
  } catch (error) {
    self.postMessage({
      type: "highlighted",
      ok: false,
      generation: request.generation,
      requestId: request.requestId,
      startLine: request.startLine,
      lineCount: request.lineCount,
      text: request.text,
      error: error instanceof Error ? error.message : String(error)
    });
  }
});
