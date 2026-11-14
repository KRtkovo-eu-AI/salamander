# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-13 | Exercise native transfer prompt selection in thin and ordinary builds. | Prevent misleading alternate-stream claims without changing existing operations. | #731 | GPT-6 |
"""Compile the actual dialog text-selection branch with deterministic request stubs."""
import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import tempfile

from safe_file_paths_tests import ROOT, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    dialog = ROOT / 'src/dialogs.cpp'
    original = dialog.read_bytes()
    text = original.decode('utf-8-sig').replace('\r\n', '\n')
    start = text.index('            const bool moving = prompt.request.IsMove();', text.index('case WORKER_DIALOG_UAC_TRANSFER:'))
    end = text.index('            const bool shareJob =', start)
    branch = text[start:end]
    template = (ROOT / 'src/tests/uac_thin_prompt_text_tests.cpp.in').read_text(encoding='utf-8')
    generated = template.replace('@PRODUCTION_SELECTION@', branch)
    if '@PRODUCTION_SELECTION@' in generated:
        raise RuntimeError('selection extraction failed')
    worker = (ROOT / 'src/worker.cpp').read_text(encoding='utf-8-sig')
    if worker.count('prompt.kind = UacF7::TransferPromptKind::ThinPlainCopy;') != 1:
        raise RuntimeError('thin prompt must be explicitly marked at exactly one call site')
    translations = sorted((ROOT / 'translations').glob('*/salamand.slt'))
    if len(translations) != 11:
        raise RuntimeError('unexpected translation set')
    for path in translations:
        lines = path.read_text(encoding='utf-8-sig').splitlines()
        matches = [line for line in lines if line.startswith('14721,1,')]
        if len(matches) != 1 or not matches[0].endswith('"'):
            raise RuntimeError('missing or ambiguous localized message: ' + str(path))
    english = (ROOT / 'src/lang/texts.rc2').read_text(encoding='utf-8-sig')
    if english.count('IDS_UAC_THIN_COPY_CONFIRM,') != 1:
        raise RuntimeError('missing or ambiguous default-language message')
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    if not compiler:
        raise RuntimeError('MSVC compiler not found')
    configs = ('Debug', 'Release') if args.configuration == 'Both' else (args.configuration,)
    with tempfile.TemporaryDirectory(prefix='uac-thin-prompt-') as temp:
        for config in configs:
            flags = ('/MTd', '/Od', '/D_DEBUG') if config == 'Debug' else ('/MT', '/O2', '/DNDEBUG')
            for thin in (False, True):
                stem = config + ('-thin' if thin else '-ordinary')
                cpp, exe = Path(temp) / (stem + '.cpp'), Path(temp) / (stem + '.exe')
                cpp.write_text(generated, encoding='utf-8')
                command = [compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX',
                           *flags, *(['/DSAM_UAC_THIN_COPY_TEST'] if thin else []),
                           str(cpp), '/Fe:' + str(exe)]
                print(stem, flush=True)
                result = run(command, cwd=temp, env=env, timeout=30)
                if result.returncode:
                    return result.returncode
                result = subprocess.run([str(exe)], cwd=temp, env=env, timeout=7)
                if result.returncode:
                    return result.returncode
    if hashlib.sha256(dialog.read_bytes()).digest() != hashlib.sha256(original).digest():
        raise RuntimeError('dialog changed while running the test')
    print('PASS: all 11 translations have exactly one message')
    print('NOTE: production selection branch with request stubs, not an interactive dialog or broker test.')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
