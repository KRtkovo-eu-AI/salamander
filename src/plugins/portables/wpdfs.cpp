// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
	Windows Portable Devices Plugin for Open Salamander
	
	Copyright (c) 2015 Milan Kase <manison@manison.cz>
	Copyright (c) 2015 Open Salamander Authors
	
	wpdfs.cpp
	Salamander file system.
*/

#include "precomp.h"
#include "fxfs.h"
#include "wpdfs.h"
#include "wpdfsdevicelevel.h"
#include "wpdfscontentlevel.h"
#include "device.h"
#include "wpdhelpers.h"
#include "config.h"
#include "globals.h"
#include "lang\lang.rh"
#include "..\shared\plugindarkmode.h"

////////////////////////////////////////////////////////////////////////////////
// CWpdFS

extern CWpdDeviceList g_oDeviceList;

PCSTR CWpdFS::SUGGESTED_NAME = SuggestedFSName;

CWpdFS::CWpdFS(CFxPluginInterfaceForFS& owner)
    : TFxPluginFSInterface(owner)
{
}

HRESULT WINAPI CWpdFS::GetChildEnumerator(
    _Out_ CFxItemEnumerator*& enumerator,
    CFxItem* parentItem,
    int level,
    bool forceRefresh)
{
    HRESULT hr;

    if (forceRefresh)
    {
        g_oDeviceList.SetForceUpdate();
    }

    if (level == 0)
    {
        auto* deviceEnumerator = new CWpdDeviceEnumerator();
        hr = deviceEnumerator->Initialize();
        if (SUCCEEDED(hr))
        {
            enumerator = deviceEnumerator;
        }
        else
        {
            deviceEnumerator->Release();
        }
    }
    else
    {
        CWpdDevice* device;
        PCWSTR parentObjectId;

        if (static_cast<CWpdItem*>(parentItem)->IsDevice())
        {
            auto* deviceItem = static_cast<CWpdDeviceItem*>(parentItem);
            device = deviceItem->GetDeviceNoAddRef();
            parentObjectId = WPD_DEVICE_OBJECT_ID;
        }
        else
        {
            auto* contentItem = static_cast<CWpdBaseContentItem*>(parentItem);
            device = contentItem->GetDeviceNoAddRef();
            parentObjectId = contentItem->GetObjectId();
        }

        CWpdBaseContentEnumerator* contentEnumerator;
        if (level == 1)
        {
            contentEnumerator = new CWpdStorageEnumerator();
        }
        else
        {
            contentEnumerator = new CWpdContentEnumerator();
        }

        hr = contentEnumerator->Initialize(device, parentObjectId);
        if (SUCCEEDED(hr))
        {
            enumerator = contentEnumerator;
        }
        else
        {
            contentEnumerator->Release();
        }
    }

    return hr;
}

CFxPluginDataInterface* WINAPI CWpdFS::CreatePluginData(CFxItemEnumerator* enumerator)
{
    // Redirect the call to the enumerator, since the enumerator knows what
    // data it needs.
    auto wpdEnum = static_cast<CWpdEnumerator*>(enumerator);
    return wpdEnum->CreatePluginData(*this);
}


static PCSTR WpdLoadStr(UINT id)
{
    return SalamanderGeneral->LoadStr(Fx::FxGetLangInstance(), id);
}

class CWpdOperationProgress;
static CWpdOperationProgress* WpdActiveOperationProgress = nullptr;

enum CWpdOperationProgressType
{
    wpdProgressCopy,
    wpdProgressMove,
    wpdProgressDelete
};

class CWpdOperationProgress
{
public:
    CWpdOperationProgress(HWND parent, CWpdOperationProgressType operationType, int totalItems)
        : m_window(nullptr),
          m_operationLabel(nullptr),
          m_text(nullptr),
          m_targetLabel(nullptr),
          m_targetText(nullptr),
          m_fileLabel(nullptr),
          m_totalLabel(nullptr),
          m_bytesLabel(nullptr),
          m_fileProgress(nullptr),
          m_totalProgress(nullptr),
          m_minimize(nullptr),
          m_pause(nullptr),
          m_cancel(nullptr),
          m_font(nullptr),
          m_canceled(false),
          m_operationType(operationType),
          m_totalItems(totalItems > 0 ? totalItems : 1),
          m_doneItems(0),
          m_filePulse(0),
          m_filePos(0),
          m_totalPos(0),
          m_doneBytes(0),
          m_currentFileBytes(0),
          m_currentFileTotal(static_cast<ULONGLONG>(-1)),
          m_totalBytes(0)
    {
        m_bytesText[0] = '\0';
        m_bytesTextDirty = true;
        RegisterWindowClass();
        BOOL useDarkMode = FALSE;
        int configType = SALCFGTYPE_NOTFOUND;
        if (SalamanderGeneral->GetConfigParameter(SALCFG_USEWINDOWSDARKMODE, &useDarkMode, sizeof(useDarkMode), &configType) &&
            configType == SALCFGTYPE_BOOL)
        {
            PluginDarkMode_SetHostPolicyAvailable(TRUE, useDarkMode);
        }

        NONCLIENTMETRICS metrics = {};
        metrics.cbSize = sizeof(metrics);
        if (::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, metrics.cbSize, &metrics, 0))
        {
            HDC dc = ::GetDC(nullptr);
            if (dc != nullptr)
            {
                metrics.lfMessageFont.lfHeight = -::MulDiv(8, ::GetDeviceCaps(dc, LOGPIXELSY), 72);
                ::ReleaseDC(nullptr, dc);
            }
            StringCchCopy(metrics.lfMessageFont.lfFaceName, _countof(metrics.lfMessageFont.lfFaceName), "MS Shell Dlg");
            m_font = ::CreateFontIndirect(&metrics.lfMessageFont);
        }

        HWND owner = SalamanderGeneral->GetMainWindowHWND();
        RECT ownerRect;
        if (owner == nullptr || !::GetWindowRect(owner, &ownerRect))
        {
            ownerRect.left = ownerRect.top = 0;
            ownerRect.right = ::GetSystemMetrics(SM_CXSCREEN);
            ownerRect.bottom = ::GetSystemMetrics(SM_CYSCREEN);
        }

        const int width = 535;
        const int height = 214;
        int x = ownerRect.left + ((ownerRect.right - ownerRect.left) - width) / 2;
        int y = ownerRect.top + ((ownerRect.bottom - ownerRect.top) - height) / 2;

        m_window = ::CreateWindowEx(
            WS_EX_APPWINDOW | WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE,
            WindowClassName(),
            WpdLoadStr(IDS_OPERATIONPROGRESS_TITLE),
            WS_POPUP | WS_CAPTION | WS_SYSMENU,
            x,
            y,
            width,
            height,
            owner,
            nullptr,
            Fx::FxGetModuleInstance(),
            this);
        if (m_window == nullptr)
        {
            return;
        }

        RECT clientRect;
        ::GetClientRect(m_window, &clientRect);
        const int topMargin = 18;
        const int bottomMargin = 10;
        const int buttonHeight = 24;
        const int buttonY = clientRect.bottom - bottomMargin - buttonHeight;

