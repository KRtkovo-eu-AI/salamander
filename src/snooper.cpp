// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include "plugins.h"
#include "fileswnd.h"
#include "mainwnd.h"
#include "snooper.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

struct WatchEntry
{
    std::string Key;                     // normalized (case-insensitive) key
    std::string Path;                    // path passed to FindFirstChangeNotification
    HANDLE ChangeHandle = INVALID_HANDLE_VALUE;
    HDEVNOTIFY DeviceNotification = NULL;
    CFilesWindow* DeviceNotificationOwner = NULL;
    std::vector<CFilesWindow*> Subscribers;
};

static std::map<std::string, WatchEntry*> WatchEntriesByPath;
static std::map<CFilesWindow*, WatchEntry*> WatchEntriesByPanel;
static std::vector<WatchEntry*> WatchEntrySlots;
static std::vector<HANDLE> WaitHandles;

HANDLE Thread = NULL;
HANDLE DataUsageMutex = NULL;       // kvuli arrayum s daty pro thread i proces
HANDLE RefreshFinishedEvent = NULL; // kvuli "PostMessage", ceka na zprac.
HANDLE WantDataEvent = NULL;        // hl. thread chce pristoupit ke spolecnym datum
HANDLE TerminateEvent = NULL;       // hl. thread chce terminovat thread cmuchala
HANDLE ContinueEvent = NULL;        // pomocne event pro synchronizaci
HANDLE BeginSuspendEvent = NULL;    // zacatek suspend modu
HANDLE EndSuspendEvent = NULL;      // konec suspend modu pro cmuchala
HANDLE SharesEvent = NULL;          // bude signaled pokud se zmeni LanMan Shares

int SnooperSuspended = 0;

CRITICAL_SECTION TimeCounterSection; // pro synchronizaci pristupu k MyTimeCounter
int MyTimeCounter = 0;               // aktualni cas

HANDLE SafeFindCloseThread = NULL;              // thread "safe handle killer"
TDirectArray<HANDLE> SafeFindCloseCNArr(10, 5); // bezpecne (netuhnouci) zavirani handlu change-notify
CRITICAL_SECTION SafeFindCloseCS;               // krit. sekce pro pristup do pole handlu
BOOL SafeFindCloseTerminate = FALSE;            // pro ukonceni threadu
HANDLE SafeFindCloseStart = NULL;               // "starter" threadu - je-li non-signaled, ceka
HANDLE SafeFindCloseFinished = NULL;            // signaled -> thread uz zavrel vsechny handly

struct PreparedWatchPath
{
    std::string Key;
    std::string Path;
};

static PreparedWatchPath PrepareWatchPath(const char* path)
{
    PreparedWatchPath prepared;
    const char* usePath = path;
    char pathCopy[3 * MAX_PATH];
    MakeCopyWithBackslashIfNeeded(usePath, pathCopy);
    prepared.Path.assign(usePath);
    prepared.Key = prepared.Path;
    if (!prepared.Key.empty())
        CharUpperBuffA(prepared.Key.data(), (DWORD)prepared.Key.length());
    return prepared;
}

static int FindWatchEntryIndex(const WatchEntry* entry)
{
    for (size_t i = 0; i < WatchEntrySlots.size(); ++i)
    {
        if (WatchEntrySlots[i] == entry)
            return (int)i;
    }
    return -1;
}

static void ResetDeviceNotification(WatchEntry* entry)
{
    if (entry->DeviceNotification != NULL)
    {
        UnregisterDeviceNotification(entry->DeviceNotification);
        entry->DeviceNotification = NULL;
    }
    if (entry->DeviceNotificationOwner != NULL)
    {
        entry->DeviceNotificationOwner->DeviceNotification = NULL;
        entry->DeviceNotificationOwner = NULL;
    }
}

static void EnsureDeviceNotification(WatchEntry* entry, CFilesWindow* win, BOOL registerDevNotification)
{
    if (entry == NULL || !registerDevNotification || win == NULL || win->HWindow == NULL)
        return;

    if (entry->DeviceNotificationOwner == win && entry->DeviceNotification != NULL)
    {
        win->DeviceNotification = entry->DeviceNotification;
        return;
    }

    ResetDeviceNotification(entry);

    DEV_BROADCAST_HANDLE dbh;
    memset(&dbh, 0, sizeof(dbh));
    dbh.dbch_size = sizeof(dbh);
    dbh.dbch_devicetype = DBT_DEVTYP_HANDLE;
    dbh.dbch_handle = entry->ChangeHandle;
    entry->DeviceNotification = RegisterDeviceNotificationA(win->HWindow, &dbh, DEVICE_NOTIFY_WINDOW_HANDLE);
    if (entry->DeviceNotification != NULL)
    {
        entry->DeviceNotificationOwner = win;
        win->DeviceNotification = entry->DeviceNotification;
    }
}

static void RemoveWatchEntryInternal(WatchEntry* entry, DWORD closeTimeout)
{
    if (entry == NULL)
        return;

    ResetDeviceNotification(entry);

    int index = FindWatchEntryIndex(entry);
    if (index >= 0)
    {
        WatchEntrySlots.erase(WatchEntrySlots.begin() + index);
        WaitHandles.erase(WaitHandles.begin() + index);
    }

    HANDLE handle = entry->ChangeHandle;
    entry->ChangeHandle = INVALID_HANDLE_VALUE;

    if (!entry->Key.empty())
        WatchEntriesByPath.erase(entry->Key);

    if (handle != INVALID_HANDLE_VALUE && handle != NULL)
    {
        HANDLES(EnterCriticalSection(&SafeFindCloseCS));
        SafeFindCloseCNArr.Add(handle);
        if (!SafeFindCloseCNArr.IsGood())
            SafeFindCloseCNArr.ResetState();
        HANDLES(LeaveCriticalSection(&SafeFindCloseCS));

        ResetEvent(SafeFindCloseFinished);
        SetEvent(SafeFindCloseStart);
        WaitForSingleObject(SafeFindCloseFinished, closeTimeout);
    }

    delete entry;
}

