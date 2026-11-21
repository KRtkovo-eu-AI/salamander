# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-21 | Compile the prefetch contention monitor in the native test. | Validate warning decisions with controlled timings. | | gpt-6.2-sol |
# | 2026-11-21 | Exercise actual viewer timing API across translation units and threads. | Verify anonymous exclusive timings and sample lifecycle before benchmarking. | | GPT-6 (Codex) |
"""Compile the actual PictView performance header; validate its emitted CSV."""

import argparse
from contextlib import contextmanager
import csv
import math
from pathlib import Path
import shutil
import tempfile

from disk_selection_snapshot_tests import ROOT, compiler_environment, run


FIELDS = (
    "pid,tid,sample,action,outcome,file_bytes,width,height,display_width,display_height,"
    "total_ms,navigation_ms,open_other_ms,decode_ms,exif_ms,transform_ms,scale_ms,paint_ms,"
    "other_ms,decode_attempts,scale_attempts,transform_attempts,draw_attempts,paint_calls"
).split(",")
PHASES = FIELDS[11:19]


@contextmanager
def fixture_directory():
    # Use an extended path for cleanup too; raw shutil.rmtree cannot remove
    # the deepest fixture directories on systems without long-path opt-in.
    with tempfile.TemporaryDirectory(prefix="pictview-performance-") as temporary:
        directory = Path(temporary).resolve()
        try:
            yield directory
        finally:
            expected_parent = Path(tempfile.gettempdir()).resolve()
            if directory.parent != expected_parent or not directory.name.startswith("pictview-performance-"):
                raise RuntimeError("Refusing cleanup outside the generated test directory")
            shutil.rmtree("\\\\?\\" + str(directory))


def check_csv(path):
    with path.open(newline="", encoding="ascii") as stream:
        reader = csv.DictReader(stream)
        assert reader.fieldnames == FIELDS, "CSV schema must contain only anonymous measurements"
        rows = list(reader)
    assert len(rows) == 7, "Exactly one row per completed or superseded sample"
    for row in rows:
        assert None not in row and all(value is not None for value in row.values()), "Complete CSV rows"
        assert row["action"] in ("open", "navigation")
        assert row["outcome"] in ("complete", "superseded", "painted", "thread")
        for name in FIELDS:
            if name not in ("action", "outcome"):
                assert math.isfinite(float(row[name])) and float(row[name]) >= 0, name
        total = float(row["total_ms"])
        accounted = sum(float(row[name]) for name in PHASES)
        # Every field is rounded to milliseconds with three decimal places.
        assert abs(total - accounted) <= 0.006, "Exclusive phases must partition total time"
    nested = rows[0]
    assert nested["action"] == "navigation" and nested["outcome"] == "complete"
    assert [int(nested[name]) for name in ("width", "height", "display_width", "display_height")] == [120, 80, 60, 40]
    assert int(nested["file_bytes"]) == 987654
    assert int(nested["decode_attempts"]) == 1 and int(nested["scale_attempts"]) == 1
    assert float(nested["open_other_ms"]) >= 15 and float(nested["decode_ms"]) >= 7 and float(nested["scale_ms"]) >= 7
    assert rows[1]["outcome"] == "superseded" and rows[2]["outcome"] == "complete"
    assert rows[3]["outcome"] == rows[4]["outcome"] == "painted"
    threads = rows[5:]
    assert all(row["outcome"] == "thread" for row in threads)
    assert len({row["tid"] for row in threads}) == 2
    assert {int(row["file_bytes"]) for row in threads} == {101, 202}
    assert all(row["sample"] == "1" and row["decode_attempts"] == "1" for row in threads)
    assert len({(row["pid"], row["tid"], row["sample"]) for row in rows}) == len(rows)
    print("CSV checks: anonymous schema, seven intact rows, exclusive totals, counters and thread isolation", flush=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--configuration", choices=("Debug", "Release", "Both"), default="Both")
    args = parser.parse_args()
    template = (ROOT / "src/tests/pictview_performance_tests.cpp.in").read_text(encoding="utf-8-sig")
    code = template.replace("@@PERFORMANCE_HEADER@@", (ROOT / "src/plugins/pictview/performance.h").as_posix())
    code = code.replace("@@PREFETCH_HEALTH_HEADER@@", (ROOT / "src/plugins/pictview/prefetchhealth.h").as_posix())
    assert "@@" not in code
    environment = compiler_environment()
    compiler = shutil.which("cl", path=environment.get("PATH"))
    if not compiler:
        raise RuntimeError("MSVC compiler was not found")
    with fixture_directory() as directory:
        source = directory / "test.cpp"
        helper = directory / "helper.cpp"
        source.write_text(code, encoding="utf-8")
        helper.write_text("#define PICTVIEW_PERF_SECOND_TU\n" + code, encoding="utf-8")
        executable = directory / "test.exe"
        unicode_directory = directory / "výkon_日本"
        unicode_directory.mkdir()
        long_directory = directory
        for index in range(5):
            long_directory /= f"segment_{index}_" + "x" * 50
        extended_directory = Path("\\\\?\\" + str(long_directory))
        extended_directory.mkdir(parents=True)
        assert len(str(long_directory)) > 260
        for configuration in ("Debug", "Release"):
            if args.configuration != "Both" and args.configuration != configuration:
                continue
            flags = ["/MDd", "/D_DEBUG", "/Od"] if configuration == "Debug" else ["/MD", "/DNDEBUG", "/O2"]
            print("Actual PictView performance API:", configuration, flush=True)
            result = run([compiler, "/nologo", "/std:c++17", "/EHsc", "/utf-8", "/W4", "/WX", *flags,
                          str(source), str(helper), "/Fe:" + str(executable), "gdi32.lib"],
                         cwd=directory, env=environment, timeout=40)
            if result.returncode:
                return result.returncode
            plain_log = directory / (configuration + ".csv")
            unicode_log = unicode_directory / (configuration + ".csv")
            raw_long_log = long_directory / (configuration + "-raw.csv")
            extended_log = extended_directory / (configuration + "-extended.csv")
            cases = (
                ("disabled", None, plain_log),
                ("relative", "relative-log.csv", directory / "relative-log.csv"),
                ("enabled", str(plain_log), plain_log),
                ("enabled", str(unicode_log), unicode_log),
                ("enabled", str(raw_long_log), Path("\\\\?\\" + str(raw_long_log))),
                ("enabled", str(extended_log), extended_log),
            )
            for mode, setting, log in cases:
                case_environment = environment.copy()
                case_environment.pop("PICTVIEW_PERF_LOG", None)
                if setting is not None:
                    case_environment["PICTVIEW_PERF_LOG"] = setting
                result = run([str(executable), mode, str(log)], cwd=directory,
                             env=case_environment, timeout=15)
                if result.returncode:
                    return result.returncode
                if mode == "enabled":
                    check_csv(log)
                else:
                    assert not log.exists(), "Disabled or relative-path logging must create no file"
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