        m_operationLabel = ::CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                            14, topMargin, 50, 16, m_window, nullptr, Fx::FxGetModuleInstance(), nullptr);
        m_text = ::CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                  70, topMargin, width - 100, 16, m_window, nullptr, Fx::FxGetModuleInstance(), nullptr);
        m_targetLabel = ::CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                         14, topMargin + 20, 50, 16, m_window, nullptr, Fx::FxGetModuleInstance(), nullptr);
        m_targetText = ::CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                        70, topMargin + 20, width - 100, 16, m_window, nullptr, Fx::FxGetModuleInstance(), nullptr);
        m_fileLabel = ::CreateWindowEx(0, "STATIC", WpdLoadStr(IDS_OPERATIONPROGRESS_FILE), WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                       24, topMargin + 51, 40, 16, m_window, nullptr, Fx::FxGetModuleInstance(), nullptr);
        m_fileProgress = ::CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                          70, topMargin + 48, width - 100, 20, m_window, nullptr, Fx::FxGetModuleInstance(), nullptr);
        m_totalLabel = ::CreateWindowEx(0, "STATIC", WpdLoadStr(IDS_OPERATIONPROGRESS_TOTAL), WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                        24, topMargin + 77, 40, 16, m_window, nullptr, Fx::FxGetModuleInstance(), nullptr);
        m_totalProgress = ::CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                           70, topMargin + 74, width - 100, 20, m_window, nullptr, Fx::FxGetModuleInstance(), nullptr);
        m_bytesLabel = ::CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
                                        70, topMargin + 100, width - 100, 16, m_window, nullptr, Fx::FxGetModuleInstance(), nullptr);
        m_minimize = ::CreateWindowEx(0, "BUTTON", WpdLoadStr(IDS_OPERATIONPROGRESS_MINIMIZE), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                                      140, buttonY, 74, buttonHeight, m_window, reinterpret_cast<HMENU>(IDOK), Fx::FxGetModuleInstance(), nullptr);
        m_pause = ::CreateWindowEx(0, "BUTTON", WpdLoadStr(IDS_OPERATIONPROGRESS_PAUSE), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                                   230, buttonY, 74, buttonHeight, m_window, nullptr, Fx::FxGetModuleInstance(), nullptr);
        m_cancel = ::CreateWindowEx(0, "BUTTON", WpdLoadStr(IDS_OPERATIONPROGRESS_CANCEL), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
                                    320, buttonY, 74, buttonHeight, m_window, reinterpret_cast<HMENU>(IDCANCEL), Fx::FxGetModuleInstance(), nullptr);
        ::EnableWindow(m_pause, FALSE);
        ApplyFont();

        ApplyTheme();
        WpdActiveOperationProgress = this;
        Step(WpdLoadStr(IDS_OPERATIONPROGRESS_PREPARING));
        ::ShowWindow(m_window, SW_SHOWNORMAL);
        ::UpdateWindow(m_window);
        PumpMessages();
    }

    ~CWpdOperationProgress()
    {
        Close();
        if (m_font != nullptr)
        {
            ::DeleteObject(m_font);
            m_font = nullptr;
        }
    }

    bool Step(PCSTR sourceName, PCSTR targetName = nullptr)
    {
        if (m_window == nullptr)
        {
            return true;
        }

        if (targetName != nullptr && targetName[0] != '\0')
        {
            char targetText[MAX_PATH + 128];
            ::SetWindowText(m_operationLabel, GetOperationVerb());
            ::SetWindowText(m_text, sourceName);
            ::SetWindowText(m_targetLabel, WpdLoadStr(IDS_OPERATIONPROGRESS_TO));
            StringCchPrintf(targetText, _countof(targetText), "%s", targetName);
            ::SetWindowText(m_targetText, targetText);
        }
        else
        {
            char text[MAX_PATH + 128];
            StringCchPrintf(text, _countof(text), "%s %s", GetOperationVerb(), sourceName);
            ::SetWindowText(m_operationLabel, "");
            ::SetWindowText(m_text, text);
            ::SetWindowText(m_targetLabel, "");
            ::SetWindowText(m_targetText, "");
        }
        m_filePulse = 0;
        m_filePos = 0;
        m_currentFileBytes = 0;
        m_currentFileTotal = static_cast<ULONGLONG>(-1);
        m_totalPos = m_doneItems * 1000 / m_totalItems;
        ::ShowWindow(m_window, SW_SHOWNORMAL);
        UpdateTitle();
        UpdateBytesText();
        Redraw();
        PumpMessages();
        return !m_canceled;
    }

    bool Advance()
    {
        ++m_doneItems;
        if (m_window != nullptr)
        {
            if (m_currentFileTotal != static_cast<ULONGLONG>(-1))
            {
                m_doneBytes += m_currentFileTotal;
            }
            else
            {
                m_doneBytes += m_currentFileBytes;
            }
            m_filePulse = 0;
            m_filePos = 0;
            m_currentFileBytes = 0;
            m_currentFileTotal = static_cast<ULONGLONG>(-1);
            m_totalPos = m_doneItems * 1000 / m_totalItems;
            UpdateTitle();
            UpdateBytesText();
            RedrawProgress();
            PumpMessages();
        }
        return !m_canceled;
    }

    bool SetFileProgress(ULONGLONG current, ULONGLONG total)
    {
        if (m_window != nullptr)
        {
            int pos;
            if (total != 0 && total != static_cast<ULONGLONG>(-1))
            {
                pos = static_cast<int>((current * 1000) / total);
                if (pos > 1000)
                {
                    pos = 1000;
                }
            }
            else
            {
                m_filePulse += 25;
                if (m_filePulse > 990)
                {
                    m_filePulse = 990;
                }
                pos = m_filePulse;
            }
            m_filePos = pos;
            m_currentFileBytes = current;
            m_currentFileTotal = total;
            m_totalPos = ((m_doneItems * 1000) + m_filePos) / m_totalItems;
            if (m_totalPos > 1000)
            {
                m_totalPos = 1000;
            }
            UpdateTitle();
            UpdateBytesText();
            RedrawProgress();
            PumpMessages();
        }
        return !m_canceled;
    }

    void SetDevice(CWpdDevice* device)
    {
        if (device != nullptr)
        {
            device->GetName(m_deviceName);
            UpdateTitle();
        }
    }

    void AddTotalBytes(ULONGLONG bytes)
    {
        if (bytes != 0 && bytes != static_cast<ULONGLONG>(-1))
        {
            m_totalBytes += bytes;
            UpdateBytesText();
        }
    }

    void ApplyFont()
    {
        if (m_font != nullptr)
        {
            HWND controls[] = {m_operationLabel, m_text, m_targetLabel, m_targetText, m_fileLabel, m_totalLabel, m_bytesLabel, m_minimize, m_pause, m_cancel};
            for (int i = 0; i < _countof(controls); ++i)
            {
                if (controls[i] != nullptr)
                {
                    ::SendMessage(controls[i], WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
                }
            }
        }
    }

    void Close()
    {
        if (WpdActiveOperationProgress == this)
        {
            WpdActiveOperationProgress = nullptr;
        }
        if (m_window != nullptr)
        {
            HWND window = m_window;
            m_window = nullptr;
            ::DestroyWindow(window);
            if (m_font != nullptr)
            {
                ::DeleteObject(m_font);
                m_font = nullptr;
            }
            PumpMessages();
        }
    }

    void ApplyTheme()
    {
        if (m_window == nullptr)
        {
            return;
        }
        PluginDarkMode_ApplyTitleBar(m_window);
        PluginDarkMode_ApplyListTreeThemeRecursive(m_window);
        if (PluginDarkMode_ShouldUseDark())
        {
            ::InvalidateRect(m_fileProgress, nullptr, TRUE);
            ::InvalidateRect(m_totalProgress, nullptr, TRUE);
        }
        InvalidateRect(m_window, nullptr, TRUE);
        Redraw();
    }

private:
    PCSTR GetOperationName() const
    {
        switch (m_operationType)
        {
        case wpdProgressCopy:
            return WpdLoadStr(IDS_OPERATIONPROGRESS_COPY);
        case wpdProgressMove:
            return WpdLoadStr(IDS_OPERATIONPROGRESS_MOVE);
        case wpdProgressDelete:
            return WpdLoadStr(IDS_OPERATIONPROGRESS_DELETE);
        default:
            return "";
        }
    }

    PCSTR GetOperationVerb() const
    {
        switch (m_operationType)
        {
        case wpdProgressCopy:
            return WpdLoadStr(IDS_OPERATIONPROGRESS_COPYING);
        case wpdProgressMove:
            return WpdLoadStr(IDS_OPERATIONPROGRESS_MOVING);
        case wpdProgressDelete:
            return WpdLoadStr(IDS_OPERATIONPROGRESS_DELETING);
        default:
            return "";
        }
    }

    PCSTR GetBytesFormat() const
    {
        switch (m_operationType)
        {
        case wpdProgressMove:
            return WpdLoadStr(IDS_OPERATIONPROGRESS_MOVED);
        case wpdProgressDelete:
            return WpdLoadStr(IDS_OPERATIONPROGRESS_PROCESSED);
        case wpdProgressCopy:
        default:
            return WpdLoadStr(IDS_OPERATIONPROGRESS_BYTES);
        }
    }

    static PCSTR WindowClassName()
    {
        return "OpenSalamanderWpdOperationProgress";
    }

    static void RegisterWindowClass()
    {
        static ATOM atom = 0;
        if (atom != 0)
        {
            return;
        }

        WNDCLASS wc = {};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = Fx::FxGetModuleInstance();
        wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        wc.lpszClassName = WindowClassName();
        atom = ::RegisterClass(&wc);
    }

    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        CWpdOperationProgress* self = reinterpret_cast<CWpdOperationProgress*>(::GetWindowLongPtr(window, GWLP_USERDATA));
        if (message == WM_NCCREATE)
        {
            auto createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
            self = reinterpret_cast<CWpdOperationProgress*>(createStruct->lpCreateParams);
            ::SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (self != nullptr)
        {
            if (PluginDarkMode_HandleThemeMessage(window, message, lParam))
            {
                self->ApplyTheme();
                return 0;
            }
            LRESULT brush = 0;
            if (PluginDarkMode_HandleCtlColor(message, wParam, lParam, &brush))
            {
                return brush;
            }
            if (message == WM_CTLCOLORSTATIC && PluginDarkMode_ShouldUseDark())
            {
                HDC dc = reinterpret_cast<HDC>(wParam);
                PluginDarkModeColors colors = PluginDarkMode_GetColors();
                ::SetTextColor(dc, colors.readableText);
                ::SetBkColor(dc, colors.background);
                ::SetBkMode(dc, TRANSPARENT);
                return reinterpret_cast<LRESULT>(PluginDarkMode_GetDialogCtlColorBrush(dc, message));
            }
            if (message == WM_DRAWITEM)
            {
                auto drawItem = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
                if (drawItem != nullptr &&
                    (drawItem->hwndItem == self->m_minimize || drawItem->hwndItem == self->m_pause || drawItem->hwndItem == self->m_cancel))
                {
                    self->DrawButton(drawItem);
                    return TRUE;
                }
                if (drawItem != nullptr &&
                    (drawItem->hwndItem == self->m_fileProgress || drawItem->hwndItem == self->m_totalProgress))
                {
                    self->DrawProgress(drawItem, drawItem->hwndItem == self->m_fileProgress ? self->m_filePos : self->m_totalPos);
                    return TRUE;
                }
                if (drawItem != nullptr && drawItem->hwndItem == self->m_bytesLabel)
                {
                    self->DrawBytesLabel(drawItem);
                    return TRUE;
                }
            }
            if (message == WM_ERASEBKGND && PluginDarkMode_ShouldUseDark())
            {
                RECT rect;
                GetClientRect(window, &rect);
                HBRUSH darkBrush = PluginDarkMode_GetDialogCtlColorBrush(reinterpret_cast<HDC>(wParam), WM_CTLCOLORDLG);
                if (darkBrush != nullptr)
                {
                    FillRect(reinterpret_cast<HDC>(wParam), &rect, darkBrush);
                    return 1;
                }
            }
            if (message == WM_COMMAND && LOWORD(wParam) == IDOK)
            {
                ::ShowWindow(window, SW_MINIMIZE);
                return 0;
            }
            if (message == WM_COMMAND && LOWORD(wParam) == IDCANCEL)
            {
                self->m_canceled = true;
                ::EnableWindow(self->m_cancel, FALSE);
                return 0;
            }
            if (message == WM_CLOSE)
            {
                self->m_canceled = true;
                ::EnableWindow(self->m_cancel, FALSE);
                return 0;
            }
        }
        return ::DefWindowProc(window, message, wParam, lParam);
    }

    void Redraw()
    {
        if (m_window == nullptr)
        {
            return;
        }

        HWND controls[] = {m_operationLabel, m_text, m_targetLabel, m_targetText, m_fileLabel, m_totalLabel, m_bytesLabel, m_fileProgress, m_totalProgress, m_minimize, m_pause, m_cancel};
        for (int i = 0; i < _countof(controls); ++i)
        {
            if (controls[i] != nullptr)
            {
                ::RedrawWindow(controls[i], nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
            }
        }
    }

    void UpdateTitle()
    {
        if (m_window == nullptr)
        {
            return;
        }

        char title[MAX_PATH + 64];
        if (!m_deviceName.IsEmpty())
        {
            StringCchPrintf(title, _countof(title), "(%d %%) %s - %s", m_totalPos / 10, GetOperationName(), m_deviceName.GetString());
        }
        else
        {
            StringCchPrintf(title, _countof(title), "(%d %%) %s", m_totalPos / 10, GetOperationName());
        }
        ::SetWindowText(m_window, title);
    }

    void UpdateBytesText()
    {
        if (m_bytesLabel == nullptr)
        {
            return;
        }

        ULONGLONG copied = m_doneBytes + m_currentFileBytes;
        ULONGLONG total = m_totalBytes;
        if (total == 0 && m_currentFileTotal != static_cast<ULONGLONG>(-1))
        {
            total = m_doneBytes + m_currentFileTotal;
        }
        if (total < copied)
        {
            total = copied;
        }
        if (copied == 0 && total == 0)
        {
            if (m_bytesText[0] != '\0')
            {
                m_bytesText[0] = '\0';
                m_bytesTextDirty = true;
            }
            return;
        }

        CQuadWord copiedSize;
        CQuadWord totalSize;
        copiedSize.SetUI64(copied);
        totalSize.SetUI64(total);

        char copiedText[64];
        char totalText[64];
        char copiedFormatted[64];
        char totalFormatted[64];
        SalamanderGeneral->PrintDiskSize(copiedFormatted, copiedSize, 1);
        SalamanderGeneral->PrintDiskSize(totalFormatted, totalSize, 1);
        ExtractSizeInParentheses(copiedFormatted, copiedText, _countof(copiedText));
        ExtractSizeInParentheses(totalFormatted, totalText, _countof(totalText));

        char text[160];
        StringCchPrintf(text, _countof(text), GetBytesFormat(), copiedText, totalText);
        if (lstrcmp(m_bytesText, text) != 0)
        {
            StringCchCopy(m_bytesText, _countof(m_bytesText), text);
            m_bytesTextDirty = true;
        }
    }

    static void ExtractSizeInParentheses(PCSTR text, char* buffer, int bufferSize)
    {
        const char* begin = text != nullptr ? strrchr(text, '(') : nullptr;
        const char* end = begin != nullptr ? strchr(begin + 1, ')') : nullptr;
        if (begin != nullptr && end != nullptr && end > begin + 1)
        {
            size_t capacity = static_cast<size_t>(bufferSize - 1);
            size_t len = static_cast<size_t>(end - begin - 1);
            if (len > capacity)
            {
                len = capacity;
            }
            memcpy(buffer, begin + 1, len);
            buffer[len] = '\0';
        }
        else
        {
            StringCchCopy(buffer, bufferSize, text != nullptr ? text : "");
        }
    }

    void RedrawProgress()
    {
        if (m_window == nullptr)
        {
            return;
        }

        HWND controls[] = {m_fileProgress, m_totalProgress};
        for (int i = 0; i < _countof(controls); ++i)
        {
            if (controls[i] != nullptr)
            {
                ::RedrawWindow(controls[i], nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
            }
        }
        if (m_bytesTextDirty && m_bytesLabel != nullptr)
        {
            ::RedrawWindow(m_bytesLabel, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
            m_bytesTextDirty = false;
        }
    }

    void DrawButton(DRAWITEMSTRUCT* drawItem)
    {
        bool dark = PluginDarkMode_ShouldUseDark();
        PluginDarkModeColors colors = PluginDarkMode_GetColors();
        COLORREF background = dark ? RGB(0x2B, 0x2B, 0x2B) : ::GetSysColor(COLOR_BTNFACE);
        COLORREF border = dark ? RGB(0x80, 0x80, 0x80) : ::GetSysColor(COLOR_BTNSHADOW);
        COLORREF text = dark ? colors.readableText : ::GetSysColor(COLOR_BTNTEXT);
        if ((drawItem->itemState & ODS_DISABLED) != 0)
        {
            text = dark ? RGB(0x80, 0x80, 0x80) : ::GetSysColor(COLOR_GRAYTEXT);
        }
        if ((drawItem->itemState & ODS_SELECTED) != 0)
        {
            background = dark ? RGB(0x3A, 0x3A, 0x3A) : ::GetSysColor(COLOR_3DLIGHT);
        }

        HBRUSH brush = ::CreateSolidBrush(background);
        ::FillRect(drawItem->hDC, &drawItem->rcItem, brush);
        ::DeleteObject(brush);
        HPEN pen = ::CreatePen(PS_SOLID, 1, border);
        HGDIOBJ oldPen = ::SelectObject(drawItem->hDC, pen);
        HGDIOBJ oldBrush = ::SelectObject(drawItem->hDC, ::GetStockObject(NULL_BRUSH));
        ::Rectangle(drawItem->hDC, drawItem->rcItem.left, drawItem->rcItem.top, drawItem->rcItem.right, drawItem->rcItem.bottom);
        ::SelectObject(drawItem->hDC, oldBrush);
        ::SelectObject(drawItem->hDC, oldPen);
        ::DeleteObject(pen);

        char textBuffer[128];
        ::GetWindowText(drawItem->hwndItem, textBuffer, _countof(textBuffer));
        HGDIOBJ oldFont = nullptr;
        if (m_font != nullptr)
        {
            oldFont = ::SelectObject(drawItem->hDC, m_font);
        }
        ::SetBkMode(drawItem->hDC, TRANSPARENT);
        ::SetTextColor(drawItem->hDC, text);
        RECT textRect = drawItem->rcItem;
        ::DrawText(drawItem->hDC, textBuffer, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (oldFont != nullptr)
        {
            ::SelectObject(drawItem->hDC, oldFont);
        }
        if ((drawItem->itemState & ODS_FOCUS) != 0)
        {
            RECT focusRect = drawItem->rcItem;
            ::InflateRect(&focusRect, -3, -3);
            ::DrawFocusRect(drawItem->hDC, &focusRect);
        }
    }

    void DrawBytesLabel(DRAWITEMSTRUCT* drawItem)
    {
        RECT targetRect = drawItem->rcItem;
        const int width = targetRect.right - targetRect.left;
        const int height = targetRect.bottom - targetRect.top;
        RECT rect = targetRect;
        HDC dc = drawItem->hDC;
        HDC memDC = ::CreateCompatibleDC(dc);
        HBITMAP bitmap = memDC != nullptr ? ::CreateCompatibleBitmap(dc, width, height) : nullptr;
        HGDIOBJ oldBitmap = nullptr;
        if (memDC != nullptr && bitmap != nullptr)
        {
            oldBitmap = ::SelectObject(memDC, bitmap);
            dc = memDC;
            ::SetRect(&rect, 0, 0, width, height);
        }

        bool dark = PluginDarkMode_ShouldUseDark();
        PluginDarkModeColors colors = PluginDarkMode_GetColors();
        COLORREF background = dark ? colors.background : ::GetSysColor(COLOR_BTNFACE);
        COLORREF text = dark ? colors.readableText : ::GetSysColor(COLOR_BTNTEXT);

        HBRUSH brush = ::CreateSolidBrush(background);
        ::FillRect(dc, &rect, brush);
        ::DeleteObject(brush);

        HGDIOBJ oldFont = nullptr;
        if (m_font != nullptr)
        {
            oldFont = ::SelectObject(dc, m_font);
        }
        ::SetBkMode(dc, TRANSPARENT);
        ::SetTextColor(dc, text);
        RECT textRect = rect;
        ::DrawText(dc, m_bytesText, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        if (oldFont != nullptr)
        {
            ::SelectObject(dc, oldFont);
        }

        if (memDC != nullptr && bitmap != nullptr)
        {
            ::BitBlt(drawItem->hDC, targetRect.left, targetRect.top, width, height, memDC, 0, 0, SRCCOPY);
            ::SelectObject(memDC, oldBitmap);
            ::DeleteObject(bitmap);
            ::DeleteDC(memDC);
        }
        else if (memDC != nullptr)
        {
            ::DeleteDC(memDC);
        }
    }

    void DrawProgress(DRAWITEMSTRUCT* drawItem, int pos)
    {
        RECT targetRect = drawItem->rcItem;
        const int width = targetRect.right - targetRect.left;
        const int height = targetRect.bottom - targetRect.top;
        RECT rect = targetRect;
        HDC dc = drawItem->hDC;
        HDC memDC = ::CreateCompatibleDC(dc);
        HBITMAP bitmap = memDC != nullptr ? ::CreateCompatibleBitmap(dc, width, height) : nullptr;
        HGDIOBJ oldBitmap = nullptr;
        if (memDC != nullptr && bitmap != nullptr)
        {
            oldBitmap = ::SelectObject(memDC, bitmap);
            dc = memDC;
            ::SetRect(&rect, 0, 0, width, height);
        }

        bool dark = PluginDarkMode_ShouldUseDark();
        PluginDarkModeColors colors = PluginDarkMode_GetColors();
        COLORREF background = dark ? RGB(0x2B, 0x2B, 0x2B) : RGB(0xF0, 0xF0, 0xF0);
        COLORREF border = dark ? RGB(0x5A, 0x5A, 0x5A) : RGB(0x80, 0x80, 0x80);
        COLORREF bar = RGB(0x00, 0x78, 0xD7);
        COLORREF text = dark ? colors.readableText : RGB(0, 0, 0);

        HBRUSH brush = ::CreateSolidBrush(background);
        ::FillRect(dc, &rect, brush);
        ::DeleteObject(brush);

        RECT fill = rect;
        ::InflateRect(&fill, -1, -1);
        if (pos < 0)
        {
            pos = 0;
        }
        if (pos > 1000)
        {
            pos = 1000;
        }
        fill.right = fill.left + ((fill.right - fill.left) * pos) / 1000;
        if (fill.right > fill.left)
        {
            brush = ::CreateSolidBrush(bar);
            ::FillRect(dc, &fill, brush);
            ::DeleteObject(brush);
        }

        HPEN pen = ::CreatePen(PS_SOLID, 1, border);
        HGDIOBJ oldPen = ::SelectObject(dc, pen);
        HGDIOBJ oldBrush = ::SelectObject(dc, ::GetStockObject(NULL_BRUSH));
        ::Rectangle(dc, rect.left, rect.top, rect.right, rect.bottom);
        ::SelectObject(dc, oldBrush);
        ::SelectObject(dc, oldPen);
        ::DeleteObject(pen);

        char percent[16];
        StringCchPrintf(percent, _countof(percent), "%d %%", pos / 10);
        HGDIOBJ oldFont = nullptr;
        if (m_font != nullptr)
        {
            oldFont = ::SelectObject(dc, m_font);
        }
        ::SetBkMode(dc, TRANSPARENT);
        ::SetTextColor(dc, text);
        ::DrawText(dc, percent, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (oldFont != nullptr)
        {
            ::SelectObject(dc, oldFont);
        }

        if (memDC != nullptr && bitmap != nullptr)
        {
            ::BitBlt(drawItem->hDC, targetRect.left, targetRect.top, width, height, memDC, 0, 0, SRCCOPY);
            ::SelectObject(memDC, oldBitmap);
            ::DeleteObject(bitmap);
            ::DeleteDC(memDC);
        }
        else if (memDC != nullptr)
        {
            ::DeleteDC(memDC);
        }
    }

    void PumpMessages()
    {
        MSG msg;
        // Dispatch only messages for the progress window and its children. Pumping
        // the whole thread here can re-enter Salamander while it is inside this
        // plug-in call and invalidate panel items used by the active operation.
        while (m_window != nullptr && ::PeekMessage(&msg, m_window, 0, 0, PM_REMOVE))
        {
            // WM_QUIT is returned regardless of the window filter. Preserve it for
            // Salamander's outer message loop instead of silently consuming it here.
            if (msg.message == WM_QUIT)
            {
                ::PostQuitMessage(static_cast<int>(msg.wParam));
                break;
            }
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

    HWND m_window;
    HWND m_operationLabel;
    HWND m_text;
    HWND m_targetLabel;
    HWND m_targetText;
    HWND m_fileLabel;
    HWND m_totalLabel;
    HWND m_bytesLabel;
    HWND m_fileProgress;
    HWND m_totalProgress;
    HWND m_minimize;
    HWND m_pause;
    HWND m_cancel;
    HFONT m_font;
    bool m_canceled;
    CWpdOperationProgressType m_operationType;
    CFxString m_deviceName;
    int m_totalItems;
    int m_doneItems;
    int m_filePulse;
    int m_filePos;
    int m_totalPos;
    ULONGLONG m_doneBytes;
    ULONGLONG m_currentFileBytes;
    ULONGLONG m_currentFileTotal;
    ULONGLONG m_totalBytes;
    char m_bytesText[160];
    bool m_bytesTextDirty;
};

static void WINAPI WpdAddSelectedPanelTotalBytes(CWpdOperationProgress& progress, int panel, BOOL focused)
{
    int index = 0;
    for (;;)
    {
        BOOL isDir = FALSE;
        const CFileData* file = focused ? SalamanderGeneral->GetPanelFocusedItem(panel, &isDir) : SalamanderGeneral->GetPanelSelectedItem(panel, &index, &isDir);
        if (file == nullptr)
        {
            break;
        }
        progress.AddTotalBytes(file->Size.Value);
        if (focused)
        {
            break;
        }
    }
}

static HRESULT WINAPI WpdCopyStream(IStream* source, IStream* target, ULONGLONG size)
{
    _ASSERTE(source != nullptr);
    _ASSERTE(target != nullptr);

    BYTE buffer[64 * 1024];
    ULONGLONG copied = 0;
    for (;;)
    {
        if (size != static_cast<ULONGLONG>(-1) && copied >= size)
        {
            return S_OK;
        }

        ULONG toRead = sizeof(buffer);

        ULONG read = 0;
        HRESULT hr = source->Read(buffer, toRead, &read);
        if (FAILED(hr))
        {
            return hr;
        }
        if (read == 0)
        {
            if (WpdActiveOperationProgress != nullptr)
            {
                WpdActiveOperationProgress->SetFileProgress(copied, copied != 0 ? copied : 1);
            }
            return S_OK;
        }

        ULONG toWrite = read;
        if (size != static_cast<ULONGLONG>(-1) && copied + toWrite > size)
        {
            toWrite = static_cast<ULONG>(size - copied);
        }

        ULONG writtenTotal = 0;
        while (writtenTotal < toWrite)
        {
            ULONG written = 0;
            hr = target->Write(buffer + writtenTotal, toWrite - writtenTotal, &written);
            if (FAILED(hr))
            {
                return hr;
            }
            if (written == 0)
            {
                return STG_E_MEDIUMFULL;
            }
            writtenTotal += written;
        }

        copied += toWrite;
        if (WpdActiveOperationProgress != nullptr &&
            !WpdActiveOperationProgress->SetFileProgress(copied, size))
        {
            return HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
        if (hr == S_FALSE)
        {
            if (WpdActiveOperationProgress != nullptr)
            {
                WpdActiveOperationProgress->SetFileProgress(copied, copied != 0 ? copied : 1);
            }
            return S_OK;
        }
    }
}

static bool WINAPI WpdIsOperationCancelled(HRESULT hr)
{
    return hr == HRESULT_FROM_WIN32(ERROR_CANCELLED);
}

static bool WINAPI WpdRequiresDeviceReconnect(HRESULT hr)
{
    return hr == HRESULT_FROM_WIN32(ERROR_SEM_TIMEOUT) ||
           hr == HRESULT_FROM_WIN32(ERROR_GEN_FAILURE) ||
           hr == HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_CONNECTED) ||
           hr == HRESULT_FROM_WIN32(ERROR_DEV_NOT_EXIST)
#ifdef ERROR_DEVICE_REINITIALIZATION_NEEDED
           || hr == HRESULT_FROM_WIN32(ERROR_DEVICE_REINITIALIZATION_NEEDED)
#endif
        ;
}

static void WINAPI WpdShowOperationError(HWND parent, PCSTR operation, PCSTR name, HRESULT hr)
{
    if (WpdIsOperationCancelled(hr))
    {
        return;
    }

    CFxString message;
    message.Format("%s '%s' failed (0x%08X).", operation, name != nullptr ? name : "", hr);
    SalamanderGeneral->ShowMessageBox(message, "Portable Devices", MSGBOX_ERROR);
}

static HRESULT WINAPI WpdObjectIdToString(PCWSTR objectId, CFxString& s)
{
    _ASSERTE(objectId != nullptr);

    int len = ::WideCharToMultiByte(CP_ACP, 0, objectId, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0)
    {
        return HRESULT_FROM_WIN32(::GetLastError());
    }

    PSTR buffer = s.GetBuffer(len);
    if (::WideCharToMultiByte(CP_ACP, 0, objectId, -1, buffer, len, nullptr, nullptr) <= 0)
    {
        HRESULT hr = HRESULT_FROM_WIN32(::GetLastError());
        s.ReleaseBuffer(0);
        return hr;
    }

    s.ReleaseBuffer();
    return S_OK;
}

static bool WINAPI WpdIsFolderContentType(const GUID& contentType)
{
    return IsEqualGUID(contentType, WPD_CONTENT_TYPE_FOLDER) ||
           IsEqualGUID(contentType, WPD_CONTENT_TYPE_FUNCTIONAL_OBJECT);
}

static HRESULT WINAPI WpdGetObjectNameAndAttributes(
    IPortableDeviceProperties* properties,
    PCWSTR objectId,
    CFxString& name,
    DWORD& attributes)
{
    static const PROPERTYKEY* const keys[] =
        {
            &WPD_OBJECT_ORIGINAL_FILE_NAME,
            &WPD_OBJECT_NAME,
            &WPD_OBJECT_CONTENT_TYPE,
        };

    ATL::CComPtr<IPortableDeviceKeyCollection> keyCollection;
    keyCollection.Attach(WpdInitKeys(keys, _countof(keys)));

    ATL::CComPtr<IPortableDeviceValues> values;
    HRESULT hr = properties->GetValues(objectId, keyCollection, &values);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = WpdGetStringValue(values, WPD_OBJECT_ORIGINAL_FILE_NAME, name);
    if (FAILED(hr) || name.IsEmpty())
    {
        hr = WpdGetStringValue(values, WPD_OBJECT_NAME, name);
    }
    if (FAILED(hr))
    {
        return hr;
    }

    GUID contentType;
    hr = values->GetGuidValue(WPD_OBJECT_CONTENT_TYPE, &contentType);
    if (FAILED(hr))
    {
        return hr;
    }

    attributes = WpdIsFolderContentType(contentType) ? FILE_ATTRIBUTE_DIRECTORY : 0;
    return S_OK;
}

static ULONGLONG WINAPI WpdGetObjectSizeOrUnknown(IPortableDeviceProperties* properties, PCWSTR objectId)
{
    static const PROPERTYKEY* const keys[] =
        {
            &WPD_OBJECT_SIZE,
        };

    ATL::CComPtr<IPortableDeviceKeyCollection> keyCollection;
    keyCollection.Attach(WpdInitKeys(keys, _countof(keys)));

    ATL::CComPtr<IPortableDeviceValues> values;
    HRESULT hr = properties->GetValues(objectId, keyCollection, &values);
    if (FAILED(hr))
    {
        return static_cast<ULONGLONG>(-1);
    }

    ULONGLONG size = 0;
    hr = values->GetUnsignedLargeIntegerValue(WPD_OBJECT_SIZE, &size);
    return SUCCEEDED(hr) ? size : static_cast<ULONGLONG>(-1);
}

static PCSTR WINAPI WpdGetUserPartFromFSPath(PCSTR fsName, PCSTR path)
{
    int fsNameLen = lstrlen(fsName);
    if (SalamanderGeneral->StrNICmp(path, fsName, fsNameLen) == 0 && path[fsNameLen] == ':')
    {
        return path + fsNameLen + 1;
    }

    return path;
}

static void WINAPI WpdStripOperationMask(PSTR userPart, int userPartSize)
{
    char* lastSlash = strrchr(userPart, '\\');
    char* lastComponent = lastSlash != nullptr ? lastSlash + 1 : userPart;
    if (strchr(lastComponent, '*') != nullptr || strchr(lastComponent, '?') != nullptr)
    {
        if (lastSlash != nullptr)
        {
            *lastSlash = '\0';
        }
        else
        {
            lstrcpyn(userPart, "\\", userPartSize);
        }
    }
}

HRESULT WINAPI CWpdFS::GetContentLocationForPath(PCSTR userPart, _Out_ CWpdDevice*& device, _Out_ CFxString& objectId)
{
    device = nullptr;
    objectId.Empty();

    CFxPath* path = CreatePath(userPart);
    HRESULT hr = path->Canonicalize();
    if (FAILED(hr))
    {
        delete path;
        return hr;
    }

    CFxPathComponentToken token;
    CFxItemEnumerator* parentEnumerator = nullptr;
    int level = 0;

    while (path->GetNextPathComponent(token))
    {
        CFxItem* parentItem = nullptr;
        if (parentEnumerator != nullptr)
        {
            parentItem = parentEnumerator->GetCurrent();
        }

        CFxItemEnumerator* childEnumerator = nullptr;
        hr = GetChildEnumerator(childEnumerator, parentItem, level, false);
        if (parentItem != nullptr)
        {
            parentItem->Release();
        }
        if (parentEnumerator != nullptr)
        {
            parentEnumerator->Release();
            parentEnumerator = nullptr;
        }
        if (FAILED(hr))
        {
            break;
        }

        bool found = false;
        while ((hr = childEnumerator->MoveNext()) == S_OK)
        {
            CFxItem* item = childEnumerator->GetCurrent();
            CFxString name;
            item->GetName(name);
            if (token.ComponentEquals(name))
            {
                found = true;
                parentEnumerator = childEnumerator;
                auto wpdItem = static_cast<CWpdItem*>(item);
                if (!wpdItem->IsDevice())
                {
                    auto contentItem = static_cast<CWpdBaseContentItem*>(item);
                    hr = WpdObjectIdToString(contentItem->GetObjectId(), objectId);
                    if (SUCCEEDED(hr))
                    {
                        if (device != nullptr)
                        {
                            device->Release();
                        }
                        device = contentItem->GetDeviceNoAddRef();
                        device->AddRef();
                    }
                }
                item->Release();
                break;
            }
            item->Release();
        }

        if (FAILED(hr))
        {
            break;
        }

        if (!found)
        {
            if (hr == S_FALSE)
            {
                hr = FX_E_PATH_NOT_FOUND;
            }
            childEnumerator->Release();
            break;
        }
        ++level;
    }

    if (parentEnumerator != nullptr)
    {
        parentEnumerator->Release();
    }
    delete path;

    if (SUCCEEDED(hr) && device == nullptr)
    {
        hr = E_NOTIMPL;
    }
    return hr;
}

HRESULT WINAPI CWpdFS::GetCurrentContentLocation(_Out_ CWpdDevice*& device, _Out_ CFxString& objectId)
{
    return GetContentLocationForPath(GetCurrentPath().GetString(), device, objectId);
}

HRESULT WINAPI CWpdFS::CreateWpdFolder(CWpdDevice* device, PCWSTR parentObjectId, PCSTR name)
{
    ATL::CComPtr<IPortableDeviceValues> values;
    HRESULT hr = values.CoCreateInstance(CLSID_PortableDeviceValues);
    if (FAILED(hr)) return hr;

    ATL::CA2W wideName(name);
    hr = values->SetStringValue(WPD_OBJECT_PARENT_ID, parentObjectId);
    if (SUCCEEDED(hr)) hr = values->SetStringValue(WPD_OBJECT_NAME, wideName);
    if (SUCCEEDED(hr)) hr = values->SetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, wideName);
    if (SUCCEEDED(hr)) hr = values->SetGuidValue(WPD_OBJECT_CONTENT_TYPE, WPD_CONTENT_TYPE_FOLDER);
    if (FAILED(hr)) return hr;

    PWSTR newObjectId = nullptr;
    hr = device->GetContentNoAddRef()->CreateObjectWithPropertiesOnly(values, &newObjectId);
    ::CoTaskMemFree(newObjectId);
    return hr;
}

HRESULT WINAPI CWpdFS::RenameWpdObject(CWpdBaseContentItem* item, PCSTR newName)
{
    CWpdDevice* device = item->GetDeviceNoAddRef();
    HRESULT hr = device->Open(GENERIC_READ | GENERIC_WRITE);
    if (FAILED(hr)) return hr;

    ATL::CComPtr<IPortableDeviceValues> values;
    hr = values.CoCreateInstance(CLSID_PortableDeviceValues);
    if (SUCCEEDED(hr))
    {
        ATL::CA2W wideName(newName);
        hr = values->SetStringValue(WPD_OBJECT_NAME, wideName);
        if (SUCCEEDED(hr)) hr = values->SetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, wideName);
        ATL::CComPtr<IPortableDeviceValues> results;
        if (SUCCEEDED(hr)) hr = device->GetPropertiesNoAddRef()->SetValues(item->GetObjectId(), values, &results);
    }
    device->Close();
    if (FAILED(hr))
    {
        return RenameWpdObjectByCopy(item, newName);
    }

    return S_OK;
}

HRESULT WINAPI CWpdFS::RenameWpdObjectByCopy(CWpdBaseContentItem* item, PCSTR newName)
{
    CWpdDevice* device = item->GetDeviceNoAddRef();
    HRESULT hr = device->Open(GENERIC_READ | GENERIC_WRITE);
    if (FAILED(hr)) return hr;

    static const PROPERTYKEY* const keys[] =
        {
            &WPD_OBJECT_PARENT_ID,
        };

    ATL::CComPtr<IPortableDeviceKeyCollection> keyCollection;
    keyCollection.Attach(WpdInitKeys(keys, _countof(keys)));

    ATL::CComPtr<IPortableDeviceValues> values;
    hr = device->GetPropertiesNoAddRef()->GetValues(item->GetObjectId(), keyCollection, &values);
    PWSTR parentObjectId = nullptr;
    if (SUCCEEDED(hr))
    {
        hr = values->GetStringValue(WPD_OBJECT_PARENT_ID, &parentObjectId);
    }

    if (SUCCEEDED(hr))
    {
        DWORD attributes = item->GetAttributes();
        if (attributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // Directory rename fallback requires recursive WPD-to-WPD copy.  If
            // property rename was ignored for a directory, report a real error
            // instead of silently leaving the item unchanged.
            hr = E_NOTIMPL;
        }
        else
        {
            char tempPath[MAX_PATH];
            char tempName[MAX_PATH];
            if (::GetTempPath(_countof(tempPath), tempPath) == 0 ||
                ::GetTempFileName(tempPath, "wpd", 0, tempName) == 0)
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());
            }
            else
            {
                hr = DownloadWpdObject(device, item->GetObjectId(), tempName);
                if (SUCCEEDED(hr))
                {
                    hr = UploadDiskObject(device, parentObjectId, tempName, newName);
                }
                if (SUCCEEDED(hr))
                {
                    ATL::CComPtr<IPortableDevicePropVariantCollection> objects;
                    hr = objects.CoCreateInstance(CLSID_PortableDevicePropVariantCollection);
                    if (SUCCEEDED(hr))
                    {
                        hr = AddWpdObjectId(objects, item->GetObjectId());
                    }
                    if (SUCCEEDED(hr))
                    {
                        ATL::CComPtr<IPortableDevicePropVariantCollection> results;
                        hr = device->GetContentNoAddRef()->Delete(PORTABLE_DEVICE_DELETE_WITH_RECURSION, objects, &results);
                    }
                }
                ::DeleteFile(tempName);
            }
        }
    }

    ::CoTaskMemFree(parentObjectId);
    device->Close();
    return hr;
}

HRESULT WINAPI CWpdFS::DownloadWpdFile(CWpdBaseContentItem* item, PCSTR targetName)
{
    CWpdDevice* device = item->GetDeviceNoAddRef();
    HRESULT hr = device->Open(GENERIC_READ);
    if (FAILED(hr)) return hr;

    ATL::CComPtr<IPortableDeviceResources> resources;
    hr = device->GetContentNoAddRef()->Transfer(&resources);
    if (SUCCEEDED(hr))
    {
        ATL::CComPtr<IStream> source;
        DWORD optimalBufferSize = 0;
        hr = resources->GetStream(item->GetObjectId(), WPD_RESOURCE_DEFAULT, STGM_READ, &optimalBufferSize, &source);
        if (SUCCEEDED(hr))
        {
            ATL::CComPtr<IStream> target;
            hr = ::SHCreateStreamOnFile(targetName, STGM_CREATE | STGM_WRITE, &target);
            if (SUCCEEDED(hr))
            {
                ULONGLONG size = WpdGetObjectSizeOrUnknown(device->GetPropertiesNoAddRef(), item->GetObjectId());
                hr = WpdCopyStream(source, target, size);
                if (SUCCEEDED(hr))
                {
                    hr = target->Commit(STGC_DEFAULT);
                }
            }
        }
    }

    device->Close();
    return hr;
}

HRESULT WINAPI CWpdFS::DownloadWpdObject(CWpdDevice* device, PCWSTR objectId, PCSTR targetName)
{
    _ASSERTE(device != nullptr);
    _ASSERTE(objectId != nullptr);
    _ASSERTE(targetName != nullptr);

    CFxString name;
    DWORD attributes;
    HRESULT hr = WpdGetObjectNameAndAttributes(device->GetPropertiesNoAddRef(), objectId, name, attributes);
    if (FAILED(hr))
    {
        return hr;
    }

    if (attributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        if (!::CreateDirectory(targetName, nullptr))
        {
            DWORD err = ::GetLastError();
            if (err != ERROR_ALREADY_EXISTS)
            {
                return HRESULT_FROM_WIN32(err);
            }
        }

        ATL::CComPtr<IEnumPortableDeviceObjectIDs> childEnum;
        hr = device->GetContentNoAddRef()->EnumObjects(0U, objectId, nullptr, &childEnum);
        if (FAILED(hr))
        {
            return hr;
        }

        for (;;)
        {
            PWSTR childObjectId = nullptr;
            ULONG fetched = 0;
            hr = childEnum->Next(1, &childObjectId, &fetched);
            if (hr != S_OK)
            {
                if (hr == S_FALSE)
                {
                    hr = S_OK;
                }
                break;
            }

            CFxString childName;
            DWORD childAttributes;
            hr = WpdGetObjectNameAndAttributes(device->GetPropertiesNoAddRef(), childObjectId, childName, childAttributes);
            if (SUCCEEDED(hr))
            {
                char childTargetName[MAX_PATH];
                lstrcpyn(childTargetName, targetName, _countof(childTargetName));
                if (!SalamanderGeneral->SalPathAppend(childTargetName, childName, _countof(childTargetName)))
                {
                    hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
                }
                else
                {
                    hr = DownloadWpdObject(device, childObjectId, childTargetName);
                }
            }

            ::CoTaskMemFree(childObjectId);
            if (FAILED(hr))
            {
                break;
            }
        }

        return hr;
    }

    ATL::CComPtr<IPortableDeviceResources> resources;
    hr = device->GetContentNoAddRef()->Transfer(&resources);
    if (FAILED(hr))
    {
        return hr;
    }

    ATL::CComPtr<IStream> source;
    DWORD optimalBufferSize = 0;
    hr = resources->GetStream(objectId, WPD_RESOURCE_DEFAULT, STGM_READ, &optimalBufferSize, &source);
    if (FAILED(hr))
    {
        return hr;
    }

    ATL::CComPtr<IStream> target;
    hr = ::SHCreateStreamOnFile(targetName, STGM_CREATE | STGM_WRITE, &target);
    if (FAILED(hr))
    {
        return hr;
    }

    ULONGLONG size = WpdGetObjectSizeOrUnknown(device->GetPropertiesNoAddRef(), objectId);
    hr = WpdCopyStream(source, target, size);
    if (SUCCEEDED(hr))
    {
        hr = target->Commit(STGC_DEFAULT);
    }

    return hr;
}

HRESULT WINAPI CWpdFS::UploadDiskFile(CWpdDevice* device, PCWSTR parentObjectId, PCSTR sourceName, PCSTR targetName)
{
    ATL::CComPtr<IStream> source;
    HRESULT hr = ::SHCreateStreamOnFile(sourceName, STGM_READ, &source);
    if (FAILED(hr)) return hr;

    STATSTG stat;
    hr = source->Stat(&stat, STATFLAG_NONAME);
    if (FAILED(hr)) return hr;

    ATL::CComPtr<IPortableDeviceValues> values;
    hr = values.CoCreateInstance(CLSID_PortableDeviceValues);
    if (FAILED(hr)) return hr;

    ATL::CA2W wideTargetName(targetName);
    hr = values->SetStringValue(WPD_OBJECT_PARENT_ID, parentObjectId);
    if (SUCCEEDED(hr)) hr = values->SetStringValue(WPD_OBJECT_NAME, wideTargetName);
    if (SUCCEEDED(hr)) hr = values->SetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, wideTargetName);
    if (SUCCEEDED(hr)) hr = values->SetGuidValue(WPD_OBJECT_CONTENT_TYPE, WPD_CONTENT_TYPE_GENERIC_FILE);
    if (SUCCEEDED(hr)) hr = values->SetGuidValue(WPD_OBJECT_FORMAT, WPD_OBJECT_FORMAT_UNSPECIFIED);
    if (SUCCEEDED(hr)) hr = values->SetUnsignedLargeIntegerValue(WPD_OBJECT_SIZE, stat.cbSize.QuadPart);
    if (FAILED(hr)) return hr;

    hr = device->Open(GENERIC_READ | GENERIC_WRITE);
    if (FAILED(hr)) return hr;

    ATL::CComPtr<IStream> target;
    DWORD optimalBufferSize = 0;
    PWSTR newObjectId = nullptr;
    hr = device->GetContentNoAddRef()->CreateObjectWithPropertiesAndData(values, &target, &optimalBufferSize, &newObjectId);
    if (SUCCEEDED(hr))
    {
        hr = WpdCopyStream(source, target, stat.cbSize.QuadPart);
        if (SUCCEEDED(hr))
        {
            hr = target->Commit(STGC_DEFAULT);
        }
    }
    ::CoTaskMemFree(newObjectId);
    device->Close();
    return hr;
}

HRESULT WINAPI CWpdFS::UploadDiskObject(CWpdDevice* device, PCWSTR parentObjectId, PCSTR sourceName, PCSTR targetName)
{
    DWORD attributes = ::GetFileAttributes(sourceName);
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        return HRESULT_FROM_WIN32(::GetLastError());
    }

    if ((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
        return UploadDiskFile(device, parentObjectId, sourceName, targetName);
    }

    HRESULT hr = device->Open(GENERIC_READ | GENERIC_WRITE);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = CreateWpdFolder(device, parentObjectId, targetName);
    if (FAILED(hr))
    {
        device->Close();
        return hr;
    }

    // Resolve the newly created child by enumerating the destination folder.
    ATL::CComPtr<IEnumPortableDeviceObjectIDs> childEnum;
    hr = device->GetContentNoAddRef()->EnumObjects(0U, parentObjectId, nullptr, &childEnum);
    if (FAILED(hr))
    {
        device->Close();
        return hr;
    }

    PWSTR childObjectId = nullptr;
    for (;;)
    {
        ULONG fetched = 0;
        hr = childEnum->Next(1, &childObjectId, &fetched);
        if (hr != S_OK)
        {
            device->Close();
            return hr == S_FALSE ? HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) : hr;
        }

        CFxString childName;
        DWORD childAttributes;
        hr = WpdGetObjectNameAndAttributes(device->GetPropertiesNoAddRef(), childObjectId, childName, childAttributes);
        if (SUCCEEDED(hr) && (childAttributes & FILE_ATTRIBUTE_DIRECTORY) && childName.Compare(targetName) == 0)
        {
            break;
        }
        ::CoTaskMemFree(childObjectId);
        childObjectId = nullptr;
        if (FAILED(hr))
        {
            device->Close();
            return hr;
        }
    }

    device->Close();

    char mask[MAX_PATH];
    lstrcpyn(mask, sourceName, _countof(mask));
    if (!SalamanderGeneral->SalPathAppend(mask, "*", _countof(mask)))
    {
        ::CoTaskMemFree(childObjectId);
        return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
    }

    WIN32_FIND_DATA findData;
    HANDLE find = ::FindFirstFile(mask, &findData);
    if (find != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (lstrcmp(findData.cFileName, ".") == 0 || lstrcmp(findData.cFileName, "..") == 0)
            {
                continue;
            }

            char childSourceName[MAX_PATH];
            lstrcpyn(childSourceName, sourceName, _countof(childSourceName));
            if (!SalamanderGeneral->SalPathAppend(childSourceName, findData.cFileName, _countof(childSourceName)))
            {
                hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
                break;
            }
            hr = UploadDiskObject(device, childObjectId, childSourceName, findData.cFileName);
        } while (SUCCEEDED(hr) && ::FindNextFile(find, &findData));

        if (SUCCEEDED(hr) && ::GetLastError() != ERROR_NO_MORE_FILES)
        {
            hr = HRESULT_FROM_WIN32(::GetLastError());
        }
        ::FindClose(find);
    }
    else if (::GetLastError() != ERROR_FILE_NOT_FOUND)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
    }

    ::CoTaskMemFree(childObjectId);
    return hr;
}