static bool AttachPanelInternal(CFilesWindow* win, const PreparedWatchPath& prepared, BOOL registerDevNotification)
{
    WatchEntry* entry = NULL;
    auto it = WatchEntriesByPath.find(prepared.Key);
    if (it != WatchEntriesByPath.end())
    {
        entry = it->second;
    }
    else
    {
        HANDLE handle = HANDLES_Q(FindFirstChangeNotification(prepared.Path.c_str(), FALSE,
                                                               FILE_NOTIFY_CHANGE_FILE_NAME |
                                                                   FILE_NOTIFY_CHANGE_DIR_NAME |
                                                                   FILE_NOTIFY_CHANGE_ATTRIBUTES |
                                                                   FILE_NOTIFY_CHANGE_SIZE |
                                                                   FILE_NOTIFY_CHANGE_LAST_WRITE |
                                                                   FILE_NOTIFY_CHANGE_CREATION |
                                                                   FILE_NOTIFY_CHANGE_SECURITY));
        if (handle == INVALID_HANDLE_VALUE)
            return false;

        entry = new WatchEntry();
        entry->Key = prepared.Key;
        entry->Path = prepared.Path;
        entry->ChangeHandle = handle;

        WatchEntriesByPath[entry->Key] = entry;
        WatchEntrySlots.push_back(entry);
        WaitHandles.push_back(handle);
    }

    if (std::find(entry->Subscribers.begin(), entry->Subscribers.end(), win) == entry->Subscribers.end())
        entry->Subscribers.push_back(win);

    WatchEntriesByPanel[win] = entry;
    win->SetAutomaticRefresh(TRUE);

    EnsureDeviceNotification(entry, win, registerDevNotification);

    return true;
}

static void DetachPanelInternal(CFilesWindow* win, DWORD closeTimeout, BOOL closeDevNotification)
{
    auto it = WatchEntriesByPanel.find(win);
    if (it == WatchEntriesByPanel.end())
    {
        if (closeDevNotification && win->DeviceNotification != NULL)
        {
            UnregisterDeviceNotification(win->DeviceNotification);
            win->DeviceNotification = NULL;
        }
        return;
    }

    WatchEntry* entry = it->second;
    WatchEntriesByPanel.erase(it);

    if (closeDevNotification && entry->DeviceNotificationOwner == win)
        ResetDeviceNotification(entry);
    win->DeviceNotification = NULL;

    entry->Subscribers.erase(std::remove(entry->Subscribers.begin(), entry->Subscribers.end(), win), entry->Subscribers.end());

    if (entry->Subscribers.empty())
        RemoveWatchEntryInternal(entry, closeTimeout);
}

static void NotifySubscribers(WatchEntry* entry)
{
    if (entry == NULL)
        return;

    HANDLES(EnterCriticalSection(&TimeCounterSection));
    for (CFilesWindow* subscriber : entry->Subscribers)
    {
        if (subscriber != NULL && subscriber->HWindow != NULL)
            PostMessage(subscriber->HWindow, WM_USER_REFRESH_DIR, TRUE, MyTimeCounter++);
    }
    HANDLES(LeaveCriticalSection(&TimeCounterSection));
}

static void RemoveWatchEntryDuringSuspend(size_t index, TDirectArray<HWND>& refreshPanels)
{
    if (index >= WatchEntrySlots.size())
        return;

    WatchEntry* entry = WatchEntrySlots[index];
    if (entry == NULL)
        return;

    ResetDeviceNotification(entry);

    HANDLE handle = WaitHandles[index];
    HANDLES(FindCloseChangeNotification(handle));

    for (CFilesWindow* subscriber : entry->Subscribers)
    {
        if (subscriber == NULL)
            continue;

        auto panelIt = WatchEntriesByPanel.find(subscriber);
        if (panelIt != WatchEntriesByPanel.end() && panelIt->second == entry)
            WatchEntriesByPanel.erase(panelIt);

        if (subscriber->DeviceNotification != NULL)
            subscriber->DeviceNotification = NULL;

        if (subscriber->HWindow != NULL)
            refreshPanels.Add(subscriber->HWindow);
    }

    if (!entry->Key.empty())
        WatchEntriesByPath.erase(entry->Key);

    WatchEntrySlots.erase(WatchEntrySlots.begin() + index);
    WaitHandles.erase(WaitHandles.begin() + index);

    delete entry;
}

DWORD WINAPI ThreadFindCloseChangeNotification(void* param);

void DoWantDataEvent()
{
    ReleaseMutex(DataUsageMutex);                  // uvolnime data pro hl. thread
    WaitForSingleObject(WantDataEvent, INFINITE);  // pockame az je zabere
    WaitForSingleObject(DataUsageMutex, INFINITE); // az skonci jsou opet nase
    SetEvent(ContinueEvent);                       // uz jsou nase, pustime dale hl. thread
}

