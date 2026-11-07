# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Actual bounded localized inserts, hostile directives and pre-confirmation guard."""
import argparse
import json
import re
import shutil
import tempfile
from pathlib import Path
from branch_panel_getters_tests import block, read, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug', 'Release', 'Both'), default='Both')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    resources = {10137: 2, 10258: 1, 12200: 2, 12290: 1, 12406: 1}
    names = {'IDS_CANNOTREADDIR': 10137, 'IDS_CREATEDIRFAILED': 10258,
             'IDS_PATHERRORFORMAT': 12200, 'IDS_MOVECOPY_CREATEPATH': 12290,
             'IDS_PLUGINUNPACKERNOTFOUND': 12406}
    templates = []
    english = read('src/lang/texts.rc2')
    for name, resource_id in names.items():
        match = re.search(r'^\s*' + name + r', "(.*)"\s*$', english, re.M)
        assert match, name
        templates.append((name, match.group(1), resources[resource_id]))
    languages = sorted((root / 'translations').glob('*/salamand.slt'))
    assert len(languages) == 11, len(languages)
    for language in languages:
        text = language.read_text(encoding='utf-8-sig')
        for resource_id, count in resources.items():
            match = re.search(r'^' + str(resource_id) + r',[01],"(.*)"\s*$', text, re.M)
            assert match, (language, resource_id)
            templates.append((str(language.relative_to(root)) + ':' + str(resource_id), match.group(1), count))
    first = r'C:\東京\100%-literal-%n-%s.txt'
    second = 'error %n %s %p'
    golden = []
    for label, template, count in templates:
        tokens = re.findall(r'%(?:s|%)', template)
        assert re.sub(r'%(?:s|%)', '', template).find('%') == -1, label
        assert tokens.count('%s') == count, label
        values = iter((first, second))
        expected = ''.join(next(values) if token == '%s' else '%' if token == '%%' else token
                           for token in re.split(r'(%s|%%)', template))
        quote = lambda value: json.dumps(value, ensure_ascii=False)
        golden.append('{' + ','.join([quote(template), quote(first), quote(second), quote(expected), str(count)]) + '}')
    source = read('src/tests/localized_text_tests.cpp.in')
    header = '#include "' + (root / 'src/common/localizedtext.h').as_posix() + '"'
    guard = block(read('src/fileswn7.cpp'), 'if (!Salamander::LocalizedText::Write(textBuf,')
    source = source.replace('@@HEADER@@', header).replace('@@TRANSLATIONS@@', ',\n'.join(golden)).replace('@@GUARD@@', guard)
    assert '@@' not in source
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    with tempfile.TemporaryDirectory(prefix='localized-text-') as temp:
        directory = Path(temp)
        cpp, exe = directory / 'test.cpp', directory / 'test.exe'
        cpp.write_text(source, encoding='utf8')
        for config, flags in [('Debug', ['/MDd', '/D_DEBUG', '/Od', '/RTC1']), ('Release', ['/MD', '/DNDEBUG', '/O2'])]:
            if args.configuration not in ('Both', config):
                continue
            print('Actual localized string inserts:', config, flush=True)
            for command in ([compiler, '/nologo', '/std:c++17', '/EHsc', '/utf-8', '/W4', '/WX', *flags, str(cpp), '/Fe:' + str(exe)], [str(exe)]):
                result = run(command, cwd=directory, env=env, timeout=40)
                if result.returncode:
                    return result.returncode
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