HRESULT WINAPI CWpdFS::FindWpdChildObject(CWpdDevice* device, PCWSTR parentObjectId, PCSTR childName, _Out_ PWSTR* childObjectId, _Out_opt_ DWORD* attributes)
{
    _ASSERTE(childObjectId != nullptr);
    *childObjectId = nullptr;
    if (attributes != nullptr)
    {
        *attributes = 0;
    }

    HRESULT hr = device->Open(GENERIC_READ);
    if (FAILED(hr))
    {
        return hr;
    }

    ATL::CComPtr<IEnumPortableDeviceObjectIDs> childEnum;
    hr = device->GetContentNoAddRef()->EnumObjects(0U, parentObjectId, nullptr, &childEnum);
    if (FAILED(hr))
    {
        device->Close();
        return hr;
    }

    for (;;)
    {
        PWSTR enumObjectId = nullptr;
        ULONG fetched = 0;
        hr = childEnum->Next(1, &enumObjectId, &fetched);
        if (hr != S_OK)
        {
            device->Close();
            return hr == S_FALSE ? S_FALSE : hr;
        }

        CFxString enumName;
        DWORD enumAttributes = 0;
        hr = WpdGetObjectNameAndAttributes(device->GetPropertiesNoAddRef(), enumObjectId, enumName, enumAttributes);
        if (SUCCEEDED(hr) && enumName.Compare(childName) == 0)
        {
            *childObjectId = enumObjectId;
            if (attributes != nullptr)
            {
                *attributes = enumAttributes;
            }
            device->Close();
            return S_OK;
        }

        ::CoTaskMemFree(enumObjectId);
        if (FAILED(hr))
        {
            device->Close();
            return hr;
        }
    }
}