unsigned ThreadSnooperBody(void* /*param*/) // nevolat funkce hl. threadu (ani TRACE) !!!
{
    CALL_STACK_MESSAGE1("ThreadSnooperBody()");
    SetThreadNameInVCAndTrace("Snooper");
    TRACE_I("Begin");

    DWORD res;
    HKEY sharesKey;
    res = HANDLES_Q(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                 "system\\currentcontrolset\\services\\lanmanserver\\shares",
                                 0, KEY_NOTIFY, &sharesKey));
    if (res != ERROR_SUCCESS)
    {
        sharesKey = NULL;
        TRACE_E("Unable to open key in registry (LanMan Shares). error: " << GetErrorText(res));
    }
    else // klic je o.k., nahodime notifikace (bez toho se uz RegNotifyChangeKeyValue nezavola)
    {
        if ((res = RegNotifyChangeKeyValue(sharesKey, TRUE, REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET, SharesEvent,
                                           TRUE)) != ERROR_SUCCESS)
        {
            TRACE_E("Unable to monitor registry (LanMan Shares). error: " << GetErrorText(res));
        }
    }

    if (WaitForSingleObject(DataUsageMutex, INFINITE) == WAIT_OBJECT_0)
    {
        SetEvent(ContinueEvent); // ted uz jsou data cmuchala, hl. thread muze pokracovat

        WatchEntrySlots.clear();
        WaitHandles.clear();
        WatchEntrySlots.push_back(NULL); // zakladni objekty, musi byt na zacatku !
        WatchEntrySlots.push_back(NULL);
        WatchEntrySlots.push_back(NULL);
        WatchEntrySlots.push_back(NULL);
        WaitHandles.push_back(WantDataEvent);
        WaitHandles.push_back(TerminateEvent);
        WaitHandles.push_back(BeginSuspendEvent);
        WaitHandles.push_back(SharesEvent);

        BOOL ignoreRefreshes = FALSE;        // TRUE = ignorovat refreshe (zmeny v adresarich), jinak fungujeme normalne
        DWORD ignoreRefreshesAbsTimeout = 0; // az bude (int)(GetTickCount() - ignoreRefreshesAbsTimeout) >= 0, prepneme ignoreRefreshes na FALSE
        BOOL notEnd = TRUE;
        while (notEnd)
        {
            int timeout = ignoreRefreshes ? (int)(ignoreRefreshesAbsTimeout - GetTickCount()) : INFINITE;
            if (ignoreRefreshes && timeout <= 0)
            {
                ignoreRefreshes = FALSE;
                ignoreRefreshesAbsTimeout = 0;
                timeout = INFINITE;
            }
            //      TRACE_I("Snooper is waiting for: " << (ignoreRefreshes ? std::min<DWORD>(4, (DWORD)WaitHandles.size()) : (DWORD)WaitHandles.size()) << " events");
            DWORD waitCount = (DWORD)WaitHandles.size();
            DWORD waitLimit = ignoreRefreshes ? std::min<DWORD>(4, waitCount) : waitCount;
            if (waitLimit == 0)
                waitLimit = 1;
            res = WaitForMultipleObjects(waitLimit, WaitHandles.data(), FALSE, timeout);
            CALL_STACK_MESSAGE2("ThreadSnooperBody::wait_satisfied: 0x%X", res);
            switch (res)
            {
            case WAIT_OBJECT_0:
                DoWantDataEvent();
                break; // WantDataEvent
            case WAIT_OBJECT_0 + 1:
                notEnd = FALSE;
                break;              // TerminateEvent
            case WAIT_OBJECT_0 + 2: // BeginSuspendMode
            {
                TRACE_I("Start suspend mode");

                SetEvent(ContinueEvent); // uz jsme v suspendu -> pustime dale hl. thread

                TDirectArray<HWND> refreshPanels(10, 5); // pro pripad smazani sledovaneho adresare

                WaitHandles[2] = EndSuspendEvent; // misto beginu ted end suspend modu

                BOOL setSharesEvent = FALSE; // TRUE => znovu nahodit sledovani registry
                BOOL suspendNotFinished = TRUE;
                while (suspendNotFinished) // pockame na konec suspend modu
                {                          // osetreni vseho krome zmen v adresarich
                    timeout = ignoreRefreshes ? (int)(ignoreRefreshesAbsTimeout - GetTickCount()) : INFINITE;
                    if (ignoreRefreshes && timeout <= 0)
                    {
                        ignoreRefreshes = FALSE;
                        ignoreRefreshesAbsTimeout = 0;
                        timeout = INFINITE;
                    }
                    DWORD suspendWaitCount = (DWORD)WaitHandles.size();
                    DWORD suspendWaitLimit = ignoreRefreshes ? std::min<DWORD>(4, suspendWaitCount) : suspendWaitCount;
                    if (suspendWaitLimit == 0)
                        suspendWaitLimit = 1;
                    res = WaitForMultipleObjects(suspendWaitLimit, WaitHandles.data(), FALSE, timeout);

                    CALL_STACK_MESSAGE2("ThreadSnooperBody::suspend_wait_satisfied: 0x%X", res);
                    switch (res)
                    {
                    case WAIT_OBJECT_0:
                        DoWantDataEvent();
                        break; // WantDataEvent
                    case WAIT_OBJECT_0 + 1:
                        suspendNotFinished = notEnd = FALSE;
                        break; // TerminateEvent
                    case WAIT_OBJECT_0 + 2:
                        suspendNotFinished = FALSE;
                        break;              // EndSuspendEvent
                    case WAIT_OBJECT_0 + 3: // SharesEvent
                    {
                        // obnovime shary + refreshneme prip. i panely (pomoci WM_USER_REFRESH_SHARES)
                        setSharesEvent = TRUE;
                        break;
                    }

                    case WAIT_TIMEOUT:
                        break; // ignorujeme (konec rezimu ignorovani zmen v adresarich)

                    default:
                    {
                        int index = res - WAIT_OBJECT_0;
                        if (index >= 4 && index < (int)WatchEntrySlots.size())
                            RemoveWatchEntryDuringSuspend((size_t)index, refreshPanels);
                        else
                            TRACE_E("Unexpected value returned from WaitForMultipleObjects(): " << res);
                        break;
                    }
                    }
                }
                SetEvent(ContinueEvent); // uz nejsme suspendu -> pustime dale hl. thread

                if (setSharesEvent) // budeme sledovat dalsi zmeny v registry
                {
                    if (MainWindowCS.LockIfNotClosed())
                    {
                        if (MainWindow != NULL)
                            PostMessage(MainWindow->HWindow, WM_USER_REFRESH_SHARES, 0, 0);
                        MainWindowCS.Unlock();
                    }
                    if ((res = RegNotifyChangeKeyValue(sharesKey, TRUE, REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET, SharesEvent,
                                                       TRUE)) != ERROR_SUCCESS)
                    {
                        TRACE_E("Unable to monitor registry (LanMan Shares). error: " << GetErrorText(res));
                    }
                }

                WaitHandles[2] = BeginSuspendEvent;
                TRACE_I("End suspend mode");

                CALL_STACK_MESSAGE1("ThreadSnooperBody::post_refresh");

                HANDLES(EnterCriticalSection(&TimeCounterSection));
                // refreshneme zmenene panely
                int i;
                for (i = 0; i < refreshPanels.Count; i++)
                {
                    HWND wnd = refreshPanels[i];
                    if (IsWindow(wnd))
                    {
                        PostMessage(wnd, WM_USER_S_REFRESH_DIR, FALSE, MyTimeCounter++);
                    }
                }
                HANDLES(LeaveCriticalSection(&TimeCounterSection));
                // jeste posleme notifikaci o ukonceni suspend modu
                if (MainWindowCS.LockIfNotClosed())
                {
                    if (MainWindow != NULL && MainWindow->LeftPanel != NULL && MainWindow->RightPanel != NULL)
                    {
                        PostMessage(MainWindow->LeftPanel->HWindow, WM_USER_SM_END_NOTIFY, 0, 0);
                        PostMessage(MainWindow->RightPanel->HWindow, WM_USER_SM_END_NOTIFY, 0, 0);
                    }
                    MainWindowCS.Unlock();
                }

                if (refreshPanels.Count > 0)
                {
                    // dame si prestavku, aby se nezahltil system
                    ignoreRefreshes = TRUE;
                    ignoreRefreshesAbsTimeout = GetTickCount() + REFRESH_PAUSE;
                }
                break;
            }

            case WAIT_OBJECT_0 + 3: // SharesEvent
            {                       // nechame refreshout panely
                if (MainWindowCS.LockIfNotClosed())
                {
                    if (MainWindow != NULL)
                        PostMessage(MainWindow->HWindow, WM_USER_REFRESH_SHARES, 0, 0);
                    MainWindowCS.Unlock();
                }
                // budeme sledovat dalsi zmeny v registry
                if ((res = RegNotifyChangeKeyValue(sharesKey, TRUE, REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET, SharesEvent,
                                                   TRUE)) != ERROR_SUCCESS)
                {
                    TRACE_E("Unable to monitor registry (LanMan Shares). error: " << GetErrorText(res));
                }
                break;
            }

            case WAIT_TIMEOUT:
                break; // ignorujeme (konec rezimu ignorovani zmen v adresarich)

            default:
            {
                int index;
                index = res - WAIT_OBJECT_0;
                if (index < 4 || index >= (int)WatchEntrySlots.size())
                {
                    DWORD err = GetLastError();
                    TRACE_E("Unexpected value returned from WaitForMultipleObjects(): " << res);
                    break; // pro pripad nejake jine hodnoty res
                }

                WatchEntry* entry = WatchEntrySlots[index];
                if (entry == NULL)
                    break;

                NotifySubscribers(entry);
                FindNextChangeNotification(WaitHandles[index]); // stornujem tuto zmenu
                                                                        // indexy se muzou zmenit...
                HANDLE objects[4];
                objects[0] = WantDataEvent;        // v refreshi se muzou menit data
                objects[1] = TerminateEvent;       // pro pripad konce bez stihnuti refreshe
                objects[2] = BeginSuspendEvent;    // pro pripad volani BeginSuspendMode pri refreshi
                objects[3] = RefreshFinishedEvent; // zprava od hl. threadu o ukonceni r.

                BOOL refreshNotFinished = TRUE;
                while (refreshNotFinished) // pockame na zpracovani
                {                          // osetreni vseho krome zmen v adresarich
                    res = WaitForMultipleObjects(4, objects, FALSE, INFINITE);

                    switch (res)
                    {
                    case WAIT_OBJECT_0 + 0:
                        DoWantDataEvent();
                        break;              // WantDataEvent
                    case WAIT_OBJECT_0 + 1: // TerminateEvent
                        refreshNotFinished = notEnd = FALSE;
                        break;
                    case WAIT_OBJECT_0 + 2: // BeginSuspendEvent
                        refreshNotFinished = FALSE;
                        SetEvent(BeginSuspendEvent);
                        break;
                    default:
                        refreshNotFinished = FALSE;
                        break; // RefreshFinishedEvent
                    }
                }

                // dame si prestavku, aby se nezahltil system
                ignoreRefreshes = TRUE;
                ignoreRefreshesAbsTimeout = GetTickCount() + REFRESH_PAUSE;

                break;
            }
            }
        }
        ReleaseMutex(DataUsageMutex);
    }
    if (sharesKey != NULL)
        HANDLES(RegCloseKey(sharesKey));
    TRACE_I("End");
    return 0;
}

