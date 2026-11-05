# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-05 | Contract-check SVG FS-panel icon sizes and alpha pipeline. | Salamatrix icons were stuck at 16px and double-premultiplied. | | Composer |

"""Source contracts for SVG icons used as FS-panel item artwork."""

from pathlib import Path
import re
import sys


ROOT = Path(__file__).resolve().parents[1]


def require(text: str, pattern: str, message: str) -> None:
    if not re.search(pattern, text, re.MULTILINE | re.DOTALL):
        raise AssertionError(message)


def require_absent(text: str, pattern: str, message: str) -> None:
    if re.search(pattern, text, re.MULTILINE | re.DOTALL):
        raise AssertionError(message)


def main() -> int:
    svg = (ROOT / "svg.cpp").read_text(encoding="utf-8")
    svg_header = (ROOT / "svg.h").read_text(encoding="utf-8")
    plugins3 = (ROOT / "plugins3.cpp").read_text(encoding="utf-8")
    packages = (
        ROOT / "plugins" / "salamatrix" / "salamatrix_packages.cpp"
    ).read_text(encoding="utf-8")
    thumbs = (
        ROOT / "plugins" / "pictview" / "thumbs.cpp"
    ).read_text(encoding="utf-8")
    fileswn0 = (ROOT / "fileswn0.cpp").read_text(encoding="utf-8")

    require(
        fileswn0,
        r"case vmThumbnails:\s*return ICONSIZE_48",
        "thumbnail view must request the 48px panel icon bucket",
    )
    require(
        packages,
        r"SalamatrixIconSizeToPixels\(int iconSize\).*?"
        r"case SALICONSIZE_48:\s*return 48;.*?"
        r"case SALICONSIZE_32:\s*return 32;.*?"
        r"return 16;",
        "Salamatrix must map SALICONSIZE_16/32/48 to matching pixel sizes",
    )
    require(
        packages,
        r"GetPluginIcon\([\s\S]*?"
        r"SalamatrixIconSizeToPixels\(iconSize\).*?"
        r"CreateSVGIcon\(preferred\.c_str\(\), pixels\)",
        "Salamatrix FS SVG icons must be rasterized at the requested pixel size",
    )
    require_absent(
        packages,
        r"CreateSVGIcon\([^;]*iconSize == SALICONSIZE_32 \? 32 : 16\)",
        "Salamatrix must not collapse SALICONSIZE_48 SVG icons to 16px",
    )

    require(
        svg,
        r"DownsamplePremultipliedBGRA2x|"
        r"renderSize = \(iconSize <= 64\) \? iconSize \* 2 : iconSize",
        "SVG icon bitmaps must supersample small panel sizes for detail",
    )
    require(
        svg,
        r'nsvgParse\(svg, "px", 96\.0f\)',
        "SVG icon parse must use the CSS reference DPI, not the percent scale",
    )
    require(
        svg_header + svg,
        r"UnpremultiplySVGIconBitmap\(HBITMAP hBitmap\)",
        "SVG icon HICON creation needs a premul-to-straight conversion helper",
    )
    require_absent(
        svg,
        r"red = \(BYTE\)\(\(red \* alpha \+ 127\) / 255\);\s*"
        r"green = \(BYTE\)\(\(green \* alpha \+ 127\) / 255\);\s*"
        r"blue = \(BYTE\)\(\(blue \* alpha \+ 127\) / 255\);",
        "RenderSVGIconBitmapText must not double-premultiply NanoSVG output",
    )
    require(
        plugins3,
        r"CreateSVGIcon\([\s\S]*?"
        r"UnpremultiplySVGIconBitmap\(hColorBitmap\).*?"
        r"src\[x \* 4 \+ 3\] < 16",
        "CreateSVGIcon must emit straight alpha and build the AND mask from it",
    )
    require(
        thumbs,
        r"nsvgRasterize emits premultiplied BGRA.*?"
        r"p\[0\] = \(BYTE\)min\(255u, \(unsigned\)p\[0\] \+ \(bgB \* inv \+ 127\) / 255\)",
        "SVG file thumbnails must composite premul pixels without a second alpha multiply",
    )
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except AssertionError as exc:
        print(str(exc))
        sys.exit(1)