HRESULT WINAPI CWpdFS::ConfirmAndDeleteExistingWpdObject(HWND parent, CWpdDevice* device, PCWSTR parentObjectId, PCSTR targetName, PCSTR sourceName, bool& overwriteAll, bool& skipAll, _Out_ bool& skip)
{
    skip = false;

    PWSTR existingObjectId = nullptr;
    DWORD existingAttributes = 0;
    HRESULT hr = FindWpdChildObject(device, parentObjectId, targetName, &existingObjectId, &existingAttributes);
    if (hr == S_FALSE)
    {
        return S_OK;
    }
    if (FAILED(hr))
    {
        return hr;
    }

    if (skipAll)
    {
        skip = true;
        ::CoTaskMemFree(existingObjectId);
        return S_OK;
    }

    if (!overwriteAll)
    {
        char existingInfo[64];
        StringCchCopy(existingInfo, _countof(existingInfo), (existingAttributes & FILE_ATTRIBUTE_DIRECTORY) ? "Folder" : "File");
        char sourceInfo[64];
        StringCchCopy(sourceInfo, _countof(sourceInfo), "File");
        int answer = SalamanderGeneral->DialogOverwrite(parent, BUTTONS_YESALLSKIPCANCEL, targetName, existingInfo, sourceName, sourceInfo);
        if (answer == DIALOG_ALL)
        {
            overwriteAll = true;
        }
        else if (answer == DIALOG_SKIPALL)
        {
            skipAll = true;
            skip = true;
            ::CoTaskMemFree(existingObjectId);
            return S_OK;
        }
        else if (answer == DIALOG_SKIP)
        {
            skip = true;
            ::CoTaskMemFree(existingObjectId);
            return S_OK;
        }
        else if (answer != DIALOG_YES)
        {
            ::CoTaskMemFree(existingObjectId);
            return HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
    }

    ATL::CComPtr<IPortableDevicePropVariantCollection> objects;
    hr = objects.CoCreateInstance(CLSID_PortableDevicePropVariantCollection);
    if (SUCCEEDED(hr))
    {
        hr = AddWpdObjectId(objects, existingObjectId);
    }
    if (SUCCEEDED(hr))
    {
        hr = DeleteWpdObjects(device, objects);
    }
    ::CoTaskMemFree(existingObjectId);
    return hr;
}

HRESULT WINAPI CWpdFS::AddWpdObjectId(IPortableDevicePropVariantCollection* objects, PCWSTR objectId)
{
    _ASSERTE(objects != nullptr);
    _ASSERTE(objectId != nullptr);

    PROPVARIANT pv;
    PropVariantInit(&pv);
    pv.vt = VT_LPWSTR;
    pv.pwszVal = const_cast<PWSTR>(objectId);
    return objects->Add(&pv);
}

HRESULT WINAPI CWpdFS::DeleteWpdObjects(CWpdDevice* device, IPortableDevicePropVariantCollection* objects)
{
    HRESULT hr = device->Open(GENERIC_READ | GENERIC_WRITE);
    if (FAILED(hr)) return hr;

    ATL::CComPtr<IPortableDevicePropVariantCollection> results;
    hr = device->GetContentNoAddRef()->Delete(PORTABLE_DEVICE_DELETE_WITH_RECURSION, objects, &results);
    device->Close();
    return hr;
}

HRESULT WINAPI CWpdFS::CopyOrMoveWpdObjects(
    CWpdDevice* device,
    IPortableDevicePropVariantCollection* objects,
    PCWSTR destinationObjectId,
    bool copy)
{
    HRESULT hr = device->Open(GENERIC_READ | GENERIC_WRITE);
    if (FAILED(hr)) return hr;

    ATL::CComPtr<IPortableDevicePropVariantCollection> results;
    if (copy)
    {
        hr = device->GetContentNoAddRef()->Copy(objects, destinationObjectId, &results);
    }
    else
    {
        hr = device->GetContentNoAddRef()->Move(objects, destinationObjectId, &results);
    }

    device->Close();
    return hr;
}

BOOL WINAPI CWpdFS::QuickRename(const char*, int mode, HWND parent, CFileData& file, BOOL, char* newName, BOOL& cancel)
{
    cancel = FALSE;
    if (mode == 1) return FALSE;
    auto item = static_cast<CWpdBaseContentItem*>(reinterpret_cast<CFxItem*>(file.PluginData));
    HRESULT hr = RenameWpdObject(item, newName);
    if (FAILED(hr))
    {
        if (!HandleDeviceReconnectRequired(parent, hr)) WpdShowOperationError(parent, "Rename", file.Name, hr);
        return FALSE;
    }
    SalamanderGeneral->PostRefreshPanelFS(this);
    return TRUE;
}

BOOL WINAPI CWpdFS::GetPathForMainWindowTitle(const char* fsName, int mode, char* buf, int bufSize)
{
    if (buf == nullptr || bufSize <= 0)
    {
        return FALSE;
    }

    buf[0] = '\0';

    char currentPath[MAX_PATH];
    GetCurrentPath(currentPath);

    const char* lastComponent = currentPath;
    int len = lstrlen(currentPath);
    while (len > 1 && currentPath[len - 1] == '\\')
    {
        currentPath[--len] = '\0';
    }

    char* lastSlash = strrchr(currentPath, '\\');
    if (lastSlash != nullptr && lastSlash[1] != '\0')
    {
        lastComponent = lastSlash + 1;
    }

    if (mode == 1)
    {
        if (lastComponent[0] == '\\' && lastComponent[1] == '\0')
        {
            StringCchPrintf(buf, bufSize, "%s:%s", fsName, currentPath);
        }
        else
        {
            StringCchCopy(buf, bufSize, lastComponent);
        }
        return TRUE;
    }

    if (mode == 2)
    {
        const char* firstComponent = currentPath;
        if (firstComponent[0] == '\\')
        {
            ++firstComponent;
        }

        const char* firstSlash = strchr(firstComponent, '\\');
        if (firstSlash != nullptr && firstSlash[1] != '\0')
        {
            char rootComponent[MAX_PATH];
            size_t rootLen = firstSlash - firstComponent;
            if (rootLen >= _countof(rootComponent))
            {
                rootLen = _countof(rootComponent) - 1;
            }
            memcpy(rootComponent, firstComponent, rootLen);
            rootComponent[rootLen] = '\0';
            StringCchPrintf(buf, bufSize, "%s:\\%s\\...\\%s", fsName, rootComponent, lastComponent);
        }
        else
        {
            StringCchPrintf(buf, bufSize, "%s:%s", fsName, currentPath);
        }
        return TRUE;
    }

    return FALSE;
}

BOOL WINAPI CWpdFS::GetNextDirectoryLineHotPath(const char* text, int pathLen, int& offset)
{
    const char* end = text + pathLen;
    const char* root = text;
    while (root < end && *root != ':')
    {
        ++root;
    }
    if (root < end && *root == ':')
    {
        ++root;
        if (root < end && *root == '\\')
        {
            ++root;
        }
    }

    const char* s = text + offset;
    if (s >= end)
    {
        return FALSE;
    }
    if (s < root)
    {
        offset = static_cast<int>(root - text);
    }
    else
    {
        if (*s == '\\')
        {
            ++s;
        }
        while (s < end && *s != '\\')
        {
            ++s;
        }
        offset = static_cast<int>(s - text);
    }
    return s < end;
}

void WINAPI CWpdFS::CompleteDirectoryLineHotPath(char* path, int pathBufSize)
{
    if (path == nullptr || pathBufSize <= 0)
    {
        return;
    }

    int len = lstrlen(path);
    while (len > 1 && path[len - 1] == '\\')
    {
        path[--len] = '\0';
    }
}

BOOL WINAPI CWpdFS::HandleDeviceReconnectRequired(HWND parent, HRESULT hr)
{
    if (!WpdRequiresDeviceReconnect(hr))
    {
        return FALSE;
    }

    SalamanderGeneral->ShowMessageBox(WpdLoadStr(IDS_RECONNECT_DEVICE_REQUIRED), WpdLoadStr(IDS_OPERATIONPROGRESS_TITLE), MSGBOX_ERROR);
    ChangeDirectory("\\");
    SalamanderGeneral->PostRefreshPanelFS(this);
    return TRUE;
}

BOOL WINAPI CWpdFS::CreateDir(const char*, int mode, HWND parent, char* newName, BOOL& cancel)
{
    cancel = FALSE;
    if (mode == 1) return FALSE;

    CWpdDevice* device = nullptr;
    CFxString objectId;
    HRESULT hr = GetCurrentContentLocation(device, objectId);
    if (SUCCEEDED(hr))
    {
        hr = device->Open(GENERIC_READ | GENERIC_WRITE);
        if (SUCCEEDED(hr))
        {
            ATL::CA2W wideObjectId(objectId);
            hr = CreateWpdFolder(device, wideObjectId, newName);
            device->Close();
        }
        device->Release();
    }
    if (FAILED(hr))
    {
        if (!HandleDeviceReconnectRequired(parent, hr)) WpdShowOperationError(parent, "Create folder", newName, hr);
        return FALSE;
    }
    SalamanderGeneral->PostRefreshPanelFS(this);
    return TRUE;
}

void WINAPI CWpdFS::ViewFile(const char* fsName, HWND parent, CSalamanderForViewFileOnFSAbstract* salamander, CFileData& file)
{
    char uniqueFileName[2 * MAX_PATH];
    lstrcpyn(uniqueFileName, fsName, _countof(uniqueFileName));
    StringCchCat(uniqueFileName, _countof(uniqueFileName), ":");
    char currentPath[MAX_PATH];
    GetCurrentPath(currentPath);
    StringCchCat(uniqueFileName, _countof(uniqueFileName), currentPath);
    if (!SalamanderGeneral->SalPathAppend(uniqueFileName, file.Name, _countof(uniqueFileName)))
    {
        WpdShowOperationError(parent, "View", file.Name, HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE));
        return;
    }

    char nameInCache[MAX_PATH];
    lstrcpyn(nameInCache, file.Name, _countof(nameInCache));
    SalamanderGeneral->SalMakeValidFileNameComponent(nameInCache);

    BOOL fileExists = FALSE;
    const char* tmpFileName = salamander->AllocFileNameInCache(parent, uniqueFileName, nameInCache, nullptr, fileExists);
    if (tmpFileName == nullptr)
    {
        return;
    }

    BOOL newFileOK = FALSE;
    CQuadWord newFileSize(0, 0);
    if (!fileExists)
    {
        auto item = static_cast<CWpdBaseContentItem*>(reinterpret_cast<CFxItem*>(file.PluginData));
        HRESULT hr = DownloadWpdFile(item, tmpFileName);
        if (SUCCEEDED(hr))
        {
            newFileOK = TRUE;
            ULONGLONG size = 0;
            if (SUCCEEDED(item->GetSize(size)))
            {
                newFileSize.SetUI64(size);
            }
        }
        else
        {
            if (!HandleDeviceReconnectRequired(parent, hr)) WpdShowOperationError(parent, "View", file.Name, hr);
        }
    }

    HANDLE fileLock = nullptr;
    BOOL fileLockOwner = FALSE;
    if ((fileExists || newFileOK) && !salamander->OpenViewer(parent, tmpFileName, &fileLock, &fileLockOwner))
    {
        fileLock = nullptr;
        fileLockOwner = FALSE;
    }

    salamander->FreeFileNameInCache(uniqueFileName, fileExists, newFileOK, newFileSize, fileLock, fileLockOwner, FALSE);
}

