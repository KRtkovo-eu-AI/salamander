# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Check private-memory sampling against the current process. | Distinguish missing and mismatched processes from real measurements. | #731 | GPT-6 |
"""Real self-process sample only; this is not a Samandarin memory test."""
import os
import sys
import uac_thin_process_memory as memory


def main():
    result = memory.sample(os.getpid(), sys.executable)
    assert result["status"] == "measured" and result["private_bytes"] > 0, result
    mismatch = memory.sample(os.getpid(), sys.executable + ".not-this-image")
    assert mismatch["status"] == "image_mismatch", mismatch
    invalid = memory.sample(0, sys.executable)
    assert invalid["status"] == "unavailable" and "private_bytes" not in invalid, invalid
    print("3 process-memory checks passed; self only, no host or broker was started")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
