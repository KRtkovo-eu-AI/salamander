﻿// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include <aclapi.h>
#include <crtdbg.h>
#include <process.h>
#include <ostream>
#include <stdio.h>
#include <commctrl.h>
#include <limits.h>

#ifndef __TRACESERVER
#pragma error "macro __TRACESERVER not defined";
#endif // __TRACESERVER

#include "lstrfix.h"
#include "trace.h"
#include "messages.h"
#include "handles.h"
#include "array.h"
#include "str.h"
#include "strutils.h"
#include "winlib.h"
#include "tablist.h"
#include "tserver.h"
#include "openedit.h"
#include "registry.h"
#include "config.h"
#include "allochan.h"

#include "tserver.rh"
#include "tserver.rh2"

#pragma comment(lib, "UxTheme.lib")

#ifndef MULTITHREADED_HEAP_ENABLE
#pragma error "macro MULTITHREADED_HEAP_ENABLE not defined";
#endif // MULTITHREADED_HEAP_ENABLE

#ifdef __HEAP_DISABLE
#pragma error "macro __HEAP_DISABLE defined";
#endif // __HEAP_DISABLE

DWORD CGlobalDataMessage::StaticIndex = 0;

BOOL UseMaxMessagesCount = FALSE;
int MaxMessagesCount = 10000;

BOOL WindowsVistaAndLater = FALSE;

// texts for the About dialog
WCHAR AboutText1[] = L"Version 2.03";

CMainWindow* MainWindow = NULL;

// application name
const WCHAR* MAINWINDOW_NAME = L"Trace Server";

// file names
//WCHAR *TRACE_FILENAME = L"tserver.trs";

CGlobalData Data;

// mutex owned by the client process that writes into shared memory
HANDLE OpenConnectionMutex = NULL;
// event - signaled -> shared memory contains the requested data
HANDLE ConnectDataReadyEvent = NULL;
// event - signaled -> the server has accepted data from shared memory
HANDLE ConnectDataAcceptedEvent = NULL;
BOOL ConnectDataAcceptedEventMayBeSignaled = FALSE;

// event - manual reset - signaled -> the server is shutting down -> all threads should finish
HANDLE TerminateEvent = NULL;
// event used when starting ReadPipeThread to load input data
HANDLE ContinueEvent = NULL;
// event - manual reset - set after flushing the message cache
HANDLE MessagesFlushDoneEvent = NULL;

// thread that handles connecting to the server
HANDLE ConnectingThread = NULL;

BOOL IconControlEnable = TRUE;

// array of active pipe threads
struct CReadPipeThreadInfo
{
    HANDLE Thread;
    DWORD ClientPID;
};
TSynchronizedDirectArray<CReadPipeThreadInfo> ActiveReadPipeThreads(10, 5);

// function executed by the connecting thread
unsigned __stdcall ConnectingThreadF(void* mainWndPtr);

//****************************************************************************
//
// InitializeServer
//