BOOL WINAPI CWpdFS::TryShellContextMenu(const char* fsName, HWND parent, int menuX, int menuY, int panel)
{
    UNREFERENCED_PARAMETER(fsName);
    BOOL focusIsDir = FALSE;
    const CFileData* focused = SalamanderGeneral->GetPanelFocusedItem(panel, &focusIsDir);
    UNREFERENCED_PARAMETER(focusIsDir);
    if (focused == nullptr)
    {
        return FALSE;
    }

    char currentPath[MAX_PATH];
    GetCurrentPath(currentPath);

    char fullPortablePath[2 * MAX_PATH];
    lstrcpyn(fullPortablePath, currentPath, _countof(fullPortablePath));
    if (!SalamanderGeneral->SalPathAppend(fullPortablePath, focused->Name, _countof(fullPortablePath)))
    {
        return FALSE;
    }

    char components[16][MAX_PATH];
    int componentCount = 0;
    char* context = nullptr;
    char* token = strtok_s(fullPortablePath, "\\", &context);
    while (token != nullptr && componentCount < _countof(components))
    {
        if (token[0] != '\0')
        {
            lstrcpyn(components[componentCount++], token, _countof(components[0]));
        }
        token = strtok_s(nullptr, "\\", &context);
    }
    if (componentCount == 0)
    {
        return FALSE;
    }

    auto findChildByDisplayName = [](IShellFolder* folder, HWND enumParent, PCSTR name, _Out_ LPITEMIDLIST* childPidl) -> HRESULT
    {
        *childPidl = nullptr;
        ATL::CComPtr<IEnumIDList> childEnum;
        HRESULT hr = folder->EnumObjects(enumParent, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, &childEnum);
        if (FAILED(hr))
        {
            return hr;
        }

        for (;;)
        {
            LPITEMIDLIST enumPidl = nullptr;
            ULONG fetched = 0;
            hr = childEnum->Next(1, &enumPidl, &fetched);
            if (hr != S_OK)
            {
                return S_FALSE;
            }

            char displayName[MAX_PATH];
            STRRET strret;
            hr = folder->GetDisplayNameOf(enumPidl, SHGDN_NORMAL, &strret);
            if (SUCCEEDED(hr))
            {
                hr = ::StrRetToBuf(&strret, enumPidl, displayName, _countof(displayName));
            }
            if (SUCCEEDED(hr) && SalamanderGeneral->StrICmp(displayName, name) == 0)
            {
                *childPidl = enumPidl;
                return S_OK;
            }
            ::CoTaskMemFree(enumPidl);
        }
    };

    static PCWSTR const shellRoots[] =
        {
            L"::{35786D3C-B075-49b9-88DD-029876E11C01}", // Portable Devices
            L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}", // This PC / My Computer
        };

    ATL::CComPtr<IShellFolder> parentFolder;
    LPITEMIDLIST childPidl = nullptr;
    HRESULT hr = E_FAIL;
    for (int rootIndex = 0; rootIndex < _countof(shellRoots) && parentFolder == nullptr; ++rootIndex)
    {
        LPITEMIDLIST rootPidl = nullptr;
        hr = ::SHParseDisplayName(shellRoots[rootIndex], nullptr, &rootPidl, 0, nullptr);
        if (FAILED(hr) || rootPidl == nullptr)
        {
            continue;
        }

        ATL::CComPtr<IShellFolder> desktop;
        ATL::CComPtr<IShellFolder> currentFolder;
        hr = ::SHGetDesktopFolder(&desktop);
        if (SUCCEEDED(hr))
        {
            hr = desktop->BindToObject(rootPidl, nullptr, IID_IShellFolder, reinterpret_cast<void**>(&currentFolder.p));
        }
        ::CoTaskMemFree(rootPidl);
        if (FAILED(hr) || currentFolder == nullptr)
        {
            continue;
        }

        for (int component = 0; component < componentCount; ++component)
        {
            LPITEMIDLIST nextPidl = nullptr;
            hr = findChildByDisplayName(currentFolder, parent, components[component], &nextPidl);
            if (FAILED(hr) || nextPidl == nullptr)
            {
                break;
            }
            if (component == componentCount - 1)
            {
                parentFolder = currentFolder;
                childPidl = nextPidl;
                hr = S_OK;
                break;
            }

            ATL::CComPtr<IShellFolder> nextFolder;
            hr = currentFolder->BindToObject(nextPidl, nullptr, IID_IShellFolder, reinterpret_cast<void**>(&nextFolder.p));
            ::CoTaskMemFree(nextPidl);
            if (FAILED(hr) || nextFolder == nullptr)
            {
                break;
            }
            currentFolder = nextFolder;
        }
    }
    if (parentFolder == nullptr || childPidl == nullptr)
    {
        return FALSE;
    }

    LPCITEMIDLIST childPidlConst = childPidl;
    ATL::CComPtr<IContextMenu> contextMenu;
    hr = parentFolder->GetUIObjectOf(parent, 1, &childPidlConst, IID_IContextMenu, nullptr, reinterpret_cast<void**>(&contextMenu.p));
    if (FAILED(hr) || contextMenu == nullptr)
    {
        ::CoTaskMemFree(childPidl);
        return FALSE;
    }

    HMENU menu = ::CreatePopupMenu();
    if (menu == nullptr)
    {
        ::CoTaskMemFree(childPidl);
        return FALSE;
    }

    m_shellContextMenu2.Release();
    m_shellContextMenu3.Release();
    contextMenu->QueryInterface(IID_IContextMenu2, reinterpret_cast<void**>(&m_shellContextMenu2.p));
    contextMenu->QueryInterface(IID_IContextMenu3, reinterpret_cast<void**>(&m_shellContextMenu3.p));

    bool menuShown = SUCCEEDED(contextMenu->QueryContextMenu(menu, 0, 1, 0x7FFF, CMF_NORMAL | CMF_EXPLORE));
    if (menuShown)
    {
        UINT cmd = ::TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, menuX, menuY, parent, nullptr);
        if (cmd != 0)
        {
            CMINVOKECOMMANDINFO invoke = {};
            invoke.cbSize = sizeof(invoke);
            invoke.hwnd = parent;
            invoke.lpVerb = MAKEINTRESOURCEA(cmd - 1);
            invoke.nShow = SW_SHOWNORMAL;
            contextMenu->InvokeCommand(&invoke);
        }
    }

    m_shellContextMenu3.Release();
    m_shellContextMenu2.Release();
    ::DestroyMenu(menu);
    ::CoTaskMemFree(childPidl);
    return menuShown;
}

