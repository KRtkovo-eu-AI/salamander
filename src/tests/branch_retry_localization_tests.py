# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Verify partial-merge diagnostics. | Keep all eleven localizations consistent. | #731 | GPT-6.1 Terra |
# | 2026-11-12 | Retry denied Branch listings after scoped consent. | Keep partial results and verify listed directory identities. | #731 | GPT-6.1 Terra |
"""Check the exact Branch omission diagnostics in every shipped locale."""
import re
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
names = {'IDS_BRANCH_IDENTITY_UNAVAILABLE': 14716, 'IDS_BRANCH_LIST_LIMIT': 14717, 'IDS_BRANCH_MERGE_UNCERTAIN': 14719, 'IDS_BRANCH_MERGE_UNCERTAIN_DETAIL': 14720}
locales = {'arabic', 'bulgarian', 'chinesesimplified', 'chinesetraditional', 'croatian',
           'czech', 'dutch', 'french', 'german', 'greek', 'hebrew', 'hungarian',
           'indonesian', 'italian', 'japanese', 'korean', 'polish', 'portuguese',
           'romanian', 'russian', 'slovak', 'spanish', 'swedish', 'turkish', 'ukrainian', 'vietnamese'}
fallbacks = {'catalan', 'danish', 'finnish', 'lithuanian', 'norwegian', 'slovenian'}
header = (ROOT / 'src/texts.rh2').read_text(encoding='utf-8-sig')
resources = (ROOT / 'src/lang/texts.rc2').read_text(encoding='utf-8-sig')
english = {}
for name, number in names.items():
    assert re.findall(r'^#define\s+(\w+)\s+' + str(number) + r'\s*$', header, re.M) == [name]
    matches = re.findall(r'^\s*' + name + r',\s*"([^"\r\n]+)"\s*$', resources, re.M)
    assert len(matches) == 1
    english[number] = matches[0]
paths = {p.parent.name: p for p in (ROOT / 'translations').glob('*/salamand.slt')}
assert set(paths) == locales | fallbacks
for locale, path in paths.items():
    raw = path.read_bytes()
    assert raw.startswith(b'\xef\xbb\xbf[EXPORTINFO]') and raw.count(b'\n') == raw.count(b'\r\n'), locale
    text = raw.decode('utf-8-sig')
    headers = [int(n) for n in re.findall(r'^\[STRINGTABLE (\d+)\]$', text, re.M)]
    assert headers == list(range(len(headers))), locale
    section = text.split('[STRINGTABLE 198]', 1)[1].split('[STRINGTABLE 199]', 1)[0]
    for number in names.values():
        state = '0' if locale in fallbacks else '1'
        pattern = r'^' + str(number) + ',' + state + r',"([^"\r\n]+)"\s*$'
        matches = re.findall(pattern, text, re.M)
        assert len(matches) == 1 and len(re.findall(pattern, section, re.M)) == 1, (locale, number)
        assert matches[0].count('%I64u') == (1 if number == 14719 else 0), (locale, number)
        assert matches[0].count('%') == (1 if number == 14719 else 0), (locale, number)
        if locale in fallbacks:
            assert matches[0] == english[number], (locale, number)
print('PASS: four Branch diagnostics and all 32 locale resource blocks')
