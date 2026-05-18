// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// CommentsTranslationProject: TRANSLATED

#include "precomp.h"

#include "plugins.h"
#include "fileswnd.h"
#include "mainwnd.h"
#include "snooper.h"

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#ifdef min
#    undef min
#endif
#ifdef max
#    undef max
#endif

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
static std::vector<HANDLE> WaitHandleBuffer;
static size_t NextWaitChunkStart = 4;

HANDLE Thread = NULL;
HANDLE DataUsageMutex = NULL;       // for arrays with data shared by the thread and the process
HANDLE RefreshFinishedEvent = NULL; // due to "PostMessage" waits for processing to complete
HANDLE WantDataEvent = NULL;        // main thread requests access to the shared data
HANDLE TerminateEvent = NULL;       // main thread requests termination of the snooper thread
HANDLE ContinueEvent = NULL;        // helper event used for synchronization
HANDLE BeginSuspendEvent = NULL;    // beginning of suspend mode
HANDLE EndSuspendEvent = NULL;      // end of the snooper's suspend mode
HANDLE SharesEvent = NULL;          // signaled when LanMan Shares changes

int SnooperSuspended = 0;

CRITICAL_SECTION TimeCounterSection; // synchronizes access to MyTimeCounter
int MyTimeCounter = 0;               // current time

HANDLE SafeFindCloseThread = NULL;              // "safe handle killer" thread
TDirectArray<HANDLE> SafeFindCloseCNArr(10, 5); // safely (without hanging) closes change-notify handles
CRITICAL_SECTION SafeFindCloseCS;               // critical section for accessing the handle array
BOOL SafeFindCloseTerminate = FALSE;            // until thread termination is requested
HANDLE SafeFindCloseStart = NULL;               // thread "starter"—waits while non-signaled
HANDLE SafeFindCloseFinished = NULL;            // signaled once the thread has closed all handles

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

static DWORD WaitForChangeNotifications(DWORD timeout, bool ignoreRefreshes, DWORD& outIndex)
{
    outIndex = (DWORD)-1;

    size_t waitCount = WaitHandles.size();
    if (waitCount == 0)
        return WAIT_TIMEOUT;

    if (ignoreRefreshes || waitCount <= MAXIMUM_WAIT_OBJECTS)
    {
        DWORD waitLimit = ignoreRefreshes ? std::min<DWORD>(4, (DWORD)waitCount) : (DWORD)waitCount;
        if (waitLimit == 0)
            waitLimit = 1;
        DWORD res = WaitForMultipleObjects(waitLimit, WaitHandles.data(), FALSE, timeout);
        if (res >= WAIT_OBJECT_0 && res < WAIT_OBJECT_0 + waitLimit)
            outIndex = res - WAIT_OBJECT_0;
        return res;
    }

    const size_t baseCount = std::min<size_t>(4, waitCount);
    const size_t maxExtra = MAXIMUM_WAIT_OBJECTS > baseCount ? MAXIMUM_WAIT_OBJECTS - baseCount : 0;
    DWORD startTick = (timeout == INFINITE) ? 0 : GetTickCount();

    while (true)
    {
        waitCount = WaitHandles.size();
        size_t currentBase = std::min<size_t>(baseCount, waitCount);
        if (NextWaitChunkStart < currentBase || NextWaitChunkStart >= waitCount)
            NextWaitChunkStart = currentBase;

        size_t available = waitCount > NextWaitChunkStart ? waitCount - NextWaitChunkStart : 0;
        size_t extraCount = std::min(available, maxExtra);

        WaitHandleBuffer.clear();
        WaitHandleBuffer.reserve(currentBase + extraCount);
        if (currentBase > 0)
            WaitHandleBuffer.insert(WaitHandleBuffer.end(), WaitHandles.begin(), WaitHandles.begin() + currentBase);
        if (extraCount > 0)
        {
            WaitHandleBuffer.insert(WaitHandleBuffer.end(),
                                    WaitHandles.begin() + NextWaitChunkStart,
                                    WaitHandles.begin() + NextWaitChunkStart + extraCount);
        }

        DWORD sliceTimeout;
        if (timeout == INFINITE)
            sliceTimeout = REFRESH_PAUSE;
        else
        {
            DWORD now = GetTickCount();
            DWORD elapsed = now - startTick;
            if (elapsed >= timeout)
                return WAIT_TIMEOUT;
            sliceTimeout = std::min<DWORD>(timeout - elapsed, REFRESH_PAUSE);
        }

        DWORD res = WaitForMultipleObjects((DWORD)WaitHandleBuffer.size(), WaitHandleBuffer.data(), FALSE, sliceTimeout);
        if (res == WAIT_TIMEOUT)
        {
            if (timeout != INFINITE)
            {
                DWORD now = GetTickCount();
                if (now - startTick >= timeout)
                    return WAIT_TIMEOUT;
            }

            if (extraCount > 0)
            {
                NextWaitChunkStart += extraCount;
                if (NextWaitChunkStart >= waitCount)
                    NextWaitChunkStart = currentBase;
            }
            continue;
        }

        if (res >= WAIT_OBJECT_0 && res < WAIT_OBJECT_0 + WaitHandleBuffer.size())
        {
            DWORD localIndex = res - WAIT_OBJECT_0;
            if (localIndex < currentBase)
                outIndex = localIndex;
            else if (extraCount > 0)
                outIndex = (DWORD)(NextWaitChunkStart + (localIndex - currentBase));
        }

        if (extraCount > 0)
        {
            size_t nextStart = NextWaitChunkStart + extraCount;
            if (nextStart >= waitCount)
                nextStart = currentBase;
            NextWaitChunkStart = nextStart;
        }

        return res;
    }
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
    ReleaseMutex(DataUsageMutex);                  // release the data to the main thread
    WaitForSingleObject(WantDataEvent, INFINITE);  // wait until it takes ownership
    WaitForSingleObject(DataUsageMutex, INFINITE); // once it finishes, take ownership again
    SetEvent(ContinueEvent);                       // we own the data again; allow the main thread to continue
}