void WINAPI CWpdFS::ContextMenu(const char* fsName, HWND parent, int menuX, int menuY, int type, int panel, int selectedFiles, int selectedDirs)
{
    if (type == fscmItemsInPanel && selectedFiles + selectedDirs <= 1 && TryShellContextMenu(fsName, parent, menuX, menuY, panel))
    {
        return;
    }

    CGUIMenuPopupAbstract* menu = SalamanderGUI->CreateMenuPopup();
    if (menu == nullptr)
    {
        return;
    }

    auto insertSalamanderCommand = [menu](int& index, int command, DWORD state = 0)
    {
        char name[256];
        if (!SalamanderGeneral->GetSalamanderCommand(command, name, _countof(name), nullptr, nullptr))
        {
            return;
        }

        MENU_ITEM_INFO mi;
        mi.Mask = MENU_MASK_TYPE | MENU_MASK_STATE | MENU_MASK_ID | MENU_MASK_STRING;
        mi.Type = MENU_TYPE_STRING;
        mi.State = state;
        mi.ID = command + 1;
        mi.String = name;
        menu->InsertItem(index++, TRUE, &mi);
    };

    auto insertSeparator = [menu](int& index)
    {
        MENU_ITEM_INFO mi;
        mi.Mask = MENU_MASK_TYPE;
        mi.Type = MENU_TYPE_SEPARATOR;
        menu->InsertItem(index++, TRUE, &mi);
    };

    int index = 0;
    if (type == fscmItemsInPanel)
    {
        BOOL focusIsDir = FALSE;
        const CFileData* focused = SalamanderGeneral->GetPanelFocusedItem(panel, &focusIsDir);
        const bool hasFocusedItem = focused != nullptr;
        const bool multipleSelection = selectedFiles + selectedDirs > 1;
        const DWORD fileOnlyState = (!hasFocusedItem || focusIsDir || multipleSelection) ? MENU_STATE_GRAYED : 0;

        insertSalamanderCommand(index, SALCMD_OPEN, MENU_STATE_DEFAULT);
        insertSalamanderCommand(index, SALCMD_VIEW, fileOnlyState);
        insertSalamanderCommand(index, SALCMD_VIEWWITH, fileOnlyState);
        insertSalamanderCommand(index, SALCMD_EDIT, fileOnlyState);
        insertSalamanderCommand(index, SALCMD_EDITWITH, fileOnlyState);
        insertSeparator(index);
        insertSalamanderCommand(index, SALCMD_COPY);
        insertSalamanderCommand(index, SALCMD_MOVE);
        insertSalamanderCommand(index, SALCMD_DELETE);
        insertSalamanderCommand(index, SALCMD_QUICKRENAME, multipleSelection ? MENU_STATE_GRAYED : 0);
    }
    else if (type == fscmPathInPanel || type == fscmPanel)
    {
        insertSalamanderCommand(index, SALCMD_CREATEDIRECTORY);
        insertSeparator(index);
        insertSalamanderCommand(index, SALCMD_REFRESH);
    }

    DWORD cmd = menu->Track(MENU_TRACK_RETURNCMD | MENU_TRACK_RIGHTBUTTON | MENU_TRACK_NONOTIFY, menuX, menuY, parent, nullptr);
    if (cmd > 0)
    {
        SalamanderGeneral->PostSalamanderCommand(cmd - 1);
    }

    SalamanderGUI->DestroyMenuPopup(menu);
}

