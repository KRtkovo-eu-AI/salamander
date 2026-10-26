# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual disk target snapshot with explicit tab-registry doubles; guard placement contracts."""
import argparse
from pathlib import Path
import shutil
import tempfile
from branch_panel_getters_tests import block, read, compiler_environment, run


def controller_contract():
    for unit in ('src/fileswn7.cpp', 'src/fileswn8.cpp'):
        assert '#include "fileactiondisktarget.h"' in read(unit), unit
    controller = block(read('src/fileswn8.cpp'), 'void CFilesWindow::FilesAction(')
    assert 'if (target != NULL && target->IsBranchView() &&' in controller
    assert '!CFilesActionDiskTarget::AllowsBranchTransfer(type, countSizeMode, copyToSelectedDirs, Is(ptDisk), target->Is(ptDisk))) return;' in controller
    assert controller.index('!CFilesActionDiskTarget::AllowsBranchTransfer(') < controller.index('diskTarget.Capture(')
    assert controller.index('diskTarget.Capture(MainWindow, target)') < controller.index('CheckPath(TRUE)')
    assert 'memcpy(path, diskTarget.DefaultPath.c_str(), diskTarget.DefaultPath.size() + 1);' in controller
    assert 'CheckPath(TRUE) == ERROR_SUCCESS && validateDiskTarget()' in controller
    assert 'BOOL res2 = validateDiskTarget();' in controller
    assert 'if (!validateDiskTarget() ||\n                                !BuildScriptMain(' in controller
    assert 'else if (res2)\n                    {\n                        res2 = BuildScriptMain(' in controller
    dialog = controller.index('res = (int)CCopyMoveMoreDialog(')
    parser = controller.index('if (ParsePath(')
    assert controller[dialog:parser].count('if (!validateDiskTarget())') >= 2
    parsed = controller.index('if (IsBranchView() && pathType != PATH_TYPE_WINDOWS)')
    assert 'if (!validateDiskTarget())' in controller[parser:parsed]
    late = controller.index('if (!validateDiskTarget()) cancel = TRUE;')
    assert controller.index('BOOL occupiedSpTooBig') < late < controller.index('!StartProgressDialog(')
    classify = controller.index('const bool useDefaultDiskTarget =')
    normalize = controller.index('if (!IsPluginFSPath(path)')
    preflight = controller.index('!diskTarget.MatchesParsedDefault(')
    split = controller.index('if (SalSplitWindowsPath(')
    packed = controller.index('!diskTarget.MatchesSplitDefault(')
    assert dialog < classify < normalize < parser < preflight < split < packed
    assert 'useDefaultDiskTarget &&' in controller[preflight-80:preflight]
    assert 'useDefaultDiskTarget &&' in controller[packed-80:packed]
    assert 'if (!validateDiskTarget())' in controller[split:packed]
    assert '#define PATH_TYPE_WINDOWS 1 ' in read('src/plugins/shared/spl_gen.h')
    print('Disk target controller placement: PASS (only disk Copy/Move Branch admission)', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug','Release','Both'), default='Both')
    args = parser.parse_args()
    controller_contract()
    wide = read('src/common/widepath.cpp')
    helpers = '\n'.join(block(wide, marker) for marker in (
        'BOOL IsValidPathUtf8Text(', 'std::wstring SalMultiByteToWidePath(',
        'std::string SalWideToMultiBytePath('))
    generated = read('src/tests/disk_target_identity_tests.cpp.in')
    generated = generated.replace('@@ACTION@@', block(read('src/fileswnd.h'), 'enum CActionType')+';')
    generated = generated.replace('@@WIDE@@', helpers).replace('@@SNAPSHOT@@',
        block(read('src/fileactiondisktarget.h'), 'struct CFilesActionDiskTarget')+';')
    assert '@@' not in generated
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='disk-target-identity-') as temp:
        directory = Path(temp)
        source = directory/'test.cpp'
        source.write_text(generated, encoding='utf8')
        executable = directory/'test.exe'
        for config, flags in [('Debug',['/MDd','/D_DEBUG','/Od']),('Release',['/MD','/DNDEBUG','/O2'])]:
            if args.configuration not in ('Both',config): continue
            print('Actual disk target identity:', config, flush=True)
            for command in ([compiler,'/nologo','/std:c++17','/EHsc','/utf-8','/J','/W4','/WX',
                             *flags,str(source),'/Fe:'+str(executable)], [str(executable)]):
                result = run(command, cwd=directory, env=env, timeout=40)
                if result.returncode: return result.returncode
    return 0


if __name__ == '__main__': raise SystemExit(main())
