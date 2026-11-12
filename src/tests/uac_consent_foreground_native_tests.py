# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-12 | Exercise actual consent focus logic against isolated Win32 stubs. | Cover stale delivery, user input, and foreign foreground windows without changing the desktop. | #731 | GPT-6 |
"""Compile actual extracted foreground helpers; no desktop windows or real UAC."""
import argparse
import hashlib
from pathlib import Path
import shutil
import tempfile
from branch_panel_getters_tests import ROOT, compiler_environment, run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configuration', choices=('Debug','Release'), default='Debug')
    parser.add_argument('--source-root', type=Path, default=ROOT)
    args = parser.parse_args()
    broker = args.source_root / 'src/uacbroker/uacbroker.cpp'
    header = args.source_root / 'src/uacbroker/f7pilot.h'
    before = {p: hashlib.sha256(p.read_bytes()).hexdigest() for p in (broker, header)}
    cpp = broker.read_text(encoding='utf-8')
    h = header.read_text(encoding='utf-8')
    owned = cpp[cpp.index('bool IsOwnedWindow('):cpp.index('\nBOOL LaunchBrokerWithOwner(', cpp.index('bool IsOwnedWindow('))]
    decl = h[h.index('constexpr UINT restoreConsentForegroundMessage'):h.index('\nbool CanOfferElevation()', h.index('constexpr UINT restoreConsentForegroundMessage'))]
    methods = cpp[cpp.index('ConsentForeground::ConsentForeground('):cpp.index('\nbool CanOfferElevation()', cpp.index('ConsentForeground::ConsentForeground('))]
    template = (ROOT / 'src/tests/uac_consent_foreground_native_tests.cpp.in').read_text(encoding='utf-8')
    code = template.replace('@@IS_OWNED@@',owned).replace('@@CLASS@@',decl).replace('@@METHODS@@',methods)
    if '@@' in code:
        raise RuntimeError('unfilled source placeholder')
    env = compiler_environment()
    compiler = shutil.which('cl', path=env.get('PATH'))
    if not compiler:
        raise RuntimeError('MSVC cl.exe unavailable')
    flags = ('/MTd','/Od','/D_DEBUG') if args.configuration == 'Debug' else ('/MT','/O2','/DNDEBUG')
    with tempfile.TemporaryDirectory(prefix='uac-consent-focus-', dir=ROOT.parent / 'builds') as temp:
        source=Path(temp)/'focus.cpp'; exe=Path(temp)/'focus.exe'
        source.write_text(code,encoding='utf-8')
        built=run([compiler,'/nologo','/std:c++17','/EHsc','/W4','/WX',*flags,str(source),'/Fe:'+str(exe)],cwd=temp,env=env,timeout=60)
        if built.returncode: return built.returncode
        result=run([str(exe)],cwd=temp,env=env,timeout=20)
        if result.returncode: return result.returncode
    if any(hashlib.sha256(p.read_bytes()).hexdigest()!=before[p] for p in before):
        raise RuntimeError('production source changed during test')
    print('Actual extracted ConsentForeground / RestoreConsentForeground and IsOwnedWindow; Windows calls are deterministic stubs.')
    print('No desktop was touched. This does not verify real Windows foreground restrictions or UAC.')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