BOOL WINAPI CWpdFS::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plResult)
{
    if (m_shellContextMenu3 != nullptr)
    {
        return SUCCEEDED(m_shellContextMenu3->HandleMenuMsg2(uMsg, wParam, lParam, plResult));
    }
    if (m_shellContextMenu2 != nullptr)
    {
        return SUCCEEDED(m_shellContextMenu2->HandleMenuMsg(uMsg, wParam, lParam));
    }
    return FALSE;
}

BOOL WINAPI CWpdFS::Delete(const char*, int mode, HWND parent, int panel, int selectedFiles, int selectedDirs, BOOL& cancelOrError)
{
    cancelOrError = FALSE;
    if (mode == 1) return FALSE;

    ATL::CComPtr<IPortableDevicePropVariantCollection> objects;
    HRESULT hr = objects.CoCreateInstance(CLSID_PortableDevicePropVariantCollection);
    if (FAILED(hr))
    {
        if (!HandleDeviceReconnectRequired(parent, hr)) WpdShowOperationError(parent, "Delete", "", hr);
        cancelOrError = TRUE;
        return TRUE;
    }

    CWpdDevice* device = nullptr;
    BOOL focused = (selectedFiles == 0 && selectedDirs == 0);
    CWpdOperationProgress progress(parent, wpdProgressDelete, focused ? 1 : selectedFiles + selectedDirs);
    int index = 0;
    bool ok = true;
    for (;;)
    {
        BOOL isDir = FALSE;
        const CFileData* f = focused ? SalamanderGeneral->GetPanelFocusedItem(panel, &isDir) : SalamanderGeneral->GetPanelSelectedItem(panel, &index, &isDir);
        if (f == nullptr) break;

        auto item = static_cast<CWpdBaseContentItem*>(reinterpret_cast<CFxItem*>(f->PluginData));
        if (device == nullptr)
        {
            device = item->GetDeviceNoAddRef();
            progress.SetDevice(device);
        }
        if (!progress.Step(f->Name))
        {
            ok = false;
            break;
        }
        hr = AddWpdObjectId(objects, item->GetObjectId());
        if (FAILED(hr))
        {
            if (!HandleDeviceReconnectRequired(parent, hr)) WpdShowOperationError(parent, "Delete", f->Name, hr);
            ok = false;
            break;
        }
        if (!progress.Advance())
        {
            ok = false;
            break;
        }
        if (focused) break;
    }
    if (ok && device != nullptr)
    {
        progress.Step(WpdLoadStr(IDS_OPERATIONPROGRESS_DELETINGSELECTED));
        hr = DeleteWpdObjects(device, objects);
        if (FAILED(hr))
        {
            if (!HandleDeviceReconnectRequired(parent, hr)) WpdShowOperationError(parent, "Delete", "", hr);
            ok = false;
        }
    }
    progress.Close();
    cancelOrError = !ok;
    if (ok)
    {
        SalamanderGeneral->PostRefreshPanelFS(this);
    }
    return TRUE;
}

