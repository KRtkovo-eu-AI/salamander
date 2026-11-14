// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-12 | Observe progress-style window destruction on a private desktop. | Verify window visibility during WM_DESTROY without touching the user's desktop. | #731 | GPT-6 |
#include <windows.h>
#include <objbase.h>
#include <cstdio>
#include <string>

struct Evidence { HDESK desktop=nullptr; bool created=false, before=false, shown=false, destroy=false, visibleDuringDestroy=false, isWindowDuringDestroy=false, testWindowForegroundDuringDestroy=false; DWORD error=0; };
static Evidence evidence;
static LRESULT CALLBACK procedure(HWND h,UINT msg,WPARAM wp,LPARAM lp) {
    if(msg==WM_DESTROY) {
        evidence.destroy=true;
        evidence.visibleDuringDestroy=IsWindowVisible(h)!=FALSE;
        evidence.isWindowDuringDestroy=IsWindow(h)!=FALSE;
        evidence.testWindowForegroundDuringDestroy=GetForegroundWindow()==h;
    }
    return DefWindowProcW(h,msg,wp,lp);
}
static DWORD WINAPI worker(void*) {
    if(!SetThreadDesktop(evidence.desktop)) { evidence.error=GetLastError(); return 1; }
    HINSTANCE instance=GetModuleHandleW(nullptr);
    WNDCLASSW cls{}; cls.lpfnWndProc=procedure; cls.hInstance=instance; cls.lpszClassName=L"SamandarinProgressDestroyProbe";
    if(!RegisterClassW(&cls)) { evidence.error=GetLastError(); return 1; }
    HWND w=CreateWindowExW(WS_EX_TOOLWINDOW,cls.lpszClassName,L"private progress probe",WS_OVERLAPPEDWINDOW,
                           20,20,240,120,nullptr,nullptr,instance,nullptr);
    if(!w) { evidence.error=GetLastError(); UnregisterClassW(cls.lpszClassName,instance); return 1; }
    evidence.created=true;
    evidence.before=IsWindowVisible(w)!=FALSE;
    ShowWindow(w,SW_SHOWNOACTIVATE);
    evidence.shown=IsWindowVisible(w)!=FALSE;
    if(!DestroyWindow(w)) { evidence.error=GetLastError(); UnregisterClassW(cls.lpszClassName,instance); return 1; }
    if(!UnregisterClassW(cls.lpszClassName,instance)) { evidence.error=GetLastError(); return 1; }
    return 0;
}
static bool desktopName(HDESK h,std::wstring& name) {
    DWORD bytes=0;
    GetUserObjectInformationW(h,UOI_NAME,nullptr,0,&bytes);
    if(!bytes) return false;
    std::wstring result((bytes+sizeof(wchar_t)-1)/sizeof(wchar_t),L'\0');
    if(!GetUserObjectInformationW(h,UOI_NAME,result.data(),bytes,&bytes)) return false;
    name.assign(result.c_str()); return true;
}
int main() {
    GUID id{}; wchar_t suffix[40]{};
    if(FAILED(CoCreateGuid(&id)) || !StringFromGUID2(id,suffix,static_cast<int>(sizeof(suffix)/sizeof(*suffix)))) { std::puts("FAIL: unique desktop name"); return 1; }
    const std::wstring name=std::wstring(L"Samandarin-focus-probe-")+suffix;
    evidence.desktop=CreateDesktopW(name.c_str(),nullptr,nullptr,0,DESKTOP_CREATEWINDOW|DESKTOP_READOBJECTS|DESKTOP_WRITEOBJECTS,nullptr);
    if(!evidence.desktop) { std::printf("SKIP: private desktop unavailable (%lu)\n",GetLastError()); return 77; }
    HDESK input=OpenInputDesktop(0,FALSE,DESKTOP_READOBJECTS);
    std::wstring ownName, inputName;
    if(!input || !desktopName(evidence.desktop,ownName) || !desktopName(input,inputName) || ownName==inputName) {
        const DWORD error=GetLastError(); if(input) CloseDesktop(input); CloseDesktop(evidence.desktop);
        std::printf("SKIP: cannot prove distinct input desktop (%lu)\n",error); return 77;
    }
    CloseDesktop(input);
    HANDLE thread=CreateThread(nullptr,0,worker,nullptr,0,nullptr);
    if(!thread) { const DWORD error=GetLastError(); CloseDesktop(evidence.desktop); std::printf("FAIL: private thread (%lu)\n",error); return 1; }
    DWORD wait=WaitForSingleObject(thread,10000), code=1;
    if(wait==WAIT_OBJECT_0) GetExitCodeThread(thread,&code);
    CloseHandle(thread);
    if(wait!=WAIT_OBJECT_0) { std::puts("FAIL: private thread timeout; desktop left for process cleanup"); return 1; }
    const BOOL closed=CloseDesktop(evidence.desktop);
    std::printf("created=%d visible_before_show=%d visible_after_show=%d wm_destroy=%d visible_during_destroy=%d is_window_during_destroy=%d test_window_foreground_during_destroy=%d close_desktop=%d worker_error=%lu\n",
      evidence.created,evidence.before,evidence.shown,evidence.destroy,evidence.visibleDuringDestroy,evidence.isWindowDuringDestroy,evidence.testWindowForegroundDuringDestroy,closed,evidence.error);
    if(code || !closed || !evidence.created || evidence.before || !evidence.shown || !evidence.destroy || !evidence.isWindowDuringDestroy) return 1;
    std::puts("PASS: observed WM_DESTROY on separate non-input desktop; no desktop switch and no activation requested.");
    std::puts("LIMIT: foreground behavior on the active user desktop and actual UAC are not measured.");
    return 0;
}
