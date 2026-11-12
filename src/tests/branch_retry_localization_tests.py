# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Retry denied Branch listings after scoped consent. | Keep partial results and verify listed directory identities. | #731 | GPT-6.1 Terra |
"""Check the exact Branch omission diagnostics in every shipped locale."""
import re
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
names = {'IDS_BRANCH_IDENTITY_UNAVAILABLE': 14716, 'IDS_BRANCH_LIST_LIMIT': 14717}
locales = {'chinesesimplified', 'czech', 'dutch', 'french', 'german', 'hungarian', 'italian', 'romanian', 'russian', 'slovak', 'spanish'}
header = (ROOT / 'src/texts.rh2').read_text(encoding='utf-8-sig')
resources = (ROOT / 'src/lang/texts.rc2').read_text(encoding='utf-8-sig')
for name, number in names.items():
    assert re.findall(r'^#define\s+(\w+)\s+' + str(number) + r'\s*$', header, re.M) == [name]
    assert len(re.findall(r'^\s*' + name + r',\s*"[^"\r\n]+"\s*$', resources, re.M)) == 1
paths = {p.parent.name: p for p in (ROOT / 'translations').glob('*/salamand.slt')}
assert set(paths) == locales
for locale, path in paths.items():
    raw = path.read_bytes()
    assert raw.startswith(b'\xef\xbb\xbf[EXPORTINFO]') and raw.count(b'\n') == raw.count(b'\r\n'), locale
    text = raw.decode('utf-8-sig')
    headers = [int(n) for n in re.findall(r'^\[STRINGTABLE (\d+)\]$', text, re.M)]
    assert headers == list(range(len(headers))), locale
    section = text.split('[STRINGTABLE 198]', 1)[1].split('[STRINGTABLE 199]', 1)[0]
    for number in names.values():
        pattern = r'^' + str(number) + r',1,"([^"\r\n]+)"\s*$'
        assert len(re.findall(pattern, text, re.M)) == 1 and len(re.findall(pattern, section, re.M)) == 1, (locale, number)
print('PASS: two Branch diagnostics and all 11 locale resource blocks')
