# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Test production conflict selection and preflight OOM paths. | Verify coordinator lock release and lease consistency. | #731 | GPT-6 (Codex) |
"""Compile coordinator methods with isolated fault and path seams."""
import argparse
from pathlib import Path
import shutil
import tempfile

from branch_panel_getters_tests import block, compiler_environment, read, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release'), default='Debug')
    args = parser.parse_args()
    worker = read('src/worker.cpp')
    code = read('src/tests/uac_parallel_coordinator_oom_tests.cpp.in')
    methods = {
        '@@LOCK@@': ('class CScopedConflictLock', ';'),
        '@@PREPARED@@': ('BOOL COperations::IsPreparedParentDirectoryLocked(', ''),
        '@@ANCESTORS@@': ('BOOL COperations::AreConflictAncestorsPreparedLocked(', ''),
        '@@READY@@': ('BOOL COperations::IsConflictItemReadyLocked(', ''),
        '@@RETRACT@@': ('void COperations::RetractConflictSubtreeLocked(', ''),
        '@@GET_NEXT@@': ('BOOL COperations::GetNextConflictReadyItem(', ''),
        '@@LEASE_NEXT@@': ('BOOL COperations::LeaseNextParallelCopy(', ''),
        '@@FINISH@@': ('CConflictLeaseOutcome COperations::FinishParallelCopyLeaseLocked(', ''),
        '@@RELEASE@@': ('CConflictLeaseOutcome COperations::ReleaseParallelCopyLease(', ''),
        '@@REBLOCK@@': ('CConflictLeaseOutcome COperations::ReblockParallelCopyLease(', ''),
        '@@REVALIDATE@@': ('BOOL COperations::RevalidatePreparedConflictAncestors(', ''),
        '@@REQUEUE@@': ('void COperations::RequeueChangedConflictItem(', ''),
        '@@VALIDATE@@': ('BOOL COperations::ValidateParallelCopyLeaseForSideEffect(', ''),
        '@@PARALLEL_COPY@@': ('static BOOL ParallelCopyFileW(', ''),
        '@@PARALLEL_THREAD@@': ('static DWORD WINAPI ParallelCopyThreadProc(', ''),
    }
    for placeholder, (marker, trailing) in methods.items():
        extracted = block(worker, marker, trailing)
        if placeholder == '@@REVALIDATE@@':
            old = 'ancestors.push_back(parent);'
            assert extracted.count(old) == 1
            # Inject at the vector allocation point inside the actual locked scope.
            extracted = extracted.replace(old, '{ if (InjectAncestors) throw std::bad_alloc(); ancestors.push_back(parent); }')
        if placeholder == '@@LEASE_NEXT@@':
            extracted = extracted.replace('int startIndex)', 'int /*startIndex*/)')
        code = code.replace(placeholder, extracted)
    assert '@@' not in code
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    flags = ('/MTd', '/Od', '/D_DEBUG') if args.configuration == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='parallel-coordinator-oom-') as temp:
        folder = Path(temp)
        source, exe = folder / 'fixture.cpp', folder / 'fixture.exe'
        source.write_text(code, encoding='utf-8')
        result = run([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags,
                      str(source), '/Fe:' + str(exe)], cwd=folder, env=env, timeout=35)
        if result.returncode:
            return result.returncode
        return run([str(exe)], cwd=folder, env=env, timeout=25).returncode


if __name__ == '__main__':
    raise SystemExit(main())