BOOL WINAPI CWpdFS::CopyOrMoveFromFS(
    BOOL copy,
    int mode,
    const char* fsName,
    HWND parent,
    int panel,
    int selectedFiles,
    int selectedDirs,
    char* targetPath,
    BOOL& operationMask,
    BOOL& cancelOrHandlePath,
    HWND)
{
    operationMask = FALSE;
    cancelOrHandlePath = FALSE;

    if (mode == 1)
    {
        if (*targetPath == '\0')
        {
            char path[2 * MAX_PATH];
            int targetPanel = (panel == PANEL_LEFT ? PANEL_RIGHT : PANEL_LEFT);
            int type;
            char* fs;
            if (SalamanderGeneral->GetPanelPath(targetPanel, path, _countof(path), &type, &fs))
            {
                lstrcpyn(targetPath, path, 2 * MAX_PATH);
                SalamanderGeneral->SetUserWorkedOnPanelPath(PANEL_TARGET);
            }
        }
        return FALSE;
    }

    if (mode == 4)
    {
        return FALSE;
    }

    int fsNameLen = lstrlen(fsName);
    bool isOurFsTarget = SalamanderGeneral->StrNICmp(targetPath, fsName, fsNameLen) == 0 &&
                         targetPath[fsNameLen] == ':';
    if (isOurFsTarget)
    {
        CWpdDevice* targetDevice = nullptr;
        CFxString targetObjectId;
        char targetUserPart[2 * MAX_PATH];
        lstrcpyn(targetUserPart, WpdGetUserPartFromFSPath(fsName, targetPath), _countof(targetUserPart));
        WpdStripOperationMask(targetUserPart, _countof(targetUserPart));

        HRESULT hr = GetContentLocationForPath(targetUserPart, targetDevice, targetObjectId);
        if (FAILED(hr))
        {
            if (!HandleDeviceReconnectRequired(parent, hr)) WpdShowOperationError(parent, copy ? "Copy" : "Move", targetPath, hr);
            cancelOrHandlePath = TRUE;
            return TRUE;
        }

        ATL::CA2W wideTargetObjectId(targetObjectId);
        BOOL focused = (selectedFiles == 0 && selectedDirs == 0);
        CWpdOperationProgress progress(parent, copy ? wpdProgressCopy : wpdProgressMove, focused ? 1 : selectedFiles + selectedDirs);
        progress.SetDevice(targetDevice);
        WpdAddSelectedPanelTotalBytes(progress, panel, focused);
        int index = 0;
        bool overwriteAll = false;
        bool skipAll = false;
        for (;;)
        {
            BOOL isDir = FALSE;
            const CFileData* f = focused ? SalamanderGeneral->GetPanelFocusedItem(panel, &isDir) : SalamanderGeneral->GetPanelSelectedItem(panel, &index, &isDir);
            if (f == nullptr) break;

            if (!progress.Step(f->Name, targetPath))
            {
                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                break;
            }

            char tempPath[MAX_PATH];
            char tempName[MAX_PATH];
            if (::GetTempPath(_countof(tempPath), tempPath) == 0 ||
                ::GetTempFileName(tempPath, "wpd", 0, tempName) == 0)
            {
                hr = HRESULT_FROM_WIN32(::GetLastError());
            }
            else
            {
                ::DeleteFile(tempName);
                auto item = static_cast<CWpdBaseContentItem*>(reinterpret_cast<CFxItem*>(f->PluginData));
                CWpdDevice* sourceDevice = item->GetDeviceNoAddRef();
                hr = sourceDevice->Open(GENERIC_READ);
                if (SUCCEEDED(hr))
                {
                    hr = DownloadWpdObject(sourceDevice, item->GetObjectId(), tempName);
                    sourceDevice->Close();
                }
                if (SUCCEEDED(hr))
                {
                    bool skipExisting = false;
                    hr = ConfirmAndDeleteExistingWpdObject(parent, targetDevice, wideTargetObjectId, f->Name, f->Name, overwriteAll, skipAll, skipExisting);
                    if (SUCCEEDED(hr) && !skipExisting)
                    {
                        hr = UploadDiskObject(targetDevice, wideTargetObjectId, tempName, f->Name);
                    }
                }
                if (SUCCEEDED(hr) && !copy)
                {
                    ATL::CComPtr<IPortableDevicePropVariantCollection> objects;
                    hr = objects.CoCreateInstance(CLSID_PortableDevicePropVariantCollection);
                    if (SUCCEEDED(hr)) hr = AddWpdObjectId(objects, item->GetObjectId());
                    if (SUCCEEDED(hr)) hr = DeleteWpdObjects(sourceDevice, objects);
                }
                if (isDir)
                {
                    SalamanderGeneral->ClearReadOnlyAttr(tempName);
                    SalamanderGeneral->RemoveTemporaryDir(tempName);
                }
                else
                {
                    ::DeleteFile(tempName);
                }
            }
            if (FAILED(hr))
            {
                if (!HandleDeviceReconnectRequired(parent, hr)) WpdShowOperationError(parent, copy ? "Copy" : "Move", f->Name, hr);
                break;
            }

            if (!progress.Advance())
            {
                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                break;
            }
            if (focused) break;
        }

        progress.Close();
        targetDevice->Release();
        cancelOrHandlePath = FAILED(hr);
        if (SUCCEEDED(hr))
        {
            targetPath[0] = '\0';
            SalamanderGeneral->PostRefreshPanelFS(this);
        }
        return TRUE;
    }

    // For Windows targets ask Salamander to parse the path and use its standard
    // fallback path handling.
    if (mode == 2 || mode == 5)
    {
        cancelOrHandlePath = TRUE;
        return FALSE;
    }

    if (mode == 3)
    {
        bool ok = true;
        BOOL focused = (selectedFiles == 0 && selectedDirs == 0);
        CWpdOperationProgress progress(parent, copy ? wpdProgressCopy : wpdProgressMove, focused ? 1 : selectedFiles + selectedDirs);
        WpdAddSelectedPanelTotalBytes(progress, panel, focused);
        int index = 0;
        bool progressDeviceSet = false;
        bool overwriteAll = false;
        bool skipAll = false;
        for (;;)
        {
            BOOL isDir = FALSE;
            const CFileData* f = focused ? SalamanderGeneral->GetPanelFocusedItem(panel, &isDir) : SalamanderGeneral->GetPanelSelectedItem(panel, &index, &isDir);
            if (f == nullptr) break;
            auto item = static_cast<CWpdBaseContentItem*>(reinterpret_cast<CFxItem*>(f->PluginData));
            if (!progressDeviceSet)
            {
                progress.SetDevice(item->GetDeviceNoAddRef());
                progressDeviceSet = true;
            }

            char targetName[MAX_PATH];
            lstrcpyn(targetName, targetPath, _countof(targetName));
            if (!SalamanderGeneral->SalPathAppend(targetName, f->Name, _countof(targetName)))
            {
                WpdShowOperationError(parent, copy ? "Copy" : "Move", f->Name, HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE));
                ok = false;
                break;
            }

            DWORD targetAttributes = ::GetFileAttributes(targetName);
            if (targetAttributes != INVALID_FILE_ATTRIBUTES)
            {
                if (skipAll)
                {
                    if (focused) break;
                    continue;
                }
                if (!overwriteAll)
                {
                    int answer = SalamanderGeneral->DialogOverwrite(parent, BUTTONS_YESALLSKIPCANCEL, targetName, "", f->Name, "");
                    if (answer == DIALOG_ALL)
                    {
                        overwriteAll = true;
                    }
                    else if (answer == DIALOG_SKIPALL)
                    {
                        skipAll = true;
                        if (focused) break;
                        continue;
                    }
                    else if (answer == DIALOG_SKIP)
                    {
                        if (focused) break;
                        continue;
                    }
                    else if (answer != DIALOG_YES)
                    {
                        ok = false;
                        break;
                    }
                }
            }

            if (!progress.Step(f->Name, targetName))
            {
                ok = false;
                break;
            }

            CWpdDevice* device = item->GetDeviceNoAddRef();
            HRESULT hr = device->Open(GENERIC_READ);
            if (SUCCEEDED(hr))
            {
                hr = DownloadWpdObject(device, item->GetObjectId(), targetName);
                device->Close();
            }
            if (SUCCEEDED(hr) && !copy)
            {
                ATL::CComPtr<IPortableDevicePropVariantCollection> objects;
                hr = objects.CoCreateInstance(CLSID_PortableDevicePropVariantCollection);
                if (SUCCEEDED(hr))
                {
                    hr = AddWpdObjectId(objects, item->GetObjectId());
                }
                if (SUCCEEDED(hr))
                {
                    hr = DeleteWpdObjects(item->GetDeviceNoAddRef(), objects);
                }
            }
            if (FAILED(hr))
            {
                if (!HandleDeviceReconnectRequired(parent, hr)) WpdShowOperationError(parent, copy ? "Copy" : "Move", f->Name, hr);
                ok = false;
                break;
            }

            if (!progress.Advance())
            {
                ok = false;
                break;
            }
            if (focused) break;
        }
        progress.Close();
        cancelOrHandlePath = !ok;
        if (ok)
        {
            targetPath[0] = '\0';
            SalamanderGeneral->PostRefreshPanelFS(this);
        }
        return TRUE;
    }

    targetPath[0] = '\0';
    return FALSE;
}

BOOL WINAPI CWpdFS::CopyOrMoveFromDiskToFS(
    BOOL copy,
    int mode,
    const char* fsName,
    HWND parent,
    const char* sourcePath,
    SalEnumSelection2 next,
    void* nextParam,
    int sourceFiles,
    int sourceDirs,
    char* targetPath,
    BOOL* invalidPathOrCancel)
{
    if (invalidPathOrCancel != nullptr)
    {
        *invalidPathOrCancel = FALSE;
    }

    if (mode == 1)
    {
        SalamanderGeneral->SalPathAppend(targetPath, "*.*", 2 * MAX_PATH);
        return TRUE;
    }

    if (mode != 2 && mode != 3)
    {
        return FALSE;
    }

    char targetUserPart[2 * MAX_PATH];
    lstrcpyn(targetUserPart, WpdGetUserPartFromFSPath(fsName, targetPath), _countof(targetUserPart));
    WpdStripOperationMask(targetUserPart, _countof(targetUserPart));

    CWpdDevice* targetDevice = nullptr;
    CFxString targetObjectId;
    HRESULT hr = GetContentLocationForPath(targetUserPart, targetDevice, targetObjectId);
    if (FAILED(hr))
    {
        if (!HandleDeviceReconnectRequired(parent, hr)) WpdShowOperationError(parent, copy ? "Copy" : "Move", targetPath, hr);
        if (invalidPathOrCancel != nullptr)
        {
            *invalidPathOrCancel = TRUE;
        }
        return TRUE;
    }

    ATL::CA2W wideTargetObjectId(targetObjectId);
    BOOL ok = TRUE;
    const char* name;
    const char* dosName;
    BOOL isDir;
    CQuadWord size;
    DWORD attr;
    FILETIME lastWrite;
    int errorOccured = SALENUM_SUCCESS;
    CWpdOperationProgress progress(parent, copy ? wpdProgressCopy : wpdProgressMove, sourceFiles + sourceDirs);
    progress.SetDevice(targetDevice);
    bool overwriteAll = false;
    bool skipAll = false;
    while ((name = next(parent, 0, &dosName, &isDir, &size, &attr, &lastWrite, nextParam, &errorOccured)) != nullptr)
    {
        progress.AddTotalBytes(size.Value);
        if (!progress.Step(name, targetPath))
        {
            ok = FALSE;
            break;
        }

        char sourceName[MAX_PATH];
        lstrcpyn(sourceName, sourcePath, _countof(sourceName));
        if (!SalamanderGeneral->SalPathAppend(sourceName, name, _countof(sourceName)))
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
        }
        else
        {
            const char* targetName = strrchr(name, '\\');
            targetName = targetName != nullptr ? targetName + 1 : name;
            bool skipExisting = false;
            hr = ConfirmAndDeleteExistingWpdObject(parent, targetDevice, wideTargetObjectId, targetName, sourceName, overwriteAll, skipAll, skipExisting);
            if (SUCCEEDED(hr) && !skipExisting)
            {
                hr = UploadDiskObject(targetDevice, wideTargetObjectId, sourceName, targetName);
            }
            if (SUCCEEDED(hr) && !skipExisting && !copy)
            {
                if (isDir)
                {
                    SalamanderGeneral->ClearReadOnlyAttr(sourceName);
                    SalamanderGeneral->RemoveTemporaryDir(sourceName);
                }
                else if (!::DeleteFile(sourceName))
                {
                    hr = HRESULT_FROM_WIN32(::GetLastError());
                }
            }
        }

        if (FAILED(hr))
        {
            if (!HandleDeviceReconnectRequired(parent, hr)) WpdShowOperationError(parent, copy ? "Copy" : "Move", name, hr);
            ok = FALSE;
            break;
        }
        if (!progress.Advance())
        {
            ok = FALSE;
            break;
        }
    }

    progress.Close();
    targetDevice->Release();
    if (errorOccured == SALENUM_CANCEL)
    {
        ok = FALSE;
    }
    if (invalidPathOrCancel != nullptr)
    {
        *invalidPathOrCancel = !ok;
    }
    if (ok)
    {
        SalamanderGeneral->PostRefreshPanelFS(this);
    }
    return TRUE;
}