unsigned ThreadSnooperBody(void* /*param*/) // do not call main-thread functions (not even TRACE) !!!
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
    else // key opened successfully; enable notifications (otherwise RegNotifyChangeKeyValue will not be called again)
    {
        if ((res = RegNotifyChangeKeyValue(sharesKey, TRUE, REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET, SharesEvent,
                                           TRUE)) != ERROR_SUCCESS)
        {
            TRACE_E("Unable to monitor registry (LanMan Shares). error: " << GetErrorText(res));
        }
    }

    if (WaitForSingleObject(DataUsageMutex, INFINITE) == WAIT_OBJECT_0)
    {
        SetEvent(ContinueEvent); // the data now belong to the snooper; the main thread can continue

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

        BOOL ignoreRefreshes = FALSE;        // TRUE = ignore refreshes (directory changes); otherwise operate normally
        DWORD ignoreRefreshesAbsTimeout = 0; // when (int)(GetTickCount() - ignoreRefreshesAbsTimeout) >= 0, set ignoreRefreshes to FALSE
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
            DWORD waitIndex = (DWORD)-1;
            DWORD waitTimeout = (timeout == INFINITE) ? INFINITE : (timeout < 0 ? 0 : (DWORD)timeout);
            res = WaitForChangeNotifications(waitTimeout, ignoreRefreshes != FALSE, waitIndex);
            CALL_STACK_MESSAGE2("ThreadSnooperBody::wait_satisfied: 0x%X", res);

            if (res == WAIT_TIMEOUT)
                continue;

            if (res == WAIT_FAILED)
            {
                DWORD err = GetLastError();
                TRACE_E("Unexpected value returned from WaitForMultipleObjects(): " << res << ", error=" << err);
                continue;
            }

            switch (waitIndex)
            {
            case 0:
                DoWantDataEvent();
                break; // WantDataEvent
            case 1:
                notEnd = FALSE;
                break; // TerminateEvent
            case 2: // BeginSuspendMode
            {
                TRACE_I("Start suspend mode");

                SetEvent(ContinueEvent); // we are already in suspend; allow the main thread to continue

                TDirectArray<HWND> refreshPanels(10, 5); // for the case where the monitored directory is deleted

                WaitHandles[2] = EndSuspendEvent; // misto beginu ted end suspend modu

                BOOL setSharesEvent = FALSE; // TRUE => rearm registry monitoring
                BOOL suspendNotFinished = TRUE;
                while (suspendNotFinished) // wait for suspend mode to end
                {                          // handle everything except directory changes
                    timeout = ignoreRefreshes ? (int)(ignoreRefreshesAbsTimeout - GetTickCount()) : INFINITE;
                    if (ignoreRefreshes && timeout <= 0)
                    {
                        ignoreRefreshes = FALSE;
                        ignoreRefreshesAbsTimeout = 0;
                        timeout = INFINITE;
                    }
                    DWORD suspendIndex = (DWORD)-1;
                    DWORD suspendTimeout = (timeout == INFINITE) ? INFINITE : (timeout < 0 ? 0 : (DWORD)timeout);
                    res = WaitForChangeNotifications(suspendTimeout, ignoreRefreshes != FALSE, suspendIndex);

                    CALL_STACK_MESSAGE2("ThreadSnooperBody::suspend_wait_satisfied: 0x%X", res);

                    if (res == WAIT_TIMEOUT)
                        continue;

                    if (res == WAIT_FAILED)
                    {
                        DWORD err = GetLastError();
                        TRACE_E("Unexpected value returned from WaitForMultipleObjects(): " << res << ", error=" << err);
                        continue;
                    }

                    switch (suspendIndex)
                    {
                    case 0:
                        DoWantDataEvent();
                        break; // WantDataEvent
                    case 1:
                        suspendNotFinished = notEnd = FALSE;
                        break; // TerminateEvent
                    case 2:
                        suspendNotFinished = FALSE;
                        break; // EndSuspendEvent
                    case 3: // SharesEvent
                    {
                        // refresh shares and, if needed, panels (via WM_USER_REFRESH_SHARES)
                        setSharesEvent = TRUE;
                        break;
                    }

                    default:
                    {
                        if (suspendIndex >= 4 && suspendIndex < (DWORD)WatchEntrySlots.size())
                            RemoveWatchEntryDuringSuspend((size_t)suspendIndex, refreshPanels);
                        else
                            TRACE_E("Unexpected value returned from WaitForMultipleObjects(): " << res);
                        break;
                    }
                    }
                }
                SetEvent(ContinueEvent); // no longer suspended; allow the main thread to continue

                if (setSharesEvent) // continue monitoring further changes in the registry
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
                // refresh panels that changed
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
                // also notify that suspend mode ended
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
                    // pause briefly so the system is not overwhelmed
                    ignoreRefreshes = TRUE;
                    ignoreRefreshesAbsTimeout = GetTickCount() + REFRESH_PAUSE;
                }
                break;
            }

            case 3: // SharesEvent
            {                       // nechame refreshout panely
                if (MainWindowCS.LockIfNotClosed())
                {
                    if (MainWindow != NULL)
                        PostMessage(MainWindow->HWindow, WM_USER_REFRESH_SHARES, 0, 0);
                    MainWindowCS.Unlock();
                }
                // resume monitoring further registry changes
                if ((res = RegNotifyChangeKeyValue(sharesKey, TRUE, REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET, SharesEvent,
                                                   TRUE)) != ERROR_SUCCESS)
                {
                    TRACE_E("Unable to monitor registry (LanMan Shares). error: " << GetErrorText(res));
                }
                break;
            }

            default:
            {
                if (waitIndex < 4 || waitIndex >= (DWORD)WatchEntrySlots.size())
                {
                    DWORD err = GetLastError();
                    TRACE_E("Unexpected value returned from WaitForMultipleObjects(): " << res);
                    break; // for any other value of res
                }

                WatchEntry* entry = WatchEntrySlots[waitIndex];
                if (entry == NULL)
                    break;

                NotifySubscribers(entry);
                FindNextChangeNotification(WaitHandles[waitIndex]); // stornujem tuto zmenu
                                                                        // indexy se muzou zmenit...
                HANDLE objects[4];
                objects[0] = WantDataEvent;        // data may change during the refresh
                objects[1] = TerminateEvent;       // in case it terminates before the refresh finishes
                objects[2] = BeginSuspendEvent;    // in case BeginSuspendMode is called during the refresh
                objects[3] = RefreshFinishedEvent; // message from the main thread about finishing the refresh

                BOOL refreshNotFinished = TRUE;
                while (refreshNotFinished) // wait for processing to finish
                {                          // handle everything except directory changes
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
        TerminateProcess(GetCurrentProcess(), 1); // more forceful exit (this one still calls something)
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
    //--- create events and the mutex for synchronization
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

    // "starter" event for the "safe handle killer" thread
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
    //---  start the snooper thread
    DWORD ThreadID;
    Thread = HANDLES(CreateThread(NULL, 0, ThreadSnooper, NULL, 0, &ThreadID));
    if (Thread == NULL)
    {
        TRACE_E("Unable to start Snooper thread.");
        return FALSE;
    }
    //  SetThreadPriority(Thread, THREAD_PRIORITY_LOWEST);
    WaitForSingleObject(ContinueEvent, INFINITE); // wait until the snooper acquires the data

    HANDLES(InitializeCriticalSection(&SafeFindCloseCS));
    //---  start the "safe handle killer" thread
    SafeFindCloseThread = HANDLES(CreateThread(NULL, 0, ThreadFindCloseChangeNotification, NULL, 0, &ThreadID));
    if (SafeFindCloseThread == NULL)
    {
        TRACE_E("Unable to start safe-handle-killer thread.");
        return FALSE;
    }
    // raise its priority so it runs ahead of the main thread (the main thread
    // needs the handles closed immediately; on error there is no busy waiting, so this is acceptable)
    SetThreadPriority(SafeFindCloseThread, THREAD_PRIORITY_HIGHEST);

    return TRUE;
}

void TerminateThread()
{
    if (Thread != NULL) // terminate the snooper thread
    {
        SetEvent(TerminateEvent);              // request the snooper to terminate
        WaitForSingleObject(Thread, INFINITE); // wait until it terminates
        HANDLES(CloseHandle(Thread));          // close the thread handle
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
        SafeFindCloseTerminate = TRUE; // request thread termination
        SetEvent(SafeFindCloseStart);
        if (WaitForSingleObject(SafeFindCloseThread, 1000) == WAIT_TIMEOUT) // wait for it to exit
        {
            TerminateThread(SafeFindCloseThread, 666);          // failed, kill it forcefully
            WaitForSingleObject(SafeFindCloseThread, INFINITE); // wait until the thread actually ends; this can take quite a while
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
    ReleaseMutex(DataUsageMutex);                 // release the DataUsageMutex back to the snooper
    WaitForSingleObject(ContinueEvent, INFINITE); // and wait until it acquires it
}

// thread used to close handles for a "disconnected" network device (long wait)
unsigned ThreadFindCloseChangeNotificationBody(void* param)
{
    CALL_STACK_MESSAGE1("ThreadFindCloseChangeNotificationBody()");
    SetThreadNameInVCAndTrace("SafeHandleKiller");
    //  TRACE_I("Begin");

    while (!SafeFindCloseTerminate)
    {
        WaitForSingleObject(SafeFindCloseStart, INFINITE); // wait for start or termination

        while (1)
        {
            // retrieve a handle
            HANDLES(EnterCriticalSection(&SafeFindCloseCS));
            HANDLE h;
            BOOL br = FALSE;

            if (SafeFindCloseCNArr.IsGood() && SafeFindCloseCNArr.Count > 0)
            {
                h = SafeFindCloseCNArr[SafeFindCloseCNArr.Count - 1];
                SafeFindCloseCNArr.Delete(SafeFindCloseCNArr.Count - 1);
                if (!SafeFindCloseCNArr.IsGood())
                    SafeFindCloseCNArr.ResetState(); // cannot fail; it only reports lack of memory when shrinking the array
            }
            else
                br = TRUE;
            HANDLES(LeaveCriticalSection(&SafeFindCloseCS));

            if (br)
                break; // nothing left to close, wait for the next start

            // close the handle
            //      TRACE_I("Killing ... " << h);
            HANDLES(FindCloseChangeNotification(h));
        }

        SetEvent(SafeFindCloseFinished); // let the main thread continue ...
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
        TerminateProcess(GetCurrentProcess(), 1); // more forceful exit (this one still performs some calls)
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
    ReleaseMutex(DataUsageMutex);                 // release the DataUsageMutex back to the snooper
    WaitForSingleObject(ContinueEvent, INFINITE); // and wait until the snooper grabs it
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
    DWORD CallerCalledFromArr[SUSPMODESTACKSIZE];  // array of return addresses of functions from which BeginSuspendMode() was called
    DWORD CalledFromArr[SUSPMODESTACKSIZE];        // array of addresses from which BeginSuspendMode() was called
    int Count;                                     // number of elements in the previous two arrays
    int Ignored;                                   // number of BeginSuspendMode() calls we had to ignore (SUSPMODESTACKSIZE too small -> enlarge if needed)

  public:
    CSuspModeStack() {Count = 0; Ignored = 0;}
    ~CSuspModeStack() {CheckIfEmpty(1);}  // one BeginSuspendMode() is OK: invoked when the main Salamander window is deactivated (before the main window closes)

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
#ifdef _DEBUG     // verify whether BeginSuspendMode() and EndSuspendMode() are invoked from the same function (based on the return address of the calling function -> cannot detect a "bug" when called from different functions that are both invoked from the same function)
  DWORD *register_ebp;
  __asm mov register_ebp, ebp
  DWORD called_from, caller_called_from;
  __try
  {
    called_from = *(DWORD*)((char*)register_ebp + 4);

if this code ever needs to be revived, use the fact that it can be replaced (x86 and x64):
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
        SnooperSuspended = 0; // maybe CM_LEFTREFRESH, CM_RIGHTREFRESH, or CM_ACTIVEREFRESH is being misused again
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
#ifdef _DEBUG     // verify whether BeginSuspendMode() and EndSuspendMode() are called from the same function (based on the caller's return address, so it will not detect a "bug" when two different functions are both called from the same function)
void EndSuspendMode(BOOL debugDoNotTestCaller)
{
  DWORD *register_ebp;
  __asm mov register_ebp, ebp
  DWORD called_from, caller_called_from;
  __try
  {
    called_from = *(DWORD*)((char*)register_ebp + 4);

if this code ever needs to be re-enabled, note that it can be replaced with this (x86 and x64):
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
