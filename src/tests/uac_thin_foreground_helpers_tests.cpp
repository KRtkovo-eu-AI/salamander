// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-12 | Exercise cross-thread main-window handoff. | Guard timeouts, queue order and user focus changes. | #731 | GPT-6 |
// | 2026-11-12 | Exercise thin progress focus decisions without touching the desktop. | Guard foreground ownership, timeout and user input snapshots. | #731 | GPT-6 |
#include <windows.h>
#include <map>
#include <string>
#include <cstdio>
struct Window { DWORD pid=0, thread=0; HWND root=nullptr; bool visible=true, enabled=true, iconic=false; };
static std::map<HWND,Window> windows;
static DWORD currentPid=10, currentThread=1, inputTime=100;
static bool inputWorks=true;
static HWND foreground=nullptr;
static bool sendSucceeds=true, setSucceeds=true, recycleDuringTimeout=false;
static ULONG_PTR sentCookie=0;
static unsigned failPropertyCall=0, propertyCalls=0;
static unsigned sendCalls=0, setCalls=0;
static std::map<std::pair<HWND,std::wstring>,HANDLE> properties;
static HANDLE fakeGetProp(HWND h, LPCWSTR n) { auto i=properties.find({h,n}); return i==properties.end()?nullptr:i->second; }
static BOOL fakeSetProp(HWND h, LPCWSTR n, HANDLE v) { if (++propertyCalls==failPropertyCall) return FALSE; properties[{h,n}]=v; return TRUE; }
static HANDLE fakeRemoveProp(HWND h, LPCWSTR n) { auto v=fakeGetProp(h,n); properties.erase({h,n}); return v; }
static BOOL fakeSetForeground(HWND h) { ++setCalls; if (!setSucceeds) return FALSE; foreground=h; return TRUE; }
static LRESULT fakeSend(HWND,UINT,WPARAM,LPARAM,UINT,UINT,PDWORD_PTR);
static HWND window(unsigned n) { return reinterpret_cast<HWND>(static_cast<ULONG_PTR>(n)); }
static HWND fakeForeground() { return foreground; }
static BOOL fakeInput(PLASTINPUTINFO p) { if (!inputWorks) return FALSE; p->dwTime=inputTime; return TRUE; }
static DWORD fakeThread(HWND h, LPDWORD pid) { auto it=windows.find(h); if (pid) *pid=it==windows.end()?0:it->second.pid; return it==windows.end()?0:it->second.thread; }
static DWORD fakePid() { return currentPid; }
static DWORD fakeCurrentThread() { return currentThread; }
static HWND fakeAncestor(HWND h, UINT) { auto it=windows.find(h); return it==windows.end()?nullptr:(it->second.root?it->second.root:h); }
static BOOL fakeVisible(HWND h) { auto it=windows.find(h); return it!=windows.end() && it->second.visible; }
static BOOL fakeEnabled(HWND h) { auto it=windows.find(h); return it!=windows.end() && it->second.enabled; }
static BOOL fakeIconic(HWND h) { auto it=windows.find(h); return it!=windows.end() && it->second.iconic; }
#define SAM_UAC_THIN_COPY_TEST
#define GetForegroundWindow fakeForeground
#define GetLastInputInfo fakeInput
#define GetWindowThreadProcessId fakeThread
#define GetCurrentProcessId fakePid
#define GetCurrentThreadId fakeCurrentThread
#define GetAncestor fakeAncestor
#define IsWindowVisible fakeVisible
#define IsWindowEnabled fakeEnabled
#define IsIconic fakeIconic
#define SetForegroundWindow fakeSetForeground
#define SendMessageTimeoutW fakeSend
#define GetPropW fakeGetProp
#define SetPropW fakeSetProp
#define RemovePropW fakeRemoveProp
#include "uac_thin_foreground.h"
static LRESULT fakeSend(HWND target,UINT message,WPARAM progress,LPARAM input,UINT flags,UINT timeout,PDWORD_PTR result)
{
    ++sendCalls;
    sentCookie=static_cast<ULONG_PTR>(input);
    if (!sendSucceeds)
    {
        if (recycleDuringTimeout)
        {
            HWND recycled=reinterpret_cast<HWND>(progress);
            properties[{recycled,thinHandoffCookieProperty}]=reinterpret_cast<HANDLE>(9999);
            properties[{recycled,thinHandoffInputProperty}]=reinterpret_cast<HANDLE>(200);
        }
        return 0;
    }
    if (message!=thinMainForegroundHandoffMessage || flags!=(SMTO_ABORTIFHUNG|SMTO_ERRORONEXIT) || timeout!=750) return 0;
    const DWORD before=currentThread;
    currentThread=windows[target].thread;
    *result=ThinMainForegroundHandoff(target,reinterpret_cast<HWND>(progress),static_cast<ULONG_PTR>(input));
    currentThread=before;
    return 1;
}
static unsigned failures=0;
static void expect(bool ok,const char* label) { std::printf("%s: %s\n",ok?"PASS":"FAIL",label); if(!ok) ++failures; }
static void reset(HWND progress,HWND mainWindow) { windows.clear(); Window a; a.pid=10;a.thread=1; windows[progress]=a; windows[mainWindow]=a; foreground=progress;inputTime=100;inputWorks=true;currentThread=1;currentPid=10;sendSucceeds=true;setSucceeds=true;recycleDuringTimeout=false;sentCookie=0;sendCalls=0;setCalls=0;failPropertyCall=0;propertyCalls=0;properties.clear(); }
int main() {
    const HWND progress=window(1), mainWindow=window(2), other=window(3);
    reset(progress,mainWindow);
    expect(ThinProgressSnapshotMatches(progress,100),"unchanged foreground and input match");
    foreground=other; expect(!ThinProgressSnapshotMatches(progress,100),"changed foreground rejects snapshot");
    reset(progress,mainWindow); ++inputTime; expect(!ThinProgressSnapshotMatches(progress,100),"new input rejects snapshot");
    reset(progress,mainWindow); inputWorks=false; expect(!ThinProgressSnapshotMatches(progress,100),"unavailable last-input fails closed");
    reset(progress,mainWindow); expect(ThinProgressIsActive(progress,100),"own active progress matches");
    windows.erase(progress); expect(!ThinProgressIsActive(progress,100),"destroyed progress rejected");
    reset(progress,mainWindow); windows[progress].pid=99; expect(!ThinProgressIsActive(progress,100),"foreign process progress rejected");
    reset(progress,mainWindow); windows[progress].thread=2; expect(!ThinProgressIsActive(progress,100),"wrong owning thread rejected");
    reset(progress,mainWindow); windows[progress].visible=false; expect(!ThinProgressIsActive(progress,100),"hidden progress rejected");
    reset(progress,mainWindow); windows[progress].enabled=false; expect(!ThinProgressIsActive(progress,100),"disabled progress rejected");
    reset(progress,mainWindow); windows[progress].iconic=true; expect(!ThinProgressIsActive(progress,100),"minimized progress rejected");
    reset(progress,mainWindow); expect(ThinProgressReturnTarget(progress,mainWindow,true)==mainWindow,"active progress can return to own main window");
    expect(ThinProgressReturnTarget(progress,mainWindow,false)==nullptr,"without successful consent return no preferred target");
    foreground=other; expect(ThinProgressReturnTarget(progress,mainWindow,true)==nullptr,"intentional switch to another app is respected");
    reset(progress,mainWindow); windows[mainWindow].pid=99; expect(ThinProgressReturnTarget(progress,mainWindow,true)==nullptr,"foreign main window rejected");
    reset(progress,mainWindow); windows[mainWindow].root=other; expect(ThinProgressReturnTarget(progress,mainWindow,true)==nullptr,"non-root target rejected");
    reset(progress,mainWindow); windows[mainWindow].visible=false; expect(ThinProgressReturnTarget(progress,mainWindow,true)==nullptr,"hidden main rejected");
    reset(progress,mainWindow); windows[mainWindow].enabled=false; expect(ThinProgressReturnTarget(progress,mainWindow,true)==nullptr,"disabled main rejected");
    reset(progress,mainWindow); windows[mainWindow].iconic=true; expect(ThinProgressReturnTarget(progress,mainWindow,true)==nullptr,"minimized main rejected");
    reset(progress,mainWindow); windows.erase(mainWindow); expect(ThinProgressReturnTarget(progress,mainWindow,true)==nullptr,"destroyed main rejected");
    reset(progress,mainWindow); HWND target=ThinProgressReturnTarget(progress,mainWindow,true); ++inputTime;
    expect(target==mainWindow && !ThinProgressIsActive(progress,100),"input after target selection suppresses activation");
    reset(progress,mainWindow); target=ThinProgressReturnTarget(progress,mainWindow,true); foreground=other;
    expect(target==mainWindow && !ThinProgressIsActive(progress,100),"foreground switch after target selection suppresses activation");
    reset(progress,mainWindow); windows[mainWindow].thread=2;
    expect(ThinSendMainForegroundHandoff(progress,mainWindow,100,false) && foreground==mainWindow && sendCalls==1 && setCalls==1,
           "active progress synchronously hands activation to main thread");
    reset(progress,mainWindow); windows[mainWindow].thread=2;
    expect(!ThinSendMainForegroundHandoff(progress,mainWindow,100,true) && sendCalls==0 && setCalls==0,
           "queued operation takes priority over main handoff");
    reset(progress,mainWindow); windows[mainWindow].thread=2; failPropertyCall=1;
    expect(!ThinSendMainForegroundHandoff(progress,mainWindow,100,false) && sendCalls==0 && properties.empty(),
           "failed snapshot property prevents dispatch");
    reset(progress,mainWindow); windows[mainWindow].thread=2; failPropertyCall=2;
    expect(!ThinSendMainForegroundHandoff(progress,mainWindow,100,false) && sendCalls==0 && properties.empty(),
           "failed identity property removes snapshot and prevents dispatch");
    reset(progress,mainWindow); windows[mainWindow].thread=2; sendSucceeds=false;
    expect(!ThinSendMainForegroundHandoff(progress,mainWindow,100,false) && sendCalls==1 && setCalls==0,
           "timeout leaves activation unchanged and has no retry");
    currentThread=2;
    expect(!ThinMainForegroundHandoff(mainWindow,progress,sentCookie) && setCalls==0,
           "late timeout delivery refuses even a still-existing original HWND");
    windows.erase(progress);
    expect(!ThinMainForegroundHandoff(mainWindow,progress,sentCookie) && setCalls==0,
           "late timeout delivery refuses a destroyed progress window");
    reset(progress,mainWindow); windows[mainWindow].thread=2; sendSucceeds=false; recycleDuringTimeout=true;
    expect(!ThinSendMainForegroundHandoff(progress,mainWindow,100,false) &&
           reinterpret_cast<ULONG_PTR>(fakeGetProp(progress,thinHandoffCookieProperty))==9999 &&
           reinterpret_cast<ULONG_PTR>(fakeGetProp(progress,thinHandoffInputProperty))==200,
           "timeout cleanup preserves properties of a recycled HWND");
    currentThread=2;
    expect(!ThinMainForegroundHandoff(mainWindow,progress,sentCookie) && setCalls==0,
           "stale message cannot activate a recycled HWND with another cookie");
    reset(progress,mainWindow); windows[mainWindow].thread=2; currentThread=2; properties[{progress,thinHandoffCookieProperty}]=reinterpret_cast<HANDLE>(100); properties[{progress,thinHandoffInputProperty}]=reinterpret_cast<HANDLE>(100); ++inputTime;
    expect(!ThinMainForegroundHandoff(mainWindow,progress,100) && setCalls==0,
           "receiver rejects input after dispatch");
    reset(progress,mainWindow); windows[mainWindow].thread=2; currentThread=2; properties[{progress,thinHandoffCookieProperty}]=reinterpret_cast<HANDLE>(100); properties[{progress,thinHandoffInputProperty}]=reinterpret_cast<HANDLE>(100); foreground=other;
    expect(!ThinMainForegroundHandoff(mainWindow,progress,100) && setCalls==0,
           "receiver rejects foreground change after dispatch");
    reset(progress,mainWindow); windows[mainWindow].thread=2; currentThread=2; properties[{progress,thinHandoffCookieProperty}]=reinterpret_cast<HANDLE>(100); properties[{progress,thinHandoffInputProperty}]=reinterpret_cast<HANDLE>(100); windows[progress].pid=99;
    expect(!ThinMainForegroundHandoff(mainWindow,progress,100) && setCalls==0,
           "receiver rejects another process progress window");
    reset(progress,mainWindow); windows[mainWindow].thread=2; currentThread=2; properties[{progress,thinHandoffCookieProperty}]=reinterpret_cast<HANDLE>(100); properties[{progress,thinHandoffInputProperty}]=reinterpret_cast<HANDLE>(100); windows[mainWindow].pid=99;
    expect(!ThinMainForegroundHandoff(mainWindow,progress,100) && setCalls==0,
           "receiver rejects another process target window");
    reset(progress,mainWindow); windows[mainWindow].thread=2; currentThread=2; properties[{progress,thinHandoffCookieProperty}]=reinterpret_cast<HANDLE>(100); properties[{progress,thinHandoffInputProperty}]=reinterpret_cast<HANDLE>(100); windows[mainWindow].enabled=false;
    expect(!ThinMainForegroundHandoff(mainWindow,progress,100) && setCalls==0,
           "receiver respects disabled main window");
    reset(progress,mainWindow); windows[mainWindow].thread=2; currentThread=2; properties[{progress,thinHandoffCookieProperty}]=reinterpret_cast<HANDLE>(100); properties[{progress,thinHandoffInputProperty}]=reinterpret_cast<HANDLE>(100); windows[progress].visible=false;
    expect(!ThinMainForegroundHandoff(mainWindow,progress,100) && setCalls==0,
           "receiver refuses a hidden progress window");
    reset(progress,mainWindow); windows[mainWindow].thread=2; setSucceeds=false;
    expect(!ThinSendMainForegroundHandoff(progress,mainWindow,100,false) && foreground==progress && setCalls==1,
           "Windows activation refusal is not reported as success");
    std::printf("Failures: %u\n",failures); return failures?1:0;
}