unsigned ThreadSnooperEH(void* param)
{
#ifndef CALLSTK_DISABLE
    __try
    {
#endif // CALLSTK_DISABLE
        return ThreadSnooperBody(param);
#ifndef CALLSTK_DISABLE
    }
    __except (CCallStack::HandleException(GetExceptionInformation()))
    {
        TRACE_I("Thread Snooper: calling ExitProcess(1).");
        //    ExitProcess(1);
        TerminateProcess(GetCurrentProcess(), 1); // tvrdsi exit (tenhle jeste neco vola)
        return 1;
    }
#endif // CALLSTK_DISABLE
}

DWORD WINAPI ThreadSnooper(void* param)
{
#ifndef CALLSTK_DISABLE
    CCallStack stack;
#endif // CALLSTK_DISABLE
    return ThreadSnooperEH(param);
}

BOOL InitializeThread()
{
    //---  vytvoreni eventu a mutexu pro synchronizaci
    DataUsageMutex = HANDLES(CreateMutex(NULL, FALSE, NULL));
    if (DataUsageMutex == NULL)
    {
        TRACE_E("Unable to create DataUsageMutex mutex.");
        return FALSE;
    }
    WantDataEvent = HANDLES(CreateEvent(NULL, FALSE, FALSE, NULL));
    if (WantDataEvent == NULL)
    {
        TRACE_E("Unable to create WantDataEvent event.");
        return FALSE;
    }
    ContinueEvent = HANDLES(CreateEvent(NULL, FALSE, FALSE, NULL));
    if (ContinueEvent == NULL)
    {
        TRACE_E("Unable to create ContinueEvent event.");
        return FALSE;
    }
    RefreshFinishedEvent = HANDLES(CreateEvent(NULL, FALSE, FALSE, NULL));
    if (RefreshFinishedEvent == NULL)
    {
        TRACE_E("Unable to create RefreshFinishedEvent event.");
        return FALSE;
    }
    TerminateEvent = HANDLES(CreateEvent(NULL, FALSE, FALSE, NULL));
    if (TerminateEvent == NULL)
    {
        TRACE_E("Unable to create TerminateEvent event.");
        return FALSE;
    }
    BeginSuspendEvent = HANDLES(CreateEvent(NULL, FALSE, FALSE, NULL));
    if (BeginSuspendEvent == NULL)
    {
        TRACE_E("Unable to create BeginSuspendEvent event.");
        return FALSE;
    }
    EndSuspendEvent = HANDLES(CreateEvent(NULL, FALSE, FALSE, NULL));
    if (EndSuspendEvent == NULL)
    {
        TRACE_E("Unable to create EndSuspendEvent event.");
        return FALSE;
    }
    SharesEvent = HANDLES(CreateEvent(NULL, FALSE, FALSE, NULL));
    if (SharesEvent == NULL)
    {
        TRACE_E("Unable to create SharesEvent event.");
        return FALSE;
    }

    // event "starteru" pro thread "safe handle killer"
    SafeFindCloseStart = HANDLES(CreateEvent(NULL, FALSE, FALSE, NULL));
    if (SafeFindCloseStart == NULL)
    {
        TRACE_E("Unable to create SafeFindCloseStart event.");
        return FALSE;
    }
    SafeFindCloseFinished = HANDLES(CreateEvent(NULL, FALSE, FALSE, NULL));
    if (SafeFindCloseFinished == NULL)
    {
        TRACE_E("Unable to create SafeFindCloseFinished event.");
        return FALSE;
    }

    HANDLES(InitializeCriticalSection(&TimeCounterSection));
    //---  start threadu cmuchala
    DWORD ThreadID;
    Thread = HANDLES(CreateThread(NULL, 0, ThreadSnooper, NULL, 0, &ThreadID));
    if (Thread == NULL)
    {
        TRACE_E("Unable to start Snooper thread.");
        return FALSE;
    }
    //  SetThreadPriority(Thread, THREAD_PRIORITY_LOWEST);
    WaitForSingleObject(ContinueEvent, INFINITE); // pockame na zabrani dat cmuchalem

    HANDLES(InitializeCriticalSection(&SafeFindCloseCS));
    //---  start threadu "safe handle killer"
    SafeFindCloseThread = HANDLES(CreateThread(NULL, 0, ThreadFindCloseChangeNotification, NULL, 0, &ThreadID));
    if (SafeFindCloseThread == NULL)
    {
        TRACE_E("Unable to start safe-handle-killer thread.");
        return FALSE;
    }
    // nutne zvyseni priority aby bezel pred hl. threadem (hl. thread
    // potrebuje mit zavrene handly okamzite, pri chybe nedochazi k aktivnimu cekani, pohoda)
    SetThreadPriority(SafeFindCloseThread, THREAD_PRIORITY_HIGHEST);

    return TRUE;
}

