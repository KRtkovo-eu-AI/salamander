"use strict";

(function (root) {
  const ESCAPED_QUOTE_STRING = '"(?:[\\\\"]"|[^"])*"(?!")';
  const WINDOWS_QUOTED_ARGUMENT = '"[^"\\r\\n]*"';
  const VARIABLE = /%%?[~:\w]+%?|!\S+!/;

  function rewritePattern(pattern) {
    if (!(pattern instanceof RegExp) || pattern.source.indexOf(ESCAPED_QUOTE_STRING) < 0) {
      return pattern;
    }
    return new RegExp(
      pattern.source.split(ESCAPED_QUOTE_STRING).join(WINDOWS_QUOTED_ARGUMENT),
      pattern.flags
    );
  }

  // Visual Studio Dark+ paints these as keyword.control (#c586c0). Keep the
  // source as a string so every grammar / nested `inside` gets a fresh /g
  // RegExp; sharing one lastIndex across insides drops later matches.
  const CONTROL_KEYWORD_SOURCE =
    "\\b(?:await|break|case|catch|continue|default|do|else|finally|for|foreach|goto|if|lock|return|switch|throw|try|when|while|yield)\\b";

  function controlKeywordToken() {
    return {
      pattern: new RegExp(CONTROL_KEYWORD_SOURCE, "g"),
      greedy: true
    };
  }

  function insertBeforeInPlace(grammar, before, insert) {
    if (!grammar || typeof grammar !== "object") {
      return grammar;
    }
    const rebuilt = {};
    Object.keys(grammar).forEach(function (token) {
      if (token === before) {
        Object.keys(insert).forEach(function (name) {
          rebuilt[name] = insert[name];
        });
      }
      if (!Object.prototype.hasOwnProperty.call(insert, token)) {
        rebuilt[token] = grammar[token];
      }
    });
    Object.keys(grammar).forEach(function (token) {
      delete grammar[token];
    });
    Object.keys(rebuilt).forEach(function (token) {
      grammar[token] = rebuilt[token];
    });
    return grammar;
  }

  function addControlKeywords(grammar, seen) {
    if (!grammar || typeof grammar !== "object" || seen.has(grammar)) {
      return;
    }
    seen.add(grammar);
    if (grammar.keyword) {
      insertBeforeInPlace(grammar, "keyword", {
        "control-keyword": controlKeywordToken()
      });
    }
    Object.keys(grammar).forEach(function (key) {
      const value = grammar[key];
      if (Array.isArray(value)) {
        value.forEach(function (item) {
          if (item && item.inside) {
            addControlKeywords(item.inside, seen);
          }
        });
      } else if (value && typeof value === "object" && value.inside) {
        addControlKeywords(value.inside, seen);
      }
    });
  }

  function addTypeNamespacePrefixes(grammar, seen, isRoot) {
    if (!grammar || typeof grammar !== "object" || seen.has(grammar)) {
      return;
    }
    seen.add(grammar);
    // Only inside type tokens. At the root, PascalCase before "." is a type
    // (IntPtr.Zero). Inside a qualified type name, those prefixes are namespaces
    // (System.Threading.Timer).
    if (!isRoot && grammar.punctuation && grammar.keyword && !grammar["maybe-namespace"]) {
      insertBeforeInPlace(grammar, "punctuation", {
        "maybe-namespace": {
          pattern: new RegExp("[A-Z][A-Za-z0-9_]*(?=\\s*\\.)", "g"),
          greedy: true,
          alias: "namespace"
        }
      });
    }
    Object.keys(grammar).forEach(function (key) {
      const value = grammar[key];
      if (Array.isArray(value)) {
        value.forEach(function (item) {
          if (item && item.inside) {
            addTypeNamespacePrefixes(item.inside, seen, false);
          }
        });
      } else if (value && typeof value === "object" && value.inside) {
        addTypeNamespacePrefixes(value.inside, seen, false);
      }
    });
  }

  function patchCsharpGrammar(Prism, language) {
    if (language !== "csharp" || !Prism || !Prism.languages || !Prism.languages.csharp) {
      return;
    }
    const csharp = Prism.languages.csharp;
    const seen = typeof WeakSet === "function" ? new WeakSet() : new Set();
    addControlKeywords(csharp, seen);
    addTypeNamespacePrefixes(csharp, typeof WeakSet === "function" ? new WeakSet() : new Set(), true);
    // Match Visual Studio: IntPtr parentHandle = IntPtr.Zero;
    //   IntPtr        type (teal)
    //   parentHandle  local/field (pale blue)
    //   Zero          member after "." (default foreground)
    insertBeforeInPlace(csharp, "punctuation", {
      "maybe-member": {
        pattern: new RegExp("(\\?\\.|\\.)[A-Z][A-Za-z0-9_]*(?!\\s*\\()", "g"),
        lookbehind: true,
        greedy: true
      },
      "maybe-class-name": {
        pattern: /\b[A-Z][A-Za-z0-9_]*(?=\s*\.)/,
        alias: "class-name"
      },
      variable: {
        pattern: new RegExp("\\b[A-Za-z_]\\w*\\b", "g"),
        greedy: true
      }
    });
  }

  function patchBatchGrammar(Prism, language) {
    if (language !== "batch" || !Prism || !Array.isArray(Prism.languages.batch.command)) {
      return;
    }
    const quotedArgument = new RegExp(WINDOWS_QUOTED_ARGUMENT);
    Prism.languages.batch.command.forEach(function (command) {
      if (!command) {
        return;
      }
      command.pattern = rewritePattern(command.pattern);
      if (command.inside && command.inside.string) {
        command.inside.string = {
          // cmd.exe does not use a backslash to escape a quote. Treating \"
          // as an escaped quote joins adjacent Windows path arguments and
          // makes later copy lines inherit the previous string token.
          pattern: quotedArgument,
          greedy: true,
          inside: {
            variable: VARIABLE
          }
        };
      }
    });
  }

  function encodeMarkupText(Prism, text) {
    if (Prism && Prism.util && typeof Prism.util.encode === "function") {
      const encoded = Prism.util.encode(text);
      if (typeof encoded === "string") {
        return encoded;
      }
    }
    return String(text).replace(/&/g, "&amp;").replace(/</g, "&lt;");
  }

  function lastUnclosedMarkupComment(text) {
    let open = -1;
    let index = 0;
    while (index < text.length) {
      if (open < 0) {
        const start = text.indexOf("<!--", index);
        if (start < 0) {
          return -1;
        }
        open = start;
        index = start + 4;
      } else {
        const close = text.indexOf("-->", index);
        if (close < 0) {
          return open;
        }
        open = -1;
        index = close + 3;
      }
    }
    return open;
  }

  // Virtual chunks are highlighted independently. XML comments that cross a
  // chunk boundary must stay comment tokens, or `</Import>` inside them is
  // painted as a live tag.
  function highlightMarkupChunk(Prism, text, grammar, language, insideComment) {
    if (!Prism || typeof Prism.highlight !== "function" || !grammar) {
      return encodeMarkupText(Prism, text);
    }
    let html = "";
    let remaining = String(text || "");
    let inComment = Boolean(insideComment);
    while (remaining) {
      if (inComment) {
        const close = remaining.indexOf("-->");
        if (close < 0) {
          html += '<span class="token comment">' + encodeMarkupText(Prism, remaining) + "</span>";
          break;
        }
        const commentEnd = close + 3;
        html +=
          '<span class="token comment">' +
          encodeMarkupText(Prism, remaining.slice(0, commentEnd)) +
          "</span>";
        remaining = remaining.slice(commentEnd);
        inComment = false;
        continue;
      }
      const open = lastUnclosedMarkupComment(remaining);
      if (open < 0) {
        html += Prism.highlight(remaining, grammar, language);
        break;
      }
      if (open > 0) {
        html += Prism.highlight(remaining.slice(0, open), grammar, language);
      }
      remaining = remaining.slice(open);
      inComment = true;
    }
    return html;
  }

  function patchMarkupGrammar(Prism, language) {
    if (
      (language !== "markup" && language !== "xml" && language !== "html") ||
      !Prism ||
      !Prism.languages ||
      !Prism.languages.markup
    ) {
      return;
    }
    const markup = Prism.languages.markup;
    const msbuildProperty = {
      pattern: /\$\([A-Za-z_][\w.]*\)/,
      greedy: true,
      alias: "variable"
    };
    if (!markup["msbuild-property"]) {
      insertBeforeInPlace(markup, "tag", {
        "msbuild-property": msbuildProperty
      });
    }
    const tagInside = markup.tag && markup.tag.inside;
    const attrValue = tagInside && tagInside["attr-value"];
    const attrValues = Array.isArray(attrValue) ? attrValue : attrValue ? [attrValue] : [];
    attrValues.forEach(function (value) {
      if (!value || !value.inside || value.inside["msbuild-property"]) {
        return;
      }
      insertBeforeInPlace(value.inside, "punctuation", {
        "msbuild-property": {
          pattern: /\$\([A-Za-z_][\w.]*\)/,
          greedy: true,
          alias: "variable"
        }
      });
    });
  }

  root.SalamanderPrism = {
    patchLanguage: function (Prism, language) {
      patchBatchGrammar(Prism, language);
      patchCsharpGrammar(Prism, language);
      patchMarkupGrammar(Prism, language);
    },
    highlightMarkupChunk: highlightMarkupChunk
  };
}(self));
