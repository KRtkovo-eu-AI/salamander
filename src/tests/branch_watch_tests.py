# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise actual watcher path preparation and notification ownership with real Win32 watches."""
import argparse,pathlib,runpy,tempfile,shutil
ROOT=pathlib.Path(__file__).resolve().parents[2]
h=runpy.run_path(str(ROOT/'src/tests/disk_selection_snapshot_tests.py'))
read,block,run=h['read'],h['block'],h['run']
def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--configuration',choices=('Debug','Release','Both'),default='Both');a=parser.parse_args()
 snooper=read('src/snooper.cpp');host=read('src/mainwnd3.cpp');wide=read('src/common/widepath.cpp')
 attach=block(snooper,'static bool AttachPanelInternal(')
 queue=host[host.index('    case WM_USER_DISPACHCHANGENOTIF:'):];start=queue.index('                    if (!ChangeNotifArray.empty())');end=queue.index('                    if (!ok)',start)
 pieces={'PATH':block(snooper,'struct PreparedWatchPath',';')+'\n'+block(snooper,'static PreparedWatchPath PrepareWatchPath('),
 'CONVERSIONS':'\n'.join(block(wide,s) for s in ('BOOL IsValidPathUtf8Text(', 'std::wstring SalMultiByteToWidePath(', 'std::string SalWideToMultiBytePath(')),
 'EXTENDED':block(read('src/branch_view.cpp'),'std::wstring ExtendedPath('),
 'OPEN_WATCH':attach[attach.index('        HANDLE handle ='):attach.index('));',attach.index('        HANDLE handle ='))+3],
 'QUEUE_TYPE':block(read('src/mainwnd.h'),'struct CChangeNotifData',';'),
 'POST':block(host,'void CMainWindow::PostChangeOnPathNotification('),'DRAIN':queue[start:end]}
 text=read('src/tests/branch_watch_tests.cpp.in')
 for k,v in pieces.items():text=text.replace('@@'+k+'@@',v)
 assert '@@' not in text
 env=h['compiler_environment']();cl=shutil.which('cl',path=env.get('PATH'))
 with tempfile.TemporaryDirectory(prefix='branch-watch-build-') as folder:
  folder=pathlib.Path(folder);src=folder/'test.cpp';exe=folder/'test.exe';src.write_text(text,encoding='utf-8')
  for cfg in ('Debug','Release') if a.configuration=='Both' else (a.configuration,):
   print('Actual watcher/notification methods:',cfg,flush=True)
   flags=['/MDd','/D_DEBUG','/Od'] if cfg=='Debug' else ['/MD','/DNDEBUG','/O2']
   for cmd in ([cl,'/nologo','/std:c++17','/EHsc','/utf-8','/W4','/WX','/wd4100','/wd4505',*flags,str(src),'/Fe:'+str(exe),'user32.lib'],[str(exe)]):
    r=run(cmd,cwd=folder,env=env)
    if r.returncode:return r.returncode
 return 0
if __name__=='__main__':raise SystemExit(main())