BOOL InitializeServer(HWND mainWnd)
{
    // prepare a "NULL PACL", i.e. a descriptor completely open from the permissions point of view
    // a foreign process can, for example, adjust the rights of objects created this way
    // in the case of TraceServer we do not mind and it keeps things simple
    char secDesc[SECURITY_DESCRIPTOR_MIN_LENGTH];
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = FALSE;
    sa.lpSecurityDescriptor = &secDesc;
    InitializeSecurityDescriptor(sa.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    // give the security descriptor a NULL DACL, done using the  "TRUE, (PACL)NULL" here
    SetSecurityDescriptorDacl(sa.lpSecurityDescriptor, TRUE, 0, FALSE);
    SECURITY_ATTRIBUTES* saPtr = &sa;

    OpenConnectionMutex = HANDLES_Q(CreateMutex(saPtr, TRUE, __OPEN_CONNECTION_MUTEX));
    ConnectDataReadyEvent = HANDLES_Q(CreateEvent(saPtr, FALSE, FALSE,
                                                  __CONNECT_DATA_READY_EVENT_NAME));
    ConnectDataAcceptedEvent = HANDLES_Q(CreateEvent(saPtr, FALSE, FALSE,
                                                     __CONNECT_DATA_ACCEPTED_EVENT_NAME));

    ContinueEvent = HANDLES(CreateEvent(saPtr, FALSE, FALSE, NULL));
    TerminateEvent = HANDLES(CreateEvent(saPtr, TRUE, FALSE, NULL));         // manual reset
    MessagesFlushDoneEvent = HANDLES(CreateEvent(saPtr, TRUE, FALSE, NULL)); // manual reset

    if (OpenConnectionMutex == NULL || ConnectDataReadyEvent == NULL ||
        ConnectDataAcceptedEvent == NULL || TerminateEvent == NULL ||
        ContinueEvent == NULL || MessagesFlushDoneEvent == NULL)
    {
        MESSAGE_EW(NULL, L"Unable to create synchronization objects.", MB_OK);
        return FALSE;
    }

    unsigned dummyID;
    ConnectingThread = (HANDLE)HANDLES(_beginthreadex(NULL, 1000,
                                                      ConnectingThreadF,
                                                      mainWnd, 0, &dummyID));
    if (ConnectingThread == NULL)
    {
        MESSAGE_EW(NULL, L"Unable to create connecting thread.", MB_OK);
        return FALSE;
    }
    return TRUE; // when ConnectingThread != NULL it must return TRUE!!!
}

//****************************************************************************
//
// ReleaseServer
//

void ReleaseServer()
{
    if (ConnectingThread != NULL)
    {
        SetEvent(TerminateEvent);
        WaitForSingleObject(ConnectingThread, INFINITE);
        HANDLES(CloseHandle(ConnectingThread));

        ActiveReadPipeThreads.BlockArray();
        int count = ActiveReadPipeThreads.GetCount();
        for (int i = 0; i < count; i++)
        {
            TerminateThread(ActiveReadPipeThreads[i].Thread, 0);
            WaitForSingleObject(ActiveReadPipeThreads[i].Thread, INFINITE);
            HANDLES(CloseHandle(ActiveReadPipeThreads[i].Thread));
        }
        ActiveReadPipeThreads.UnBlockArray();
    }
    if (OpenConnectionMutex != NULL)
        HANDLES(CloseHandle(OpenConnectionMutex));
    if (ConnectDataReadyEvent != NULL)
        HANDLES(CloseHandle(ConnectDataReadyEvent));
    if (ConnectDataAcceptedEvent != NULL)
        HANDLES(CloseHandle(ConnectDataAcceptedEvent));
    if (ContinueEvent != NULL)
        HANDLES(CloseHandle(ContinueEvent));
    if (TerminateEvent != NULL)
        HANDLES(CloseHandle(TerminateEvent));
    if (MessagesFlushDoneEvent != NULL)
        HANDLES(CloseHandle(MessagesFlushDoneEvent));
}

//****************************************************************************
//
// ReadPipeThreadF
//

BOOL ReadPipe(HANDLE pipeSemaphore, DWORD& readBytesFromPipe, HANDLE hFile,
              LPVOID lpBuffer, DWORD nNumberOfBytesToRead, BOOL& showSemaphoreErr)
{
    DWORD read;
    DWORD totalBytesToRead = nNumberOfBytesToRead;
    DWORD numberOfBytesRead = 0;
    while (ReadFile(hFile, (((char*)lpBuffer) + numberOfBytesRead),
                    nNumberOfBytesToRead, &read, NULL))
    {
        readBytesFromPipe += read;
        if (readBytesFromPipe >= 1024)
        {
            if (ReleaseSemaphore(pipeSemaphore, readBytesFromPipe / 1024, NULL))
            {
                readBytesFromPipe %= 1024;
            }
            else
            {
                if (showSemaphoreErr) // it makes sense to display it only once for each pipe
                {
                    MESSAGE_TEW(L"Invalid state of pipe semaphore.", MB_OK);
                    showSemaphoreErr = FALSE;
                }
            }
        }

        numberOfBytesRead += read;
        nNumberOfBytesToRead -= read;
        if (nNumberOfBytesToRead <= 0)
            return numberOfBytesRead == totalBytesToRead;
    }
    numberOfBytesRead += read;
    return FALSE;
}

struct CReadPipeData
{
    static DWORD StaticUniqueProcessID;

    HWND MainWnd;
    HANDLE ReadPipe;
    HANDLE PipeSemaphore;
    HANDLE Thread;
    DWORD ProcessID;
    DWORD UniqueProcessID;
    BOOL SendProcessConnected;
    BOOL ShowSemaphoreErr; // because of a bug in old clients the semaphore only decreases until an error occurs; known issue, not reported
};

DWORD CReadPipeData::StaticUniqueProcessID = 0;

unsigned __stdcall ReadPipeThreadF(void* dataPtr)
{
    CReadPipeData* data = (CReadPipeData*)dataPtr;
    // Load the input data
    HWND mainWnd = data->MainWnd;
    HANDLE readPipe = data->ReadPipe;
    HANDLE pipeSemaphore = data->PipeSemaphore;
    HANDLE thread = data->Thread;
    DWORD processID = data->ProcessID;
    DWORD uniqueProcessID = data->UniqueProcessID;
    BOOL sendProcessConnected = data->SendProcessConnected;
    BOOL showSemaphoreErr = data->ShowSemaphoreErr;
    SetEvent(ContinueEvent);
    // From this point on the data pointer is invalid
    data = NULL;
    // Read messages from the pipe
    DWORD readBytesFromPipe = 0;
    CGlobalDataMessage message;
    message.ProcessID = processID;

    C__PipeDataHeader pipeData;
    BOOL error = FALSE;

    while (1)
    {
        if (ReadPipe(pipeSemaphore, readBytesFromPipe, readPipe, &pipeData, sizeof(pipeData), showSemaphoreErr))
        {
            switch (pipeData.Type)
            {
            case __mtSetProcessName:
            case __mtSetThreadName:
            case __mtSetProcessNameW:
            case __mtSetThreadNameW:
            {
                BOOL unicode = (pipeData.Type == __mtSetProcessNameW || pipeData.Type == __mtSetThreadNameW);
                char* name = (char*)malloc((unicode ? sizeof(WCHAR) : 1) * pipeData.MessageSize);
                if (name != NULL)
                {
                    if (ReadPipe(pipeSemaphore, readBytesFromPipe, readPipe, name,
                                 (unicode ? sizeof(WCHAR) : 1) * pipeData.MessageSize, showSemaphoreErr))
                    {
                        WCHAR* nameW = unicode ? (WCHAR*)name : ConvertAllocA2U(name, pipeData.MessageSize - 1);
                        if (!unicode)
                            free(name);
                        name = NULL;

                        if (nameW != NULL)
                        {
                            if (pipeData.Type == __mtSetProcessName || pipeData.Type == __mtSetProcessNameW)
                            {
                                // ProcessID arrived in pipeData.Line - see the comment in the header
                                Data.Processes.BlockArray();
                                int index = Data.FindProcessNameIndex(uniqueProcessID);
                                if (index != -1)
                                {
                                    free(Data.Processes[index].Name);
                                    Data.Processes[index].Name = nameW;
                                }
                                else
                                {
                                    CProcessInformation processInformation;
                                    processInformation.UniqueProcessID = uniqueProcessID;
                                    processInformation.Name = nameW;
                                    // add to the array
                                    Data.Processes.Add(processInformation);
                                }
                                Data.Processes.UnBlockArray();
                                PostMessage(mainWnd, WM_USER_PROCESSES_CHANGE, 0, 0);
                            }
                            else
                            {
                                Data.Threads.BlockArray();
                                int index = Data.FindThreadNameIndex(uniqueProcessID,
                                                                     pipeData.UniqueThreadID);
                                if (index != -1)
                                {
                                    free(Data.Threads[index].Name);
                                    Data.Threads[index].Name = nameW;
                                }
                                else
                                {
                                    CThreadInformation threadInformation;
                                    threadInformation.UniqueProcessID = uniqueProcessID;
                                    threadInformation.UniqueThreadID = pipeData.UniqueThreadID;
                                    threadInformation.Name = nameW;

                                    // add to the array
                                    Data.Threads.Add(threadInformation);
                                }
                                Data.Threads.UnBlockArray();
                                PostMessage(mainWnd, WM_USER_THREADS_CHANGE, 0, 0);
                            }
                        }
                        else
                        {
                            PostMessage(mainWnd, WM_USER_SHOWERROR, EC_LOW_MEMORY, 0);
                            error = TRUE;
                            SetLastError(ERROR_BROKEN_PIPE); // because of the condition below
                        }
                    }
                    else
                    {
                        DWORD err = GetLastError();
                        free(name);
                        error = TRUE;
                        if (err == ERROR_SUCCESS)
                            SetLastError(ERROR_BROKEN_PIPE); // because of the condition below
                    }
                }
                else
                {
                    PostMessage(mainWnd, WM_USER_SHOWERROR, EC_LOW_MEMORY, 0);
                    error = TRUE;
                    SetLastError(ERROR_BROKEN_PIPE); // because of the condition below
                }
                break;
            }

            case __mtInformation:
            case __mtError:
            case __mtInformationW:
            case __mtErrorW:
            {
                BOOL unicode = pipeData.Type == __mtInformationW || pipeData.Type == __mtErrorW;

                message.ThreadID = pipeData.ThreadID;
                message.Type = (C__MessageType)pipeData.Type;
                message.Time = pipeData.Time;
                message.Counter = pipeData.Counter;
                message.Line = pipeData.Line;
                message.UniqueProcessID = uniqueProcessID;
                message.UniqueThreadID = pipeData.UniqueThreadID;

                char* file = (char*)malloc((unicode ? sizeof(WCHAR) : 1) * pipeData.MessageSize);
                if (file != NULL)
                {
                    if (ReadPipe(pipeSemaphore, readBytesFromPipe, readPipe, file,
                                 (unicode ? sizeof(WCHAR) : 1) * pipeData.MessageSize, showSemaphoreErr))
                    {
                        message.File = unicode ? (WCHAR*)file : ConvertAllocA2U(file, pipeData.MessageSize - 1);
                        if (!unicode)
                            free(file);
                        file = NULL;
                        if (message.File != NULL)
                        {
                            message.Message = message.File + (unicode ? pipeData.MessageTextOffset : wcslen(message.File) + 1);

                            while (1)
                            {
                                BOOL breakCycle;
                                Data.MessagesCache.BlockArray();
                                if (Data.MessagesCache.GetCount() >= MESSAGES_CACHE_MAX)
                                {
                                    if (!Data.MessagesFlushInProgress)
                                    {
                                        ResetEvent(MessagesFlushDoneEvent);
                                        PostMessage(mainWnd, WM_USER_FLUSH_MESSAGES_CACHE, 0, 0);
                                        Data.MessagesFlushInProgress = TRUE;
                                    }
                                    breakCycle = FALSE;
                                }
                                else
                                {
                                    Data.MessagesCache.Add(message);
                                    breakCycle = TRUE;
                                }
                                Data.MessagesCache.UnBlockArray();

                                if (breakCycle)
                                    break;
                                else
                                    WaitForSingleObject(MessagesFlushDoneEvent, INFINITE);
                            } // not perfect, but it should be enough (it keeps below the maximum roughly 99%)
                        }
                        else
                        {
                            PostMessage(mainWnd, WM_USER_SHOWERROR, EC_LOW_MEMORY, 0);
                            error = TRUE;
                            SetLastError(ERROR_BROKEN_PIPE); // because of the condition below
                        }
                    }
                    else
                    {
                        DWORD err = GetLastError();
                        free(file);
                        error = TRUE;
                        if (err == ERROR_SUCCESS)
                            SetLastError(ERROR_BROKEN_PIPE); // because of the condition below
                    }
                }
                else
                {
                    PostMessage(mainWnd, WM_USER_SHOWERROR, EC_LOW_MEMORY, 0);
                    error = TRUE;
                    SetLastError(ERROR_BROKEN_PIPE); // because of the condition below
                }
                break;
            }

            case __mtIgnoreAutoClear:
            {
                if (sendProcessConnected && pipeData.ThreadID == 0) // 0 = do not ignore, 1 = ignore the auto-clear on Trace Server
                    SendMessage(mainWnd, WM_USER_PROCESS_CONNECTED, 0, 0);
                break;
            }

            default:
            {
                PostMessage(mainWnd, WM_USER_SHOWERROR, EC_UNKNOWN_MESSAGE_TYPE, 0);
                error = TRUE;
                SetLastError(ERROR_BROKEN_PIPE); // because of the condition below
                break;
            }
            }
        }
        else
            error = TRUE;

        if (error)
        {
            if (GetLastError() != ERROR_BROKEN_PIPE)
                PostMessage(mainWnd, WM_USER_SHOWSYSTEMERROR, GetLastError(), 0);
            break;
        }
    }
    // The process disconnected
    PostMessage(mainWnd, WM_USER_PROCESS_DISCONNECTED, processID, 0);
    HANDLES(CloseHandle(readPipe));
    HANDLES(CloseHandle(pipeSemaphore));

    ActiveReadPipeThreads.BlockArray();
    int count = ActiveReadPipeThreads.GetCount();
    int i = 0;
    for (; i < count; i++)
    {
        if (ActiveReadPipeThreads[i].Thread == thread)
        {
            HANDLES(CloseHandle(thread));
            ActiveReadPipeThreads.Delete(i);
            break;
        }
    }
    if (i == count)
    {
        MESSAGE_TEW(L"Thread handle " << thread << L" was not found in array ActiveReadPipeThreads.", MB_OK);
    }
    ActiveReadPipeThreads.UnBlockArray();
    _endthreadex(0);
    return 0;
}

//****************************************************************************
//
// IsReadPipeThreadForNewProcess
//

BOOL IsReadPipeThreadForNewProcess(DWORD clientPID)
{
    ActiveReadPipeThreads.BlockArray();
    BOOL isNewProcess = TRUE;
    int count = ActiveReadPipeThreads.GetCount();
    for (int i = 0; i < count; i++)
    {
        if (ActiveReadPipeThreads[i].ClientPID == clientPID)
        {
            isNewProcess = FALSE;
            break;
        }
    }
    ActiveReadPipeThreads.UnBlockArray();
    return isNewProcess;
}

//****************************************************************************
//
// ConnectingThreadF
//

unsigned __stdcall ConnectingThreadF(void* mainWndPtr)
{
    HWND mainWnd = (HWND)mainWndPtr;
    // Create the shared memory block

    // prepare a "NULL PACL", i.e. a descriptor completely open from the permissions point of view
    // a foreign process can, for example, adjust the rights of objects created this way
    // in the case of TraceServer we do not mind and it keeps things simple
    char secDesc[SECURITY_DESCRIPTOR_MIN_LENGTH];
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = FALSE;
    sa.lpSecurityDescriptor = &secDesc;
    InitializeSecurityDescriptor(sa.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    // give the security descriptor a NULL DACL, done using the  "TRUE, (PACL)NULL" here
    SetSecurityDescriptorDacl(sa.lpSecurityDescriptor, TRUE, 0, FALSE);
    SECURITY_ATTRIBUTES* saPtr = &sa;

    HANDLE hFileMapping = HANDLES_Q(CreateFileMapping((HANDLE)0xFFFFFFFF,
                                                      saPtr,
                                                      PAGE_READWRITE,
                                                      0, sizeof(C__ClientServerInitData),
                                                      __FILE_MAPPING_NAME));
    if (hFileMapping == NULL)
    {
        PostMessage(mainWnd, WM_USER_CT_TERMINATED, 0, 0);
        _endthreadex(CT_UNABLE_TO_CREATE_FILE_MAPPING);
        return CT_UNABLE_TO_CREATE_FILE_MAPPING;
    }

    void* mapAddress = HANDLES(MapViewOfFile(hFileMapping,
                                             FILE_MAP_ALL_ACCESS,
                                             0,
                                             0,
                                             sizeof(C__ClientServerInitData)));
    if (mapAddress == NULL)
    {
        HANDLES(CloseHandle(hFileMapping));
        PostMessage(mainWnd, WM_USER_CT_TERMINATED, 0, 0);
        _endthreadex(CT_UNABLE_TO_MAP_VIEW_OF_FILE);
        return CT_UNABLE_TO_MAP_VIEW_OF_FILE;
    }
    // Run the main execution loop
    PostMessage(mainWnd, WM_USER_CT_OPENCONNECTION, 0, 0);

    HANDLE handles[2];
    handles[0] = TerminateEvent;
    handles[1] = ConnectDataReadyEvent;
    DWORD wait, run = TRUE;

    while (run)
    {
        wait = WaitForMultipleObjects(2, handles, FALSE, INFINITE);

        switch (wait)
        {
        case WAIT_OBJECT_0:
            run = FALSE;
            break; // terminate

        case WAIT_OBJECT_0 + 1: // data ready
        {
            C__ClientServerInitData data = *((C__ClientServerInitData*)mapAddress);
            if (data.Version == TRACE_SERVER_VERSION - 1 || // the client created the pipe and semaphore, we have to adopt them
                data.Version == TRACE_SERVER_VERSION - 3)   // old client, let it connect (but without __mtIgnoreAutoClear)
            {
                HANDLE readPipe = NULL;
                HANDLE pipeSemaphore = NULL;
                // obtain the handle of the client process
                DWORD clientPID = data.ClientOrServerProcessId;
                HANDLE clientProcess = HANDLES_Q(OpenProcess(PROCESS_DUP_HANDLE, FALSE, clientPID));
                // obtain the handles of the pipe and semaphore
                if (clientProcess != NULL &&
                    HANDLES(DuplicateHandle(clientProcess, data.HReadOrWritePipe, // client
                                            GetCurrentProcess(), &readPipe,       // server
                                            GENERIC_READ, FALSE, 0)) &&
                    HANDLES(DuplicateHandle(clientProcess, data.HPipeSemaphore,  // client
                                            GetCurrentProcess(), &pipeSemaphore, // server
                                            0, FALSE, DUPLICATE_SAME_ACCESS)))
                {
                    BOOL newProcess = IsReadPipeThreadForNewProcess(clientPID);
                    unsigned threadID;
                    CReadPipeData readPipeData;
                    readPipeData.MainWnd = mainWnd;
                    readPipeData.ReadPipe = readPipe;
                    readPipeData.PipeSemaphore = pipeSemaphore;
                    readPipeData.ProcessID = clientPID;
                    readPipeData.SendProcessConnected = newProcess && data.Version == TRACE_SERVER_VERSION - 1;
                    readPipeData.ShowSemaphoreErr = data.Version == TRACE_SERVER_VERSION - 1; // report only once and only for new clients
                    // if two connections are made from one process (e.g. in POB: Test and POB.dll),
                    // we intentionally assign two unique PIDs to make process naming work
                    // in Trace Server, simply so it is visible who sent the message (e.g. Test or POB.dll)
                    readPipeData.UniqueProcessID = readPipeData.StaticUniqueProcessID++;
                    ResetEvent(ContinueEvent);
                    HANDLE thread = (HANDLE)HANDLES(_beginthreadex(NULL, 1000,
                                                                   ReadPipeThreadF,
                                                                   &readPipeData,
                                                                   CREATE_SUSPENDED,
                                                                   &threadID));
                    if (thread != NULL)
                    {
                        readPipeData.Thread = thread; // provide the thread with its HANDLE
                        CReadPipeThreadInfo rpti;
                        rpti.ClientPID = clientPID;
                        rpti.Thread = thread;
                        ActiveReadPipeThreads.BlockArray();
                        ActiveReadPipeThreads.Add(rpti); // add it among the active ones
                        ActiveReadPipeThreads.UnBlockArray();
                        if (newProcess && data.Version == TRACE_SERVER_VERSION - 3) // old server, run without __mtIgnoreAutoClear
                            SendMessage(mainWnd, WM_USER_PROCESS_CONNECTED, 0, 0);
                        ResumeThread(thread); // start readPipeThread

                        WaitForSingleObject(ContinueEvent, INFINITE);

                        *((BOOL*)mapAddress) = TRUE; // write the result
                    }
                    else
                    {
                        HANDLES(CloseHandle(readPipe));
                        HANDLES(CloseHandle(pipeSemaphore));
                        PostMessage(mainWnd, WM_USER_SHOWERROR,
                                    EC_CANNOT_CREATE_READ_PIPE_THREAD, 0);
                        *((BOOL*)mapAddress) = FALSE; // write the result
                    }
                }
                else
                {
                    if (readPipe != NULL)
                        HANDLES(CloseHandle(readPipe));
                    if (pipeSemaphore != NULL)
                        HANDLES(CloseHandle(pipeSemaphore));
                    *((BOOL*)mapAddress) = FALSE; // write the result -> it failed
                }
                if (clientProcess != NULL)
                    HANDLES(CloseHandle(clientProcess));
            }
            else
            {
                if (data.Version == TRACE_SERVER_VERSION ||   // we should create the pipe and semaphore and send them to the client
                    data.Version == TRACE_SERVER_VERSION - 2) // old client, let it connect (but without __mtIgnoreAutoClear)
                {
                    // prepare a "NULL PACL", i.e. a descriptor completely open from the permissions point of view
                    // a foreign process can, for example, adjust the rights of objects created this way
                    // in the case of TraceServer we do not mind and it keeps things simple
                    char sd[SECURITY_DESCRIPTOR_MIN_LENGTH];
                    SECURITY_ATTRIBUTES sa;
                    sa.nLength = sizeof(sa);
                    sa.bInheritHandle = FALSE;
                    sa.lpSecurityDescriptor = &sd;
                    InitializeSecurityDescriptor(sa.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
                    // give the security descriptor a NULL DACL, done using the  "TRUE, (PACL)NULL" here
                    SetSecurityDescriptorDacl(sa.lpSecurityDescriptor, TRUE, 0, FALSE);
                    SECURITY_ATTRIBUTES* saPtr = &sa;

                    C__ClientServerInitData* dataWr = (C__ClientServerInitData*)mapAddress;
                    HANDLE pipeSemaphore = HANDLES(CreateSemaphore(saPtr, __PIPE_SIZE, __PIPE_SIZE, NULL));
                    HANDLE readPipe = NULL;
                    HANDLE writePipe = NULL;
                    if (pipeSemaphore != NULL && HANDLES(CreatePipe(&readPipe, &writePipe, saPtr, __PIPE_SIZE * 1024)))
                    {
                        // write into shared memory the handle for writing to the pipe (for the client)
                        dataWr->Version = TRUE;                                  // BOOL value: TRUE = we have a pipe
                        dataWr->ClientOrServerProcessId = GetCurrentProcessId(); // here it is the server PID
                        dataWr->HReadOrWritePipe = writePipe;
                        dataWr->HPipeSemaphore = pipeSemaphore;

                        SetEvent(ConnectDataAcceptedEvent); // hand data to the client, the results are stored
                        ConnectDataAcceptedEventMayBeSignaled = TRUE;

                        // wait until the server processes the data
                        DWORD waitRet = WaitForSingleObject(ConnectDataReadyEvent, __COMMUNICATION_WAIT_TIMEOUT);
                        if (waitRet == WAIT_OBJECT_0 && dataWr->Version == 3 /* 3 = success, the client took the handles */) // look at the result from the client
                        {
                            DWORD clientPID = dataWr->ClientOrServerProcessId; /* client PID */
                            BOOL newProcess = IsReadPipeThreadForNewProcess(clientPID);
                            unsigned threadID;
                            CReadPipeData readPipeData;
                            readPipeData.MainWnd = mainWnd;
                            readPipeData.ReadPipe = readPipe;
                            readPipeData.PipeSemaphore = pipeSemaphore;
                            readPipeData.ProcessID = clientPID;
                            readPipeData.SendProcessConnected = newProcess && data.Version == TRACE_SERVER_VERSION;
                            readPipeData.ShowSemaphoreErr = data.Version == TRACE_SERVER_VERSION; // report only once and only for new clients
                            // if two connections are made from one process (e.g. in POB: Test and POB.dll),
                            // we intentionally assign two unique PIDs to make process naming work
                            // in Trace Server, simply so it is visible who sent the message (e.g. Test or POB.dll)
                            readPipeData.UniqueProcessID = readPipeData.StaticUniqueProcessID++;
                            ResetEvent(ContinueEvent);
                            HANDLE thread = (HANDLE)HANDLES(_beginthreadex(NULL, 1000,
                                                                           ReadPipeThreadF,
                                                                           &readPipeData,
                                                                           CREATE_SUSPENDED,
                                                                           &threadID));
                            if (thread != NULL)
                            {
                                readPipeData.Thread = thread; // provide the thread with its HANDLE
                                CReadPipeThreadInfo rpti;
                                rpti.ClientPID = clientPID;
                                rpti.Thread = thread;
                                ActiveReadPipeThreads.BlockArray();
                                ActiveReadPipeThreads.Add(rpti); // add it among the active ones
                                ActiveReadPipeThreads.UnBlockArray();
                                if (newProcess && data.Version == TRACE_SERVER_VERSION - 2) // old server, run without __mtIgnoreAutoClear
                                    SendMessage(mainWnd, WM_USER_PROCESS_CONNECTED, 0, 0);
                                ResumeThread(thread); // start readPipeThread

                                WaitForSingleObject(ContinueEvent, INFINITE);
                                dataWr->Version = 2; // 2 = thread started successfully, communication established!

                                readPipe = NULL; // clear these variables so the handles do not get closed (already used in the thread)
                                pipeSemaphore = NULL;
                            }
                            else
                            {
                                PostMessage(mainWnd, WM_USER_SHOWERROR, EC_CANNOT_CREATE_READ_PIPE_THREAD, 0);
                                dataWr->Version = FALSE; // BOOL value: FALSE = report failure, end of communication
                            }
                        }
                    }
                    else
                        dataWr->Version = FALSE; // BOOL value: FALSE = report failure, end of communication
                    if (readPipe != NULL)
                        HANDLES(CloseHandle(readPipe));
                    if (writePipe != NULL)
                        HANDLES(CloseHandle(writePipe));
                    if (pipeSemaphore != NULL)
                        HANDLES(CloseHandle(pipeSemaphore));
                }
                else
                {
                    *((BOOL*)mapAddress) = FALSE; // write the result -> it failed
                    PostMessage(mainWnd, WM_USER_INCORRECT_VERSION,
                                data.Version, data.ClientOrServerProcessId);
                }
            }
            SetEvent(ConnectDataAcceptedEvent); // action finished, the result is stored
            ConnectDataAcceptedEventMayBeSignaled = TRUE;
            break;
        }
        }
    }
    // Release the shared memory block
    HANDLES(UnmapViewOfFile(mapAddress));
    HANDLES(CloseHandle(hFileMapping));
    _endthreadex(CT_SUCCESS);
    return CT_SUCCESS;
}

//*****************************************************************************
//
// CGlobalData
//

CGlobalData::CGlobalData()
    : Processes(10, 5), Threads(10, 5), MessagesCache(100, 50), Messages(1000, 500)
{
    MessagesFlushInProgress = FALSE;
    EditorConnected = FALSE;
}

CGlobalData::~CGlobalData()
{
    Processes.BlockArray();
    int count = Processes.GetCount();
    for (int i = 0; i < count; i++)
        free(Processes[i].Name);
    Processes.UnBlockArray();

    Threads.BlockArray();
    count = Threads.GetCount();
    for (int i = 0; i < count; i++)
        free(Threads[i].Name);
    Threads.UnBlockArray();

    MessagesCache.BlockArray();
    count = MessagesCache.GetCount();
    for (int i = 0; i < count; i++)
    { // Message is only an offset -> do not deallocate
        if (MessagesCache[i].File != NULL)
        {
            free(MessagesCache[i].File);
            MessagesCache[i].File = NULL;
        }
    }
    MessagesCache.UnBlockArray();

    for (int i = 0; i < Messages.Count; i++)
    {
        if (Messages[i].File != NULL)
        {
            free(Messages[i].File);
            Messages[i].File = NULL;
        }
    }
    /* jr - migration to MSVC
  if (EditorConnected) EDITDisconnect();
*/
}

int CGlobalData::FindProcessNameIndex(DWORD uniqueProcessID)
{
    if (Processes.CroakIfNotBlocked())
        return -1;
    for (int i = 0; i < Processes.GetCount(); i++)
    {
        if (Processes[i].UniqueProcessID == uniqueProcessID)
            return i;
    }
    return -1;
}

int CGlobalData::FindThreadNameIndex(DWORD uniqueProcessID, DWORD uniqueThreadID)
{
    if (Threads.CroakIfNotBlocked())
        return -1;
    for (int i = 0; i < Threads.GetCount(); i++)
    {
        if (Threads[i].UniqueProcessID == uniqueProcessID && Threads[i].UniqueThreadID == uniqueThreadID)
            return i;
    }
    return -1;
}

void CGlobalData::GetProcessName(DWORD uniqueProcessID, WCHAR* buff, int buffLen)
{
    Processes.BlockArray();
    int index = FindProcessNameIndex(uniqueProcessID);
    wcsncpy_s(buff, buffLen, index != -1 ? Processes[index].Name : L"Unknown", buffLen - 1);
    Processes.UnBlockArray();
}

void CGlobalData::GetThreadName(DWORD uniqueProcessID,
                                DWORD uniqueThreadID, WCHAR* buff, int buffLen)
{
    Threads.BlockArray();
    int index = FindThreadNameIndex(uniqueProcessID, uniqueThreadID);
    wcsncpy_s(buff, buffLen, index != -1 ? Threads[index].Name : L"Unknown", buffLen - 1);
    Threads.UnBlockArray();
}

void CGlobalData::GotoEditor(int index)
{
    OpenFileInMSVC(Messages[index].File, Messages[index].Line);
}

//*****************************************************************************
//
// CGlobalDataMessage
//

BOOL CGlobalDataMessage::operator<(const CGlobalDataMessage& message)
{
    if (Counter == 0)
    {
        if (Time.wYear == message.Time.wYear)
        {
            if (Time.wMonth == message.Time.wMonth)
            {
                if (Time.wDay == message.Time.wDay)
                {
                    if (Time.wHour == message.Time.wHour)
                    {
                        if (Time.wMinute == message.Time.wMinute)
                        {
                            if (Time.wSecond == message.Time.wSecond)
                            {
                                if (Time.wMilliseconds == message.Time.wMilliseconds)
                                {
                                    return (Index < message.Index);
                                }
                                else
                                    return (Time.wMilliseconds < message.Time.wMilliseconds);
                            }
                            else
                                return (Time.wSecond < message.Time.wSecond);
                        }
                        else
                            return (Time.wMinute < message.Time.wMinute);
                    }
                    else
                        return (Time.wHour < message.Time.wHour);
                }
                else
                    return (Time.wDay < message.Time.wDay);
            }
            else
                return (Time.wMonth < message.Time.wMonth);
        }
        else
            return (Time.wYear < message.Time.wYear);
    }
    else
        return Counter < message.Counter;
}

//*****************************************************************************
//
// BuildFonts
//

BOOL BuildFonts()
{
    return TRUE;
}

void DeleteFonts()
{
}

//*****************************************************************************
//
// WinMain
//

int WINAPI
wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPWSTR /*cmdLine*/, int /*cmdShow*/)
{
    HInstance = hInstance;

    SetMessagesTitleW(MAINWINDOW_NAME);

    // not useful for Trace Server; it logs only into files...
    //SetTraceProcessNameW(MAINWINDOW_NAME);
    //SetTraceThreadNameW(L"Main");

    // configure localized messages for the ALLOCHAN module (handles out-of-memory reporting to the user + Retry button + Cancel when everything fails to terminate the software)
    SetAllocHandlerMessage(NULL, MAINWINDOW_NAME, NULL, NULL);

    HWND hPrevWindow = FindWindow(WC_MAINWINDOW, MAINWINDOW_NAME);
    if (hPrevWindow != NULL)
    {
        if (IsIconic(hPrevWindow))
            ShowWindow(hPrevWindow, SW_RESTORE);
        ShowWindow(hPrevWindow, SW_SHOW);
        SetForegroundWindow(hPrevWindow);
        const char* msg = "Other instance of Trace Server is already running.";
        TRACE_I(msg);
        DMESSAGE_TI(msg, MB_OK);
        return 0;
    }

    TRACE_I("Begin.");

    WindowsVistaAndLater = TServerIsWindowsVersionOrGreater(6, 0, 0);

    // to allow a process running under another user to access TServer on Vista
    // (runas), it was necessary to allow opening the process handle; everything is enabled here
    HANDLE hProcess = GetCurrentProcess();
    DWORD err = SetSecurityInfo(hProcess, SE_KERNEL_OBJECT,
                                DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                                NULL, NULL, NULL, NULL);

    // when TServer runs as Admin on Vista and we try to attach from another account (Salamander started
    // using runas /user:test salamand.exe), Salamander refused to connect; the code was found here:
    // http://www.vistax64.com/vista-security/72588-openprocess-process_set_information-protected-processes.html#post357171
    // Manik points to the book http://www.amazon.com/gp/product/0470101555?ie=UTF8&tag=protectyourwi-20
    // (Windows Vista Security: Securing Vista Against Malicious Attacks )
    if (WindowsVistaAndLater)
    {
        TOKEN_PRIVILEGES tp;
        LUID luid;

        if (LookupPrivilegeValue(NULL,                // lookup privilege on local system
                                 L"SeDebugPrivilege", // privilege to lookup
                                 &luid))              // receives LUID of privilege
        {
            tp.PrivilegeCount = 1;
            tp.Privileges[0].Luid = luid;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            // Enable the privilege or disable all privileges.
            HANDLE currProc = GetCurrentProcess();
            HANDLE procToken;
            if (OpenProcessToken(currProc, TOKEN_ADJUST_PRIVILEGES, &procToken))
            {
                AdjustTokenPrivileges(procToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, NULL);
                CloseHandle(procToken);
            }
            CloseHandle(currProc);
        }
    }

    ConfigData.Register(Registry);
    Registry.Load();

    // Initialize the library
    InitializeWinLib();

    if (CWindow::RegisterUniversalClass(CS_DBLCLKS,
                                        0, 0, NULL, NULL, NULL,
                                        NULL, WC_TABLIST, NULL))
    {
        HICON hIcon = LoadIcon(HInstance, MAKEINTRESOURCE(IC_TSERVER_1));
        if (CWindow::RegisterUniversalClass(CS_DBLCLKS,
                                            0, 0, hIcon, NULL, NULL,
                                            NULL, WC_MAINWINDOW, NULL))
        {
            UseMaxMessagesCount = ConfigData.UseMaxMessagesCount;
            MaxMessagesCount = min(1000000, max(100, ConfigData.MaxMessagesCount));
            if (BuildFonts())
            {
                MainWindow = new CMainWindow;
                if (MainWindow != NULL)
                {
                    HMENU hMainMenu = LoadMenu(HInstance, MAKEINTRESOURCE(IDM_MAIN));
                    DWORD exStyle;
                    exStyle = ConfigData.UseToolbarCaption ? WS_EX_TOOLWINDOW : 0;
                    if (MainWindow->CreateEx(exStyle,
                                             WC_MAINWINDOW,
                                             MAINWINDOW_NAME,
                                             WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             CW_USEDEFAULT,
                                             NULL,
                                             hMainMenu,
                                             HInstance,
                                             MainWindow))
                    {
                        if (MainWindow->TaskBarAddIcon())
                        {
                            if (ConfigData.AlwaysOnTop)
                                SetWindowPos(MainWindow->HWindow, HWND_TOPMOST, 0, 0, 0, 0,
                                             SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREPOSITION);

                            if (ConfigData.MainWindowPlacement.length != 0)
                            {
                                if (ConfigData.UseToolbarCaption && ConfigData.MainWindowHidden)
                                    ConfigData.MainWindowPlacement.showCmd = SW_HIDE;
                                SetWindowPlacement(MainWindow->HWindow, &ConfigData.MainWindowPlacement);
                            }
                            else
                            {
                                // configuration does not exist in the Registry, use defaults
                                ShowWindow(MainWindow->HWindow, ConfigData.MainWindowHidden ? SW_HIDE : SW_SHOW);
                            }
                            /*
              WINDOWPLACEMENT wp;
              GetWindowPlacement(MainWindow->HWindow, &wp);
              if (ConfigData.MainWindowPlacement.length != 0)
                if (ConfigData.UseToolbarCaption && ConfigData.MainWindowHidden)
                  wp.showCmd = SW_HIDE;
              wp.rcNormalPosition.right++;
              SetWindowPlacement(MainWindow->HWindow, &wp);
              wp.rcNormalPosition.right--;
              SetWindowPlacement(MainWindow->HWindow, &wp);
*/
                            SetMessagesParent(MainWindow->HWindow);

                            if (InitializeServer(MainWindow->HWindow))
                            {
                                // Application loop
                                MSG msg;
                                while (GetMessage(&msg, NULL, 0, 0))
                                {
                                    CWindowsObject* wnd = WindowsManager.GetWindowPtr(GetActiveWindow());
                                    if (wnd == NULL || !wnd->Is(otDialog) || !IsDialogMessage(wnd->HWindow, &msg))
                                    {
                                        TranslateMessage(&msg);
                                        DispatchMessage(&msg);
                                    }
                                }

                                // save the configuration
                                Registry.Save();

                                ReleaseServer();
                            }
                        }
                        else
                        {
                            delete MainWindow;
                            MainWindow = NULL;
                        }
                    }
                    else
                    {
                        delete MainWindow;
                        MainWindow = NULL;
                    }
                }
                else
                {
                    TRACE_EW(L"Out of memory.");
                }
                DeleteFonts();
            }
            else
            {
                TRACE_EW(L"Font creation failed.");
            }
        }
    }
    ReleaseWinLib();
    TRACE_I("End.");
    return 0;
}
