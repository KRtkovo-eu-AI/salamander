# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-11 | Exercise actual attribute-dialog dirty and shield code. | Reject edited attribute controls in the special-file time-only path. | #731 | GPT-6.1 Terra |
"""Compile the real attribute dialog's event, preview and transfer methods with UI seams."""
import argparse
import shutil
import tempfile
from pathlib import Path
from branch_panel_getters_tests import ROOT, block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    dialog = read('src/dialogs.cpp')
    pieces = {
        'HEADER': str(ROOT / 'src/uacbroker/f7pilot.h').replace('\\', '/'),
        'EDITED': block(read('src/dialogs.h'), '    bool AnyAttributeEdited() const'),
        'READ_TIME': block(dialog, 'static bool ReadAttributeDialogTime('),
        'SHIELD': block(dialog, 'void CChangeAttrDialog::UpdateElevationShield('),
        'TRANSFER': block(dialog, 'void CChangeAttrDialog::Transfer('),
        'PROC': block(dialog, 'CChangeAttrDialog::DialogProc('),
    }
    code = read('src/tests/metadata_dialog_dirty_tests.cpp.in')
    for marker, value in pieces.items():
        code = code.replace('@@' + marker + '@@', value)
    assert '@@' not in code
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='metadata-dialog-dirty-') as temp:
        directory = Path(temp)
        source = directory / 'test.cpp'
        source.write_text(code, encoding='utf-8')
        exe = directory / 'test.exe'
        for config, flags in [('Debug', ['/MDd', '/D_DEBUG', '/Od']), ('Release', ['/MD', '/DNDEBUG', '/O2'])]:
            if args.configuration not in ('Both', config):
                continue
            print('Actual metadata dialog dirty controller:', config, flush=True)
            for command in ([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                             str(source), '/Fe:' + str(exe), 'user32.lib'], [str(exe)]):
                result = run(command, cwd=directory, env=env, timeout=30)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
