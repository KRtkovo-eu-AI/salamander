# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path
import re


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
VIEWER_HOST = (
    REPOSITORY_ROOT
    / "src"
    / "plugins"
    / "webview2renderviewer"
    / "managed"
    / "ViewerHost.cs"
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


def main() -> None:
    source = VIEWER_HOST.read_text(encoding="utf-8")

    create_pipeline = re.search(
        r"private static MarkdownPipeline CreatePipeline\(\).*?\n        }\n    }",
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

    webview_project = WEBVIEW_PROJECT.read_text(encoding="utf-8")
    salamatrix_project = SALAMATRIX_PROJECT.read_text(encoding="utf-8")
    native_viewer = NATIVE_VIEWER.read_text(encoding="utf-8")
    publish_command = "dotnet publish"
    renderer_project = "markdighelper\\MarkdigRenderer.csproj"
    if publish_command in webview_project or renderer_project in webview_project:
        raise AssertionError(
            "webview2renderviewer must consume MarkdigRenderer, not publish it"
        )
    if salamatrix_project.count(publish_command) != 1 or renderer_project not in salamatrix_project:
        raise AssertionError("salamatrix must be the single MarkdigRenderer producer")
    if 'L"utils\\\\MarkdigRenderer.exe"' not in native_viewer:
        raise AssertionError("the shared native viewer must consume MarkdigRenderer from utils")
    if "WM_NV_PREWARM_ENVIRONMENT" not in native_viewer:
        raise AssertionError("WebView2 runtime must be prewarmed before the first viewer invocation")
    if "ViewerHostThread" not in native_viewer or "gSharedEnvironment" not in native_viewer:
        raise AssertionError("Viewer Frame must retain one STA host and its shared WebView2 environment")
    if 'if (extension == L".svg")' not in native_viewer or "webView_->NavigateToString(svg.c_str())" not in native_viewer:
        raise AssertionError("small standalone SVG documents must use the direct in-memory navigation path")

    print("WebView2 render viewer source-contract tests passed.")


if __name__ == "__main__":
    main()