void TerminateThread()
{
    if (Thread != NULL) // terminovani threadu cmuchala
    {
        SetEvent(TerminateEvent);              // pozadame cmuchala o ukonceni cinnosti
        WaitForSingleObject(Thread, INFINITE); // pockame az chcipne
        HANDLES(CloseHandle(Thread));          // zavreme handle threadu
    }
    if (DataUsageMutex != NULL)
        HANDLES(CloseHandle(DataUsageMutex));
    if (RefreshFinishedEvent != NULL)
        HANDLES(CloseHandle(RefreshFinishedEvent));
    if (WantDataEvent != NULL)
        HANDLES(CloseHandle(WantDataEvent));
    if (ContinueEvent != NULL)
        HANDLES(CloseHandle(ContinueEvent));
    if (TerminateEvent != NULL)
        HANDLES(CloseHandle(TerminateEvent));
    if (BeginSuspendEvent != NULL)
        HANDLES(CloseHandle(BeginSuspendEvent));
    if (EndSuspendEvent != NULL)
        HANDLES(CloseHandle(EndSuspendEvent));
    if (SharesEvent != NULL)
        HANDLES(CloseHandle(SharesEvent));
    HANDLES(DeleteCriticalSection(&TimeCounterSection));

    if (SafeFindCloseThread != NULL)
    {
        SafeFindCloseTerminate = TRUE; // zakilujeme thread
        SetEvent(SafeFindCloseStart);
        if (WaitForSingleObject(SafeFindCloseThread, 1000) == WAIT_TIMEOUT) // pockame az se ukonci
        {
            TerminateThread(SafeFindCloseThread, 666);          // nepovedlo se, zabijeme ho natvrdo
            WaitForSingleObject(SafeFindCloseThread, INFINITE); // pockame az thread skutecne skonci, nekdy mu to dost trva
        }
        HANDLES(CloseHandle(SafeFindCloseThread));
    }
    if (SafeFindCloseStart != NULL)
        HANDLES(CloseHandle(SafeFindCloseStart));
    if (SafeFindCloseFinished != NULL)
        HANDLES(CloseHandle(SafeFindCloseFinished));
    HANDLES(DeleteCriticalSection(&SafeFindCloseCS));
}

