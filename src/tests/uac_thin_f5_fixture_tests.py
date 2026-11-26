# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Validate real-host F5 fixture preparation without launching the host. | Prove paths and original test inputs are preserved separately from GUI acceptance. | #731 | GPT-6 |
"""Prepare own fixtures only; no application, broker or UAC is started."""
import os
from pathlib import Path
import uuid

import uac_thin_f5_gui as gui


def main():
    base = Path(r"C:\Samandarin\builds")
    payload = base / "uac-integrated-e5b43e00-20261112" / "Release" / "output" / "salamand.exe"
    template = base / "branch-view" / "investigation" / "default-config.reg"
    if not payload.is_file() or not template.is_file():
        raise RuntimeError("the known test-only payload or template is missing")
    payload_hash, template_hash = gui.digest(payload), gui.digest(template)
    outer = base / ("uac-thin-fixture-probe-" + uuid.uuid4().hex)
    outer.mkdir()
    for name, long_path, collision in (("normal", False, "none"), ("unicode-long", True, "none"),
                                       ("collision", False, "skip")):
        root = outer / name
        source, target, config, file, copied_exe = gui.prepare(root, template, payload, 1, long_path, collision)
        assert gui.digest(file) == gui.digest(source / "01-copy-東.bin")
        assert os.path.getsize(gui.windows_long(file)) == 1024 * 1024
        assert gui.digest(copied_exe) == payload_hash and gui.digest(config) == template_hash
        assert gui.exists(target / file.name) == (collision != "none")
        if collision != "none":
            assert gui.digest(target / file.name) != gui.digest(file)
        if long_path:
            assert len(str(file)) > 260 and len(str(target / file.name)) > 260
        bootstrap = (root / "app" / "configstorage.ini").read_text(encoding="ascii")
        assert "StorageType=RegFile" in bootstrap and "RegFilePath=" + str(config) in bootstrap
    assert gui.digest(payload) == payload_hash and gui.digest(template) == template_hash
    print("3 fixture shapes passed (normal, long Unicode, collision); no GUI/UAC/worker started:", outer)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
