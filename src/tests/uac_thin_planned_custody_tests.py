# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-13 | Test planned thin-session custody. | Preserve generation, ownership and fail-closed item cleanup. | #731 | GPT-6 |
"""Exercise selected actual custody branches with real Windows pipe handles."""
import argparse
import hashlib
from pathlib import Path
import shutil
import tempfile

from branch_panel_getters_tests import ROOT, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    config = parser.parse_args().configuration
    paths = [ROOT / 'src/worker.cpp', ROOT / 'src/uacbroker/uacbroker.cpp',
             ROOT / 'src/worker.h', ROOT / 'src/fileswn6.cpp']
    digests = [hashlib.sha256(path.read_bytes()).hexdigest() for path in paths]
    worker = read('src/worker.cpp')
    broker = read('src/uacbroker/uacbroker.cpp')
    header = read('src/worker.h')
    planner = read('src/fileswn6.cpp')
    if 'ThinAnchor = std::make_shared<ThinCopyPlanAnchor>();' not in header:
        raise RuntimeError('Plan anchor must be constructed before publishing the plan')
    construction = planner.index('try { script->CopySource.emplace(); }')
    if 'catch (const std::bad_alloc&) { TRACE_E(LOW_MEMORY); return FALSE; }' not in planner[construction:construction + 180]:
        raise RuntimeError('Plan anchor allocation must fail before dispatch')
    start = worker.index('    ~CThinCopyWorker()')
    end = worker.index('    bool Configure(', start)
    item = worker[start:end]
    op = worker.index('HANDLE CThinCopyWorker::Open(')
    start = worker.index('#ifdef SAM_UAC_THIN_PLANNED_PROTOTYPE', op)
    end = worker.index('#endif', start) + len('#endif')
    adoption = worker[start:end]
    start = broker.index('bool ThinCopySession::TakePlanned(')
    end = broker.index('\n#endif', start)
    custody = broker[start:end]
    template = (ROOT / 'src/tests/uac_thin_planned_custody_tests.cpp.in').read_text(encoding='utf-8')
    code = (template.replace('@@ITEM_METHODS@@', item).replace('@@ADOPTION@@', adoption)
            .replace('@@CUSTODY@@', custody))
    if '@@' in code:
        raise RuntimeError('Missing source replacement')
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    if not compiler:
        raise RuntimeError('MSVC cl.exe unavailable')
    flags = ('/MTd', '/Od', '/D_DEBUG') if config == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-thin-custody-', dir=ROOT.parent / 'builds') as temp:
        source = Path(temp) / 'test.cpp'
        source.write_text(code, encoding='utf-8')
        executable = Path(temp) / 'test.exe'
        built = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/W4', '/WX',
                     '/DSAM_UAC_THIN_COPY_TEST', '/DSAM_UAC_THIN_PLANNED_PROTOTYPE', *flags,
                     str(source), '/Fe:' + str(executable)], cwd=temp, env=env, timeout=60)
        if built.returncode:
            return built.returncode
        result = run([str(executable)], cwd=temp, env=env, timeout=20)
        if result.returncode:
            return result.returncode
    if [hashlib.sha256(path.read_bytes()).hexdigest() for path in paths] != digests:
        raise RuntimeError('Sources changed during test')
    for path, digest in zip(paths, digests):
        print(path.name + ' SHA-256:', digest)
    print('Actual custody/item branches with real pipe handles; job and broker outcomes are stubs.')
    print('Pre-dispatch anchor allocation and failure handling checked structurally.')
    print('Does not run the full DoCopyFile, authenticated broker, UAC, or VM.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
