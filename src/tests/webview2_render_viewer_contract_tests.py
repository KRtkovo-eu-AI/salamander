# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path
import re


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
MARKDIG_RENDERER = (
    REPOSITORY_ROOT
    / "src"
    / "plugins"
    / "shared"
    / "webviewviewer"
    / "markdighelper"
    / "Program.cs"
)
TEXTVIEWER_PROJECT = (
    REPOSITORY_ROOT / "src" / "plugins" / "textviewer" / "vcxproj" / "textviewer.vcxproj"
)
TEXTVIEWER_SOURCE = (
    REPOSITORY_ROOT / "src" / "plugins" / "textviewer" / "textviewer.cpp"
)
TEXTVIEWER_RESOURCES = (
    REPOSITORY_ROOT / "src" / "plugins" / "textviewer" / "lang" / "lang.rc2",
    REPOSITORY_ROOT / "src" / "plugins" / "textviewer" / "textviewer.rh2",
)
WEBVIEW_PROJECT = (
    REPOSITORY_ROOT
    / "src"
    / "plugins"
    / "webview2renderviewer"
    / "vcxproj"
    / "webview2renderviewer.vcxproj"
)
SALAMATRIX_PROJECT = (
    REPOSITORY_ROOT
    / "src"
    / "plugins"
    / "salamatrix"
    / "vcxproj"
    / "salamatrix.vcxproj"
)
NATIVE_VIEWER = (
    REPOSITORY_ROOT
    / "src"
    / "plugins"
    / "shared"
    / "webviewviewer"
    / "native_viewer.cpp"
)
WEBVIEW_PLUGIN_SOURCE = (
    REPOSITORY_ROOT
    / "src"
    / "plugins"
    / "webview2renderviewer"
    / "webview2renderviewer.cpp"
)
VIRTUAL_PRISM_DIRECTORY = (
    REPOSITORY_ROOT
    / "src"
    / "plugins"
    / "shared"
    / "webviewviewer"
    / "prism"
    / "viewer"
)


