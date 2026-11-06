# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual ZIP extraction destination guards and metadata/failed-output I/O on owned NTFS fixtures."""
import argparse, shutil, tempfile
from pathlib import Path
from branch_panel_getters_tests import block, read, compiler_environment, run

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both')
    args=parser.parse_args();source=read('src/plugins/zip/extract.cpp');common=read('src/plugins/zip/common.cpp')
    text=read('src/tests/zip_extract_target_tests.cpp.in')
    text=text.replace('@@CONVERSIONS@@','\n'.join(block(common,m) for m in ('static std::wstring ZipPathAddExtendedPrefix(', 'static bool ZipBytesToWide(', 'std::wstring ZipPathToWide(')))
    text=text.replace('@@ACTUAL@@','\n'.join(block(source,m) for m in ('static DWORD ZipExtractTargetError(', 'static BOOL SetZipExtractAttributes(', 'static BOOL DeleteZipExtractOutput(')))
    assert '@@' not in text
    single=block(source,'int CZipUnpack::ExtractSingleFile(');whole=block(source,'int CZipUnpack::ExtractFiles(')
    assert single.index('ZipExtractTargetError(')<single.index('*(targetDir + targetDirLen++)')
    assert 'ZipExtractTargetError(tempDir, tempDirLen, fileInfo->Name, RootLen, NULL, Test)' in whole
    assert whole.index('strlen(targetDir) >= ZIP_MAX_PATH')<whole.index('lstrcpy(tempDir, targetDir)')
    assert 'Test ? ZIP_MAX_PATH : MAX_PATH' not in whole
    assert 'SafeCreateCFile(&OutputFile, targetDir' in single and 'Crc != fileInfo->Crc' in single
    assert 'DeleteZipExtractOutput(targetDir, true)' in single and 'DeleteZipExtractOutput(targetDir, false)' in single
    assert 'DeleteFile(targetDir)' not in single and 'SetFileAttributes(targetDir' not in single
    assert 'SalamanderSafeFile->SafeFileCreate(fileName' in block(source,'int CZipUnpack::SafeCreateCFile(')
    environment=compiler_environment();compiler=shutil.which('cl',path=environment.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='zip-extract-target-') as temp:
        directory=Path(temp);cpp=directory/'test.cpp';exe=directory/'test.exe';cpp.write_text(text,encoding='utf8')
        for configuration in ('Debug','Release') if args.configuration=='Both' else (args.configuration,):
            flags=['/MDd','/D_DEBUG','/Od'] if configuration=='Debug' else ['/MD','/DNDEBUG','/O2']
            print('Actual ZIP extraction target:',configuration,flush=True)
            for command in ([compiler,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX',*flags,str(cpp),'/Fe:'+str(exe)],[str(exe),str(directory/'owned-fixtures')]):
                result=run(command,cwd=directory,env=environment,timeout=40)
                if result.returncode:return result.returncode
    return 0
if __name__=='__main__':raise SystemExit(main())
