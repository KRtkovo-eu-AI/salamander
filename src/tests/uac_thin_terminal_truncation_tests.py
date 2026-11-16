# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-14 | Test both actual worker cleanup branches. | Preserve scoped data after terminal session loss. | #731 | GPT-6 |
"""Run extracted cleanup conditions on real isolated files, not full F5."""
import argparse
import hashlib
from pathlib import Path
import re
import shutil
import tempfile
from safe_file_paths_tests import ROOT, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    args = parser.parse_args()
    worker = ROOT / 'src/worker.cpp'
    before = hashlib.sha256(worker.read_bytes()).hexdigest()
    text = worker.read_text(encoding='utf-8-sig')
    branches = []
    for label, end_label in (('COPY_ERROR:', 'if (skipCopy)'), ('SKIP_COPY:', 'const bool discarded')):
        start = text.index(label, text.index('DoCopyFileLoopAsync(asyncPar, in, out,'))
        section = text[start:text.index(end_label, start)]
        pattern = r'if \(wholeFileAllocated\s*#ifdef SAM_UAC_THIN_COPY_TEST\s*&& \(!thin\.terminal \|\| thin\.NativeTarget\(\)\)\s*#endif\s*\)\s*SetEndOfFile\(out\);'
        found = re.findall(pattern, section)
        if len(found) != 1:
            raise RuntimeError('cleanup branch changed: ' + label)
        branches.append(found[0])
    template = (ROOT / 'src/tests/uac_thin_terminal_truncation_tests.cpp.in').read_text(encoding='utf-8')
    source = template.replace('@@ERROR_BRANCH@@', branches[0]).replace('@@SKIP_BRANCH@@', branches[1])
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    if not compiler:
        raise RuntimeError('MSVC cl.exe unavailable')
    flags = ('/MTd', '/Od', '/D_DEBUG') if args.configuration == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-terminal-truncation-', dir=ROOT.parent / 'builds') as temp:
        temp = Path(temp)
        cpp = temp / 'test.cpp'
        cpp.write_text(source, encoding='utf-8')
        for enabled in (False, True):
            exe = temp / ('thin.exe' if enabled else 'native.exe')
            command = [compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags]
            if enabled:
                command.append('/DSAM_UAC_THIN_COPY_TEST')
            built = run(command + [str(cpp), '/Fe:' + str(exe)], cwd=temp, env=env, timeout=60)
            if built.returncode:
                return built.returncode
            result = run([str(exe), str(temp / 'test-東.bin')], cwd=temp, env=env, timeout=20)
            if result.returncode:
                return result.returncode
    if hashlib.sha256(worker.read_bytes()).hexdigest() != before:
        raise RuntimeError('worker changed during test')
    print('Extracted worker branches; real files; synthetic session state. Not full F5 or UAC.')
    print('worker.cpp SHA-256:', before)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
