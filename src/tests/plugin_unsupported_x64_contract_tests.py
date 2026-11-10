# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-10 | Guard x64 plug-in unsupported classification. | Prevent path-based rejection of x64 Encrypt while preserving the WinSCP exception. | | GPT-6 (Codex) |

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PLUGINS = ROOT / "src" / "plugins2.cpp"


def main() -> int:
    source = PLUGINS.read_text(encoding="utf-8")
    start = source.index("BOOL IsPluginUnsupportedOnX64(")
    end = source.index("#endif // _WIN64", start)
    function = source[start:end]

    assert '_stricmp(dllName, "winscp\\\\winscp.spl") == 0' in function, (
        "The x64 WinSCP exception must remain"
    )
    assert 'nameEN = "WinSCP (SFTP/SCP Client)";' in function, (
        "The x64 WinSCP warning name must remain"
    )
    assert "encrypt\\\\encrypt.spl" not in function.lower(), (
        "encrypt\\encrypt.spl must reach normal DLL loading on x64"
    )
    assert 'nameEN = "Encrypt";' not in function, (
        "Encrypt must not be classified as always unsupported on x64"
    )
    print("x64 plug-in unsupported classification contract passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
