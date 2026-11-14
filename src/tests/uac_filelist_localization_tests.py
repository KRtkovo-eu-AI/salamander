# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Check the new Ctrl+M consent ID and its translations without compiling resources."""
import re
from pathlib import Path

root = Path(__file__).resolve().parents[2]
identifier = 'IDS_UAC_FILELIST_CREATE_CONFIRM'
header = (root / 'src/texts.rh2').read_text(encoding='utf-8-sig')
definitions = re.findall(r'^\s*#define\s+(\w+)\s+14715\s*$', header, re.MULTILINE)
assert definitions == [identifier], definitions
assert len(re.findall(r'^\s*#define\s+' + identifier + r'\s+', header, re.MULTILINE)) == 1
rc = (root / 'src/lang/texts.rc2').read_text(encoding='utf-8-sig')
assert rc.count(identifier) == 1
assert re.search(r'^\s*' + identifier + r',\s*"Access was denied\. Create this file list as an administrator\?"\s*$', rc, re.MULTILINE)
languages = {'czech', 'slovak', 'dutch', 'french', 'german', 'hungarian',
             'italian', 'romanian', 'russian', 'chinesesimplified', 'spanish'}
found = {p.parent.name for p in (root / 'translations').glob('*/salamand.slt')}
assert found == languages, (found, languages)
for language in sorted(languages):
    path = root / 'translations' / language / 'salamand.slt'
    raw = path.read_bytes()
    assert raw.startswith(b'\xef\xbb\xbf') and raw.count(b'\n') == raw.count(b'\r\n'), language
    text = raw.decode('utf-8-sig')
    matches = re.findall(r'^14715,1,"([^"\r\n]+)"\r?$', text, re.MULTILINE)
    assert len(matches) == 1 and matches[0].strip(), language
print('PASS: unique Ctrl+M resource ID and 11 UTF-8 translations')