void AddDirectory(CFilesWindow* win, const char* path, BOOL registerDevNotification)
{
    CALL_STACK_MESSAGE3("AddDirectory(, %s, %d)", path, registerDevNotification);
    SetEvent(WantDataEvent);                       // pozadame cmuchala o uvolneni DataUsageMutexu
    WaitForSingleObject(DataUsageMutex, INFINITE); // pockame na nej
    SetEvent(WantDataEvent);                       // cmuchal uz zase muze zacit cekat na DataUsageMutex
                                                   //---  ted uz jsou data hl. threadu, cmuchal ceka
    PreparedWatchPath prepared = PrepareWatchPath(path);

    bool attached = false;
    auto panelIt = WatchEntriesByPanel.find(win);
    if (panelIt != WatchEntriesByPanel.end())
    {
        WatchEntry* current = panelIt->second;
        if (current != NULL && current->Key == prepared.Key)
        {
            attached = true;
            EnsureDeviceNotification(current, win, registerDevNotification);
        }
        else
        {
            DetachPanelInternal(win, 200, TRUE);
        }
    }

    if (!attached)
    {
        if (!AttachPanelInternal(win, prepared, registerDevNotification))
        {
            win->SetAutomaticRefresh(FALSE);
            TRACE_W("Unable to receive change notifications for directory '" << prepared.Path << "' (auto-refresh will not work).");
        }
    }
    //---
    ReleaseMutex(DataUsageMutex);                 // uvolnime cmuchalovi DataUsageMutex
    WaitForSingleObject(ContinueEvent, INFINITE); // a pockame az si ho zabere
}

// thread, ve kterem provedeme zavreni handlu na "odpojene" sitove zarizeni (dlouhe cekani)
unsigned ThreadFindCloseChangeNotificationBody(void* param)
{
    CALL_STACK_MESSAGE1("ThreadFindCloseChangeNotificationBody()");
    SetThreadNameInVCAndTrace("SafeHandleKiller");
    //  TRACE_I("Begin");

    while (!SafeFindCloseTerminate)
    {
        WaitForSingleObject(SafeFindCloseStart, INFINITE); // cekame na odstartovani nebo ukonceni

        while (1)
        {
            // vyzvedneme handle
            HANDLES(EnterCriticalSection(&SafeFindCloseCS));
            HANDLE h;
            BOOL br = FALSE;

            if (SafeFindCloseCNArr.IsGood() && SafeFindCloseCNArr.Count > 0)
            {
                h = SafeFindCloseCNArr[SafeFindCloseCNArr.Count - 1];
                SafeFindCloseCNArr.Delete(SafeFindCloseCNArr.Count - 1);
                if (!SafeFindCloseCNArr.IsGood())
                    SafeFindCloseCNArr.ResetState(); // nemuze se nepovest, hlasi jen nedostatek pameti pro zmenseni pole
            }
            else
                br = TRUE;
            HANDLES(LeaveCriticalSection(&SafeFindCloseCS));

            if (br)
                break; // uz neni co zavirat, pockame na dalsi start

            // zavreme handle
            //      TRACE_I("Killing ... " << h);
            HANDLES(FindCloseChangeNotification(h));
        }

        SetEvent(SafeFindCloseFinished); // pustime hl. thread dale ...
    }
    //  TRACE_I("End");
    return 0;
}

