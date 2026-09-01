# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
SELFEXTR = ROOT / "plugins" / "zip" / "vcxproj" / "selfextr"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def normalized(path: Path) -> str:
    return path.read_text(encoding="ascii").replace("\r\n", "\n")


def main() -> int:
    makeall = normalized(SELFEXTR / "makeall.bat")
    sfxmake = normalized(SELFEXTR / "sfxmake.bat")

    lock_attempt = re.search(
        r':acquire_sfx_build_lock\n(?P<body>.*?)(?=set "sfx_build_lock_acquired=yes")',
        makeall,
        re.DOTALL | re.IGNORECASE,
    )
    require(lock_attempt is not None, "makeall must have an explicit lock acquisition loop")
    lock_body = lock_attempt.group("body")
    require(
        re.search(r'mkdir "%sfx_build_lock%"\nif errorlevel 1 \(', lock_body, re.IGNORECASE),
        "lock ownership must be based on mkdir errorlevel",
    )
    require(
        'if not exist "%sfx_build_lock%"' not in lock_body.lower(),
        "an existing lock directory must not be mistaken for successful acquisition",
    )
    require(
        "goto acquire_sfx_build_lock" in lock_body.lower(),
        "failed lock acquisition must wait and retry",
    )
    require(
        'if "%sfx_build_lock_acquired%"=="yes" rmdir' in makeall.lower(),
        "makeall may release the lock only after recording ownership",
    )
    require(
        re.search(
            r'call "%~dp0sfxmake\.bat" %%v sfx\\\n  if errorlevel 1 goto build_failed',
            makeall,
            re.IGNORECASE,
        ),
        "makeall must stop its language loop immediately when sfxmake fails",
    )
    require(
        "endlocal & exit /b %build_exit_code%" in makeall.lower(),
        "makeall must retain and return the build status through cleanup",
    )

    msbuild_checks = re.findall(
        r'^\s*"%MSB%" .*\n\s*(if errorlevel 1 goto (?:build_failed|make_sfx_failed))$',
        sfxmake,
        re.MULTILINE | re.IGNORECASE,
    )
    require(
        len(msbuild_checks) == 3,
        "tool, Release, and ReleaseEx MSBuild failures must immediately route to a nonzero exit",
    )

    packaging = re.search(
        r'^%sfxmake_exe% .*\n(?P<check>if errorlevel 1 exit /b %errorlevel%)$',
        sfxmake,
        re.MULTILINE | re.IGNORECASE,
    )
    require(packaging is not None, "the final packaging tool failure must be propagated")
    require(
        re.search(
            r':MAKE_SFX_FAILED\nendlocal & exit /b %errorlevel%',
            sfxmake,
            re.IGNORECASE,
        ),
        "nested MAKE_SFX setlocal failures must retain their status across endlocal",
    )
    require(
        'if "%making_all_versions%"=="" pause' in sfxmake.lower(),
        "standalone builds should remain interactive without pausing makeall",
    )
    require(
        "endlocal & exit /b %sfxmake_exit_code%" in sfxmake.lower(),
        "sfxmake must return its retained status to the caller",
    )

    print("SFX batch lock and error propagation contracts are enforced")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())