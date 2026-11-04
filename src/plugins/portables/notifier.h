// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-04 | Refresh open panels after a device arrives or leaves. | The device-notification window only logged the event, so the panel kept a stale device list. | | Grok 4.7 |

/*
	Windows Portable Devices Plugin for Open Salamander
	
	Copyright (c) 2015 Milan Kase <manison@manison.cz>
	Copyright (c) 2015 Open Salamander Authors
	
	notifier.h
	Receives notification from the system when a WPD device arrives or
	is removed.
*/

#pragma once

class CWpdNotifier
{
private:
    static const WCHAR CLS_WPDNOTIFIER_W[];

    HWND m_hWnd;
    HDEVNOTIFY m_hDevNotify;
    int m_refreshPasses;

    void RegisterWndCls();
    void UnregisterWndCls();

    void CreateWnd();
    void DestroyWnd();

    void RegisterDevNotification();
    void UnregisterDevNotification();

    static LRESULT CALLBACK WndProcStatic(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
    CWpdNotifier();
    ~CWpdNotifier();
};