def main() -> None:
    source = MARKDIG_RENDERER.read_text(encoding="utf-8")
    textviewer_source = TEXTVIEWER_SOURCE.read_text(encoding="utf-8")

    create_pipeline = re.search(
        r"private static MarkdownPipeline CreatePipeline\(\).*?(?=\n    private static)",
        source,
        re.DOTALL,
    )
    if create_pipeline is None:
        raise AssertionError("MarkdownRenderer.CreatePipeline was not found")

    pipeline_source = create_pipeline.group(0)
    if ".UseAdvancedExtensions()" not in pipeline_source:
        raise AssertionError("the Markdown viewer must retain advanced extensions")
    if "GenericAttributesExtension" not in pipeline_source:
        raise AssertionError(
            "generic attributes must be disabled so curly-braced placeholders remain visible"
        )
    if "builder.Extensions.RemoveAt(index)" not in pipeline_source:
        raise AssertionError("the generic attributes extension is detected but not removed")

    advanced_call = pipeline_source.index(".UseAdvancedExtensions()")
    removal = pipeline_source.index("builder.Extensions.RemoveAt(index)")
    build_call = pipeline_source.index("return builder.Build()")
    if not advanced_call < removal < build_call:
        raise AssertionError(
            "generic attributes must be removed after advanced setup and before pipeline build"
        )

    textviewer_project = TEXTVIEWER_PROJECT.read_text(encoding="utf-8")
    webview_project = WEBVIEW_PROJECT.read_text(encoding="utf-8")
    salamatrix_project = SALAMATRIX_PROJECT.read_text(encoding="utf-8")
    native_viewer = NATIVE_VIEWER.read_text(encoding="utf-8")
    webview_plugin_source = WEBVIEW_PLUGIN_SOURCE.read_text(encoding="utf-8")
    virtual_viewer = (VIRTUAL_PRISM_DIRECTORY / "virtual-viewer.js").read_text(
        encoding="utf-8"
    )
    virtual_html = (VIRTUAL_PRISM_DIRECTORY / "virtual-viewer.html").read_text(
        encoding="utf-8"
    )
    prism_worker = (VIRTUAL_PRISM_DIRECTORY / "prism-worker.js").read_text(
        encoding="utf-8"
    )
    publish_command = "dotnet publish"
    renderer_project = "markdighelper\\MarkdigRenderer.csproj"
    if publish_command in webview_project or renderer_project in webview_project:
        raise AssertionError(
            "webview2renderviewer must consume MarkdigRenderer, not publish it"
        )
    if salamatrix_project.count(publish_command) != 1 or renderer_project not in salamatrix_project:
        raise AssertionError("salamatrix must be the single MarkdigRenderer producer")
    if "StagePrismAssets" in textviewer_project or "RestorePrismAssets" in textviewer_project:
        raise AssertionError("textviewer must consume Salamatrix's shared Prism assets")
    if salamatrix_project.count("StageViewerPrismAssets") != 1:
        raise AssertionError("salamatrix must be the single Prism asset producer")
    if 'L"plugins\\\\salamatrix\\\\prism"' not in native_viewer:
        raise AssertionError("Viewer Frame must consume Prism assets from Salamatrix")
    if 'L"utils\\\\MarkdigRenderer.exe"' not in native_viewer:
        raise AssertionError("the shared native viewer must consume MarkdigRenderer from utils")
    if "WM_NV_PREWARM_ENVIRONMENT" not in native_viewer:
        raise AssertionError("WebView2 runtime must be prewarmed before the first viewer invocation")
    if "ViewerHostThread" not in native_viewer or "gSharedEnvironment" not in native_viewer:
        raise AssertionError("Viewer Frame must retain one STA host and its shared WebView2 environment")
    async_contract = (
        "CreateThread(nullptr, 0, PrepareDocumentWorker",
        "WM_NV_PREPARATION_COMPLETE",
        "preparationGeneration_",
        "add_DOMContentLoaded",
    )
    if any(token not in native_viewer for token in async_contract):
        raise AssertionError(
            "document preparation must remain asynchronous, generation-safe, and DOM-ready"
        )
    navigation_completed = re.search(
        r"add_NavigationCompleted\(.*?\)\.Get\(\), &navigationToken_\);",
        native_viewer,
        re.DOTALL,
    )
    if navigation_completed is None:
        raise AssertionError("NavigationCompleted handler was not found")
    navigation_source = navigation_completed.group(0)
    if "NV_PRISM_READY_TIMER" in native_viewer or "PostVirtualInit()" in navigation_source:
        raise AssertionError(
            "Prism navigation must not depend on a hidden-WebView readiness timer"
        )
    if (
        'value == L"salamander-prism-ready"' not in native_viewer
        or "CompletePrismDisplay()" not in native_viewer
        or 'value == L"salamander-prism-theme-ready"' not in native_viewer
        or "self->ShowPrismBrowser()" not in native_viewer
        or "ApplyControllerZoom()" not in native_viewer
    ):
        raise AssertionError(
            "Prism must reveal only its themed page and report Ready after painted content"
        )
    if "Prism.highlightElement(code);finish();" in native_viewer:
        raise AssertionError("small Prism files must not highlight on the UI thread via NavigateToString")
    if (
        'value == L"salamander-virtual-ready"' not in native_viewer
        or "self->virtualInitSent_ = false;" not in native_viewer
        or "self->PostVirtualInit()" not in native_viewer
    ):
        raise AssertionError("virtual Prism initialization must follow the page-ready handshake")
    if '{L"bat", L"batch"}' not in native_viewer or '{L"cmd", L"batch"}' not in native_viewer:
        raise AssertionError(".bat and .cmd files must use Prism's batch syntax highlighter")
    if '{L"config", L"markup"}' not in native_viewer or '{L"targets", L"markup"}' not in native_viewer:
        raise AssertionError(".config and .targets files must use Prism markup/XML highlighting")
    virtual_contract = (
        "NV_PRISM_VIRTUAL_THRESHOLD = 512U * 1024U",
        "NV_VIRTUAL_CHUNK_LINES = 80",
        "NV_VIRTUAL_CHUNK_CHARS = 96U * 1024U",
        "PreparationKind::PrismVirtual",
        "virtualLineStarts_",
        "virtualInitSent_",
        'L"salamander-chunk:"',
        'L"https://prism.local/viewer/virtual-viewer.html?g="',
        "ApplyControllerZoom()",
    )
    if any(token not in native_viewer for token in virtual_contract):
        raise AssertionError("Prism files must use native line indexing and virtualization")
    if (
        "NV_PRISM_FILE_LIMIT = 16U * 1024U * 1024U" not in native_viewer
        or "NV_MARKDOWN_FILE_LIMIT = 32U * 1024U * 1024U" not in native_viewer
        or "context->markdown ? NV_MARKDOWN_FILE_LIMIT : NV_PRISM_FILE_LIMIT"
        not in native_viewer
    ):
        raise AssertionError("native preparation limits must match both plug-in routing policies")
    if (
        'loading=\\"lazy\\"' not in native_viewer
        or 'decoding=\\"async\\"' not in native_viewer
        or 'fetchpriority=\\"low\\"' not in native_viewer
    ):
        raise AssertionError("rendered Markdown images must load lazily after DOM rendering")
    if (
        'AddWebResourceRequestedFilter(' not in native_viewer
        or 'L"https://markdown.local/*"' not in native_viewer
        or "CreateWebResourceResponse(" not in native_viewer
        or "webView_->Navigate(markdownDocumentUri_.c_str())" not in native_viewer
    ):
        raise AssertionError(
            "rendered Markdown must stream through WebView2 without NavigateToString's size limit"
        )
    if (
        (
            'new URL("prism-worker.js", document.baseURI)' not in virtual_viewer
            and 'new Worker("prism-worker.js")' not in virtual_viewer
        )
        or "IntersectionObserver" not in virtual_viewer
        or "unmount(state)" not in virtual_viewer
        or "syncVisibleChunks()" not in virtual_viewer
        or "scheduleVisibleSync()" not in virtual_viewer
        or "slotNearViewport" not in virtual_viewer
        or "materialize(first)" not in virtual_viewer
        or "READY_FALLBACK_MS" not in virtual_viewer
        or "CHUNK_REQUEST_TIMEOUT_MS" not in virtual_viewer
        or "settings.generation" not in virtual_viewer
        or 'document.readyState === "loading"' not in virtual_viewer
        or "notifyHostReady()" not in virtual_viewer
        or 'window.addEventListener("resize", scheduleResize)' not in virtual_viewer
        or 'window.addEventListener("scroll", scheduleVisibleSync' not in virtual_viewer
        or "ResizeObserver" in virtual_viewer
        or "measureLineColumns(message.text, state.lineCount)" not in virtual_viewer
        or "recreateObserver()" not in virtual_viewer
        or "cachedHtml" not in virtual_viewer
        or "ensureLanguage(canonicalLanguage())" not in virtual_viewer
        or "Prism.plugins.autoloader" not in virtual_viewer
        or 'languages_path = "https://prism.local/components/"' not in virtual_viewer
        or "loadLanguages" not in virtual_viewer
        or "highlightOnMainThread" not in virtual_viewer
        or "layoutLineNumbers(pre, state, true)" not in virtual_viewer
        or 'resolveAsset("prism-language-graph.js")' not in prism_worker
        or "SalamanderPrismLanguageGraph" not in prism_worker
        or 'resolveAsset("../components/prism-" + canonical + ".min.js")'
        not in prism_worker
        or "request.generation" not in prism_worker
        or 'resolveAsset("prism-patches.js")' not in prism_worker
        or "LANGUAGE_ALIASES" not in prism_worker
        or 'cmd: "batch"' not in prism_worker
        or 'config: "markup"' not in prism_worker
        or 'targets: "markup"' not in prism_worker
        or "HIGHLIGHT_TIMEOUT_MS = 5000" not in virtual_viewer
        or "failedWorker.terminate()" not in virtual_viewer
        or "startWorker();" not in virtual_viewer
        or "THEME_READY_MESSAGE" not in virtual_viewer
        or "highlightPending" not in virtual_viewer
        or "showing available text" not in virtual_viewer
        or "estimateChunkHeight" not in virtual_viewer
        or 'message === "salamander-resize"' not in virtual_viewer
        or "if (!state || Number(message.lineCount) !== state.lineCount" not in virtual_viewer
    ):
        raise AssertionError(
            "the virtual Prism shell must virtualize chunks and highlight them off the UI thread"
        )
    if (
        "letter-spacing: calc(var(--viewer-char-width) - 1ch)" not in virtual_html
        and "letter-spacing:calc(var(--viewer-char-width) - 1ch)" not in virtual_html
    ):
        raise AssertionError("the virtual shell must match Internal Viewer character pitch")
    if "content: \"\\21AA\"" not in virtual_html and 'content: "\\21AA"' not in virtual_html:
        raise AssertionError("the virtual shell must draw Internal Viewer wrap markers")
    if (
        "salamander-wrap-sizer" in native_viewer
        or "Prism.highlightElement(code);finish();" in native_viewer
        or "webView_->NavigateToString(html.c_str())" in native_viewer
    ):
        raise AssertionError(
            "Prism must not keep a second NavigateToString highlighting path"
        )
    if (
        'PostWebMessageAsString(L"salamander-resize")' not in native_viewer
        or "relayoutMountedChunks()" not in virtual_viewer
    ):
        raise AssertionError(
            "native resize must explicitly refresh virtual wrap-marker layouts"
        )
    if "#6a9955" not in virtual_viewer or "#569cd6" not in virtual_viewer or "#ce9178" not in virtual_viewer:
        raise AssertionError("dark Prism highlighting must keep the Visual Studio Dark+ token colors")
    if "#c586c0" not in virtual_viewer:
        raise AssertionError("Visual Studio Dark+ must keep pink control-flow keywords")
    if "#008000" not in virtual_viewer or "#0000ff" not in virtual_viewer or "#a31515" not in virtual_viewer:
        raise AssertionError("light Prism highlighting must keep the Visual Studio token colors")
    if "#dcdcaa" not in virtual_viewer or "#4ec9b0" not in virtual_viewer or "#b5cea8" not in virtual_viewer:
        raise AssertionError("Visual Studio Dark+ must keep yellow methods, teal types, and green numbers")
    if (
        "patchCsharpGrammar" not in (VIRTUAL_PRISM_DIRECTORY / "prism-patches.js").read_text(
            encoding="utf-8"
        )
        or "maybe-class-name" not in virtual_html
        or "maybe-member" not in virtual_html
        or "named-parameter" not in virtual_html
        or "--token-boolean" not in virtual_html
        or "boolean: \"#569cd6\"" not in virtual_viewer
        or "controlKeyword: \"#c586c0\"" not in virtual_viewer
        or "control-keyword" not in (VIRTUAL_PRISM_DIRECTORY / "prism-patches.js").read_text(
            encoding="utf-8"
        )
        or "insertBeforeInPlace" not in (VIRTUAL_PRISM_DIRECTORY / "prism-patches.js").read_text(
            encoding="utf-8"
        )
        or "greedy: true" not in (VIRTUAL_PRISM_DIRECTORY / "prism-patches.js").read_text(
            encoding="utf-8"
        )
        or "maybe-member" not in (VIRTUAL_PRISM_DIRECTORY / "prism-patches.js").read_text(
            encoding="utf-8"
        )
        or "maybe-namespace" not in (VIRTUAL_PRISM_DIRECTORY / "prism-patches.js").read_text(
            encoding="utf-8"
        )
        or "maybe-namespace" not in virtual_html
        or ".token.tag" not in virtual_html
        or ".token.attr-name" not in virtual_html
        or "msbuild-property" not in (VIRTUAL_PRISM_DIRECTORY / "prism-patches.js").read_text(
            encoding="utf-8"
        )
        or "patchMarkupGrammar" not in (VIRTUAL_PRISM_DIRECTORY / "prism-patches.js").read_text(
            encoding="utf-8"
        )
        or "highlightMarkupChunk" not in (VIRTUAL_PRISM_DIRECTORY / "prism-patches.js").read_text(
            encoding="utf-8"
        )
        or "insideMarkupComment" not in native_viewer
        or "insideMarkupComment" not in virtual_viewer
        or "insideMarkupComment" not in prism_worker
        or "SalamanderPrism.highlightMarkupChunk" not in prism_worker
        or "SalamanderPrism.highlightMarkupChunk" not in virtual_viewer
        or 'pattern: new RegExp("\\\\b[A-Za-z_]\\\\w*\\\\b", "g")' not in (
            VIRTUAL_PRISM_DIRECTORY / "prism-patches.js"
        ).read_text(encoding="utf-8")
        or "variable: \"#9cdcfe\"" not in virtual_viewer
    ):
        raise AssertionError(
            "the Visual Studio palette must map Prism C# tokens the way Dark+ colors types, methods, and booleans"
        )
    if (
        "IDM_NV_COLORS_VISUAL_STUDIO" not in native_viewer
        or "IDM_NV_COLORS_PRISM" not in native_viewer
        or "IDM_NV_COLORS_CUSTOM" not in native_viewer
        or "IDM_NV_COLORS_EDIT_CUSTOM" not in native_viewer
        or "ShowEditCustomPalette" not in native_viewer
        or "ControlKindColorArrowButton" not in native_viewer
        or "WriteCustomPaletteFile" not in native_viewer
        or 'WriteViewerSetting(L"PrismColorPalette"' not in native_viewer
        or 'L"PrismCustomPalette"' not in native_viewer
        or "AppendCustomPaletteJson" not in native_viewer
        or 'L",\\"custom\\":"' not in native_viewer
        or 'L"PrismColorPalette"' not in native_viewer
        or 'L",\\"palette\\":"' not in native_viewer
        or 'message.type === "palette"' not in virtual_viewer
        or 'PALETTES = {' not in virtual_viewer
        or "PALETTES.custom" not in virtual_viewer
        or '"visual-studio"' not in virtual_viewer
        or "custom-palette.json" not in native_viewer
        or "applyIncomingCustomPalette" not in virtual_viewer
        or "applyTokenColors(tokenColors(settings.palette, dark))" not in virtual_viewer
        or "parseCustomPaletteJson" not in virtual_viewer
        or "rememberCustomPalette" not in virtual_viewer
    ):
        raise AssertionError(
            "Prism Text Viewer must persist a Colors menu with Visual Studio, Default Prism, and Custom palettes"
        )
    custom_palette = VIRTUAL_PRISM_DIRECTORY / "custom-palette.json"
    if not custom_palette.is_file():
        raise AssertionError("the Custom color palette must ship as viewer/custom-palette.json")
    custom_palette_text = custom_palette.read_text(encoding="utf-8")
    if '"light"' not in custom_palette_text or '"dark"' not in custom_palette_text:
        raise AssertionError("custom-palette.json must define light and dark token maps")
    textviewer_lang = TEXTVIEWER_RESOURCES[0].read_text(encoding="utf-8")
    textviewer_ids = TEXTVIEWER_RESOURCES[1].read_text(encoding="utf-8")
    if "IDS_VIEWER_COLORS" not in textviewer_lang or "IDS_VIEWER_COLORS" not in textviewer_ids:
        raise AssertionError("Prism Colors menu captions must be localizable")
    if (
        "IDS_VIEWER_COLORS_EDIT_CUSTOM" not in textviewer_lang
        or "IDS_VIEWER_COLORS_EDIT_CUSTOM" not in textviewer_ids
        or "IDS_VIEWER_COLORS_SAVE" not in textviewer_lang
        or "IDS_VIEWER_COLORS_LIGHT" not in textviewer_lang
        or "IDS_VIEWER_COLORS_CONTROL_KEYWORD" not in textviewer_lang
    ):
        raise AssertionError("Prism Edit Custom palette dialog captions must be localizable")
    for translation in (REPOSITORY_ROOT / "translations").glob("*/textviewer.slt"):
        translation_source = translation.read_text(encoding="utf-8")
        for string_id in ("68,1,", "69,1,", "70,1,", "71,1,", "72,1,", "74,1,", "76,1,", "77,1,", "91,1,"):
            if string_id not in translation_source:
                raise AssertionError(f"{translation} is missing Colors menu string {string_id}")
    if 'src="../plugins/autoloader/prism-autoloader.min.js"' not in virtual_html:
        raise AssertionError("the virtual shell must use Prism's autoloader for every syntax language")
    if "min-height: 100%" in virtual_html.split("pre.viewer-chunk", 1)[-1].split("pre.viewer-chunk > code", 1)[0]:
        raise AssertionError("chunk content must not fill a stale slot height after wrap/resize")
    if "--viewer-background: transparent" not in virtual_html:
        raise AssertionError("the virtual shell must not flash an unthemed background")
    if "prism-line-numbers.min.js" in virtual_html:
        raise AssertionError(
            "the virtual shell must not install Prism's full-text resize handler"
        )
    prism_patches = (VIRTUAL_PRISM_DIRECTORY / "prism-patches.js").read_text(
        encoding="utf-8"
    )
    if (
        "patchBatchGrammar" not in prism_patches
        or "ESCAPED_QUOTE_STRING" not in prism_patches
        or "rewritePattern" not in prism_patches
        or "command.inside.string" not in prism_patches
        or "SalamanderPrism.patchLanguage(Prism, language)" not in virtual_viewer
        or "SalamanderPrism.patchLanguage(Prism, canonical)" not in prism_worker
        or ".language-batch .token.string .token.variable" not in virtual_html
    ):
        raise AssertionError(
            "batch variables in quoted paths must use the shared runtime grammar patch"
        )
    for staged_asset in ("components.json", r"viewer\**\*.*"):
        if staged_asset not in salamatrix_project:
            raise AssertionError(f"salamatrix does not stage required Prism asset {staged_asset}")
    if (
        'if (extension == L".html" || extension == L".htm" || extension == L".xhtml")'
        not in native_viewer
        or 'if (extension == L".svg")' not in native_viewer
        or native_viewer.count("webView_->Navigate(mappedUri.c_str())") < 2
    ):
        raise AssertionError("HTML and SVG documents must stream through mapped WebView2 navigation")

    if (
        '".cmd"' not in textviewer_source
        or '".config"' not in textviewer_source
        or '".targets"' not in textviewer_source
        or "CanViewFile" not in textviewer_source
    ):
        raise AssertionError("Prism Text Viewer must accept .cmd, .config, and .targets files")

    large_file_fallback = re.search(
        r"if \(IsFileTooLarge\(name, kMaxTextFileSize\)\).*?^\s*\}",
        textviewer_source,
        re.DOTALL | re.MULTILINE,
    )
    if large_file_fallback is None:
        raise AssertionError("textviewer large-file fallback was not found")
    fallback_source = large_file_fallback.group(0)
    required_fallback_contract = (
        "CSalamanderPluginInternalViewerData fallbackData",
        "fallbackData.FileName = name",
        "fallbackData.Mode = 0",
        "SalPathFindFileName(name)",
        "ViewFileInPluginViewer(NULL, &fallbackData, returnLock",
    )
    if any(token not in fallback_source for token in required_fallback_contract):
        raise AssertionError(
            "textviewer must transfer oversized files to the Internal Viewer in text mode"
        )
    if "SalMessageBox" in fallback_source or "return FALSE" in fallback_source:
        raise AssertionError("textviewer must not reject oversized files locally")

    obsolete_resources = list(TEXTVIEWER_RESOURCES)
    obsolete_resources.extend(
        (REPOSITORY_ROOT / "translations").glob("*/textviewer.slt")
    )
    for resource in obsolete_resources:
        resource_source = resource.read_text(encoding="utf-8")
        if "IDS_FILE_TOO_LARGE" in resource_source or re.search(
            r"^51,1,", resource_source, re.MULTILINE
        ):
            raise AssertionError(
                f"obsolete textviewer too-large resource remains in {resource}"
            )

    if "kMaxDocumentFileSize = 32ULL * 1024ULL * 1024ULL" not in webview_plugin_source:
        raise AssertionError("the render viewer must retain its 32 MB document limit")
    if "if (IsFileTooLarge(name, kMaxDocumentFileSize))" not in webview_plugin_source:
        raise AssertionError("ViewFile must apply the size limit to every supported format")
    if "RequiresFullTextPreparation" in webview_plugin_source:
        raise AssertionError("the render viewer must not exempt streamed formats from its size limit")
    if (
        "MakeLongPath(ConvertPathToWide(path))" not in webview_plugin_source
        or "GetFileAttributesExW(" not in webview_plugin_source
    ):
        raise AssertionError("file-size checks must remain Unicode and long-path safe")

    print("WebView2 render viewer source-contract tests passed.")


if __name__ == "__main__":
    main()