unsigned ThreadFindCloseChangeNotificationEH(void* param)
{
#ifndef CALLSTK_DISABLE
    __try
    {
#endif // CALLSTK_DISABLE
        return ThreadFindCloseChangeNotificationBody(param);
#ifndef CALLSTK_DISABLE
    }
    __except (CCallStack::HandleException(GetExceptionInformation()))
    {
        TRACE_I("Safe Handle Killer: calling ExitProcess(1).");
        //    ExitProcess(1);
        TerminateProcess(GetCurrentProcess(), 1); // tvrdsi exit (tenhle jeste neco vola)
        return 1;
    }
#endif // CALLSTK_DISABLE
}

DWORD WINAPI ThreadFindCloseChangeNotification(void* param)
{
#ifndef CALLSTK_DISABLE
    CCallStack stack;
#endif // CALLSTK_DISABLE
    return ThreadFindCloseChangeNotificationEH(param);
}

void ChangeDirectory(CFilesWindow* win, const char* newPath, BOOL registerDevNotification)
{
    CALL_STACK_MESSAGE3("ChangeDirectory(, %s, %d)", newPath, registerDevNotification);
    SetEvent(WantDataEvent);                       // pozadame cmuchala o uvolneni DataUsageMutexu
    WaitForSingleObject(DataUsageMutex, INFINITE); // pockame na nej
    SetEvent(WantDataEvent);                       // cmuchal uz zase muze zacit cekat na DataUsageMutex
    //---  ted uz jsou data hl. threadu, cmuchal ceka
    PreparedWatchPath prepared = PrepareWatchPath(newPath);

    bool attached = false;
    auto panelIt = WatchEntriesByPanel.find(win);
    if (panelIt != WatchEntriesByPanel.end())
    {
        WatchEntry* current = panelIt->second;
        if (current != NULL && current->Key == prepared.Key)
        {
            attached = true;
            EnsureDeviceNotification(current, win, registerDevNotification);
        }
        else
        {
            DetachPanelInternal(win, 200, TRUE);
        }
    }
    else
    {
        if (win->DeviceNotification != NULL)
        {
            UnregisterDeviceNotification(win->DeviceNotification);
            win->DeviceNotification = NULL;
        }
    }

    if (!attached)
    {
        if (!AttachPanelInternal(win, prepared, registerDevNotification))
        {
            win->SetAutomaticRefresh(FALSE);
            TRACE_W("Unable to receive change notifications for directory '" << prepared.Path << "' (auto-refresh will not work).");
        }
    }
    //---
    ReleaseMutex(DataUsageMutex);                 // uvolnime cmuchalovi DataUsageMutex
    WaitForSingleObject(ContinueEvent, INFINITE); // a pockame az si ho zabere
}

void DetachDirectory(CFilesWindow* win, BOOL waitForHandleClosure, BOOL closeDevNotifification)
{
    CALL_STACK_MESSAGE3("DetachDirectory(, %d, %d)", waitForHandleClosure, closeDevNotifification);
    SetEvent(WantDataEvent);                       // pozadame cmuchala o uvolneni DataUsageMutexu
    WaitForSingleObject(DataUsageMutex, INFINITE); // pockame na nej
    SetEvent(WantDataEvent);                       // cmuchal uz zase muze zacit cekat na DataUsageMutex
                                                   //---  ted uz jsou data hl. threadu, cmuchal ceka
    DWORD closeTimeout = waitForHandleClosure ? 5000 : 200;
    DetachPanelInternal(win, closeTimeout, closeDevNotifification);
    win->SetAutomaticRefresh(FALSE);
    //---
    ReleaseMutex(DataUsageMutex);                 // uvolnime cmuchalovi DataUsageMutex
    WaitForSingleObject(ContinueEvent, INFINITE); // a pockame az si ho zabere
}

void EnsureWatching(CFilesWindow* win, BOOL registerDevNotification)
{
    if (win == NULL || !win->GetMonitorChanges())
        return;

    const char* path = win->GetPath();
    if (path == NULL || path[0] == 0)
        return;

    CALL_STACK_MESSAGE2("EnsureWatching(%s)", path);

    SetEvent(WantDataEvent);
    WaitForSingleObject(DataUsageMutex, INFINITE);
    SetEvent(WantDataEvent);

    PreparedWatchPath prepared = PrepareWatchPath(path);
    bool attached = false;

    auto panelIt = WatchEntriesByPanel.find(win);
    if (panelIt != WatchEntriesByPanel.end())
    {
        WatchEntry* current = panelIt->second;
        if (current != NULL && current->Key == prepared.Key)
        {
            attached = true;
            EnsureDeviceNotification(current, win, registerDevNotification);
        }
        else
        {
            DetachPanelInternal(win, 200, TRUE);
        }
    }

    if (!attached)
    {
        if (!AttachPanelInternal(win, prepared, registerDevNotification))
            win->SetAutomaticRefresh(FALSE);
    }

    ReleaseMutex(DataUsageMutex);
    WaitForSingleObject(ContinueEvent, INFINITE);
}

