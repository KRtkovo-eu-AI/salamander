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

    print("WebView2 render viewer source-contract tests passed.")


if __name__ == "__main__":
    main()
