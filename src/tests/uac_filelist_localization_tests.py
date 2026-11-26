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
languages = {'arabic', 'bulgarian', 'chinesesimplified', 'chinesetraditional', 'croatian',
             'czech', 'dutch', 'french', 'german', 'greek', 'hebrew', 'hungarian',
             'indonesian', 'italian', 'japanese', 'korean', 'polish', 'portuguese',
             'romanian', 'russian', 'slovak', 'spanish', 'swedish', 'turkish', 'ukrainian', 'vietnamese'}
fallbacks = {'catalan', 'danish', 'finnish', 'lithuanian', 'norwegian', 'slovenian'}
found = {p.parent.name for p in (root / 'translations').glob('*/salamand.slt')}
assert found == languages | fallbacks, (found, languages | fallbacks)
for language in sorted(found):
    path = root / 'translations' / language / 'salamand.slt'
    raw = path.read_bytes()
    assert raw.startswith(b'\xef\xbb\xbf') and raw.count(b'\n') == raw.count(b'\r\n'), language
    text = raw.decode('utf-8-sig')
    matches = re.findall(r'^14715,([01]),"([^"\r\n]+)"\r?$', text, re.MULTILINE)
    assert len(matches) == 1 and matches[0][1].strip(), language
    if language in fallbacks:
        assert matches[0] == ('0', 'Access was denied. Create this file list as an administrator?'), language
    else:
        assert matches[0][0] == '1', language
print('PASS: unique Ctrl+M resource ID and 32 UTF-8 language catalogs')