/*
#define SUSPMODESTACKSIZE 50

class CSuspModeStack
{
  protected:
    DWORD CallerCalledFromArr[SUSPMODESTACKSIZE];  // pole navratovych adres funkci, odkud se volal BeginSuspendMode()
    DWORD CalledFromArr[SUSPMODESTACKSIZE];        // pole adres, odkud se volal BeginSuspendMode()
    int Count;                                     // pocet prvku v predchozich dvou polich
    int Ignored;                                   // pocet volani BeginSuspendMode(), ktere jsme museli ignorovat (prilis male SUSPMODESTACKSIZE -> pripadne zvetsit)

  public:
    CSuspModeStack() {Count = 0; Ignored = 0;}
    ~CSuspModeStack() {CheckIfEmpty(1);}  // jeden BeginSuspendMode() je OK: vola se pri deaktivaci hl. okna Salama (pred uzavrenim hl. okna)

    void Push(DWORD caller_called_from, DWORD called_from);
    void Pop(DWORD caller_called_from, DWORD called_from);
    void CheckIfEmpty(int checkLevel);
};

void
CSuspModeStack::Push(DWORD caller_called_from, DWORD called_from)
{
  if (Count < SUSPMODESTACKSIZE)
  {
    CallerCalledFromArr[Count] = caller_called_from;
    CalledFromArr[Count] = called_from;
    Count++;
  }
  else
  {
    Ignored++;
    TRACE_E("CSuspModeStack::Push(): you should increase SUSPMODESTACKSIZE! ignored=" << Ignored);
  }
}

void
CSuspModeStack::Pop(DWORD caller_called_from, DWORD called_from)
{
  if (Ignored == 0)
  {
    if (Count > 0)
    {
      Count--;
      if (CallerCalledFromArr[Count] != caller_called_from)
      {
        TRACE_E("CSuspModeStack::Pop(): strange situation: BeginCallerCalledFrom!=StopCallerCalledFrom - BeginCalledFrom,StopCalledFrom");
        TRACE_E("CSuspModeStack::Pop(): strange situation: 0x" << std::hex <<
                CallerCalledFromArr[Count] << "!=0x" << caller_called_from << " - 0x" <<
                CalledFromArr[Count] << ",0x" << called_from << std::dec);
      }
    }
    else TRACE_E("CSuspModeStack::Pop(): unexpected call!");
  }
  else Ignored--;
}

void
CSuspModeStack::CheckIfEmpty(int checkLevel)
{
  if (Count > checkLevel)
  {
    TRACE_E("CSuspModeStack::CheckIfEmpty(" << checkLevel << "): listing remaining BeginSuspendMode calls: CallerCalledFrom,CalledFrom");
    int i;
    for (i = 0; i < Count; i++)
    {
      TRACE_E("CSuspModeStack::CheckIfEmpty():: 0x" << std::hex <<
              CallerCalledFromArr[i] << ",0x" << CalledFromArr[i] << std::dec);
    }
  }
}

CSuspModeStack SuspModeStack;
*/

void BeginSuspendMode(BOOL debugDoNotTestCaller)
{
    /*
#ifdef _DEBUG     // testujeme, jestli se BeginSuspendMode() a EndSuspendMode() volaji ze stejne funkce (podle navratove adresy volajici funkce -> takze nepozna "chybu" pri volani z ruznych funkci, ktere se obe volaji ze stejne funkce)
  DWORD *register_ebp;
  __asm mov register_ebp, ebp
  DWORD called_from, caller_called_from;
  __try
  {
    called_from = *(DWORD*)((char*)register_ebp + 4);

pokud bude jeste nekdy potreba ozivit tenhle kod, vyuzit toho, ze lze nahradit (x86 i x64):
    called_from = *(DWORD_PTR *)_AddressOfReturnAddress();

    caller_called_from = *(DWORD*)((char*)(*register_ebp) + 4);
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    called_from = -1;
    caller_called_from = -1;
  }
  SuspModeStack.Push(debugDoNotTestCaller ? 0 : caller_called_from, called_from);
#endif // _DEBUG
*/

    if (SnooperSuspended == 0)
    {
        SetEvent(BeginSuspendEvent);
        WaitForSingleObject(ContinueEvent, INFINITE);
    }
    SnooperSuspended++;
}

//#ifdef _DEBUG
//void EndSuspendModeBody()
//#else // _DEBUG
void EndSuspendMode(BOOL debugDoNotTestCaller)
//#endif // _DEBUG
{
    CALL_STACK_MESSAGE1("EndSuspendMode()");

    if (SnooperSuspended < 1)
    {
        TRACE_E("Incorrect call to EndSuspendMode()");
        SnooperSuspended = 0; // nepouziva zase nekdo blbe CM_LEFTREFRESH, CM_RIGHTREFRESH nebo CM_ACTIVEREFRESH
    }
    else
    {
        if (SnooperSuspended == 1)
        {
            SetEvent(EndSuspendEvent);
            WaitForSingleObject(ContinueEvent, INFINITE);
        }
        SnooperSuspended--;
    }
}

/*
#ifdef _DEBUG     // testujeme, jestli se BeginSuspendMode() a EndSuspendMode() volaji ze stejne funkce (podle navratove adresy volajici funkce -> takze nepozna "chybu" pri volani z ruznych funkci, ktere se obe volaji ze stejne funkce)
void EndSuspendMode(BOOL debugDoNotTestCaller)
{
  DWORD *register_ebp;
  __asm mov register_ebp, ebp
  DWORD called_from, caller_called_from;
  __try
  {
    called_from = *(DWORD*)((char*)register_ebp + 4);

pokud bude jeste nekdy potreba ozivit tenhle kod, vyuzit toho, ze lze nahradit (x86 i x64):
    called_from = *(DWORD_PTR *)_AddressOfReturnAddress();

    caller_called_from = *(DWORD*)((char*)(*register_ebp) + 4);
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    called_from = -1;
    caller_called_from = -1;
  }
  SuspModeStack.Pop(debugDoNotTestCaller ? 0 : caller_called_from, called_from);

  EndSuspendModeBody();
}
#endif // _DEBUG
*/
