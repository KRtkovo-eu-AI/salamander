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

class CWpdOperationProgress
{
public:
    CWpdOperationProgress(HWND parent, PCSTR operation, int totalItems)
        : m_window(nullptr),
          m_text(nullptr),
          m_fileProgress(nullptr),
          m_totalProgress(nullptr),
          m_minimize(nullptr),
          m_pause(nullptr),
          m_cancel(nullptr),
          m_canceled(false),
          m_operation(operation),
          m_totalItems(totalItems > 0 ? totalItems : 1),
          m_doneItems(0)
    {
        INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_PROGRESS_CLASS};
        ::InitCommonControlsEx(&icc);
        RegisterWindowClass();

        HWND owner = SalamanderGeneral->GetMainWindowHWND();
        RECT ownerRect;
        if (owner == nullptr || !::GetWindowRect(owner, &ownerRect))
        {
            ownerRect.left = ownerRect.top = 0;
            ownerRect.right = ::GetSystemMetrics(SM_CXSCREEN);
            ownerRect.bottom = ::GetSystemMetrics(SM_CYSCREEN);
        }

        const int width = 535;
        const int height = 245;
        int x = ownerRect.left + ((ownerRect.right - ownerRect.left) - width) / 2;
        int y = ownerRect.top + ((ownerRect.bottom - ownerRect.top) - height) / 2;

        m_window = ::CreateWindowEx(
            WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE,
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

        m_text = ::CreateWindowEx(0, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_LEFT,
                                  24, 22, width - 48, 42, m_window, nullptr, Fx::FxGetModuleInstance(), nullptr);
        ::CreateWindowEx(0, "STATIC", WpdLoadStr(IDS_OPERATIONPROGRESS_FILE), WS_CHILD | WS_VISIBLE | SS_RIGHT,
                         24, 76, 40, 16, m_window, nullptr, Fx::FxGetModuleInstance(), nullptr);
        m_fileProgress = ::CreateWindowEx(0, PROGRESS_CLASS, "", WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
                                          70, 74, width - 90, 20, m_window, nullptr, Fx::FxGetModuleInstance(), nullptr);
        ::CreateWindowEx(0, "STATIC", WpdLoadStr(IDS_OPERATIONPROGRESS_TOTAL), WS_CHILD | WS_VISIBLE | SS_RIGHT,
                         24, 102, 40, 16, m_window, nullptr, Fx::FxGetModuleInstance(), nullptr);
        m_totalProgress = ::CreateWindowEx(0, PROGRESS_CLASS, "", WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
                                           70, 100, width - 90, 20, m_window, nullptr, Fx::FxGetModuleInstance(), nullptr);
        m_minimize = ::CreateWindowEx(0, "BUTTON", WpdLoadStr(IDS_OPERATIONPROGRESS_MINIMIZE), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                      140, 150, 74, 24, m_window, reinterpret_cast<HMENU>(IDOK), Fx::FxGetModuleInstance(), nullptr);
        m_pause = ::CreateWindowEx(0, "BUTTON", WpdLoadStr(IDS_OPERATIONPROGRESS_PAUSE), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                   230, 150, 74, 24, m_window, nullptr, Fx::FxGetModuleInstance(), nullptr);
        m_cancel = ::CreateWindowEx(0, "BUTTON", WpdLoadStr(IDS_OPERATIONPROGRESS_CANCEL), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                    320, 150, 74, 24, m_window, reinterpret_cast<HMENU>(IDCANCEL), Fx::FxGetModuleInstance(), nullptr);
        ::EnableWindow(m_pause, FALSE);

        ApplyTheme();
        ::SendMessage(m_fileProgress, PBM_SETRANGE32, 0, 1000);
        ::SendMessage(m_fileProgress, PBM_SETPOS, 0, 0);
        ::SendMessage(m_totalProgress, PBM_SETRANGE32, 0, m_totalItems);
        ::SendMessage(m_totalProgress, PBM_SETPOS, 0, 0);
        WpdActiveOperationProgress = this;
        Step(WpdLoadStr(IDS_OPERATIONPROGRESS_PREPARING));
        ::ShowWindow(m_window, SW_SHOWNORMAL);
        ::UpdateWindow(m_window);
        PumpMessages();
    }

    ~CWpdOperationProgress()
    {
        Close();
    }

    bool Step(PCSTR sourceName, PCSTR targetName = nullptr)
    {
        if (m_window == nullptr)
        {
            return true;
        }

        char text[2 * MAX_PATH + 256];
        if (targetName != nullptr && targetName[0] != '\0')
        {
            StringCchPrintf(text, _countof(text), "%sing %s\r\nto %s", m_operation, sourceName, targetName);
        }
        else
        {
            StringCchPrintf(text, _countof(text), "%sing %s", m_operation, sourceName);
        }
        ::SetWindowText(m_text, text);
        ::SendMessage(m_fileProgress, PBM_SETPOS, 0, 0);
        ::SendMessage(m_totalProgress, PBM_SETPOS, m_doneItems, 0);
        ::ShowWindow(m_window, SW_SHOWNORMAL);
        ::UpdateWindow(m_window);
        PumpMessages();
        return !m_canceled;
    }

    bool Advance()
    {
        ++m_doneItems;
        if (m_window != nullptr)
        {
            ::SendMessage(m_fileProgress, PBM_SETPOS, 0, 0);
            ::SendMessage(m_totalProgress, PBM_SETPOS, m_doneItems, 0);
            PumpMessages();
        }
        return !m_canceled;
    }

    void SetFileProgress(ULONGLONG current, ULONGLONG total)
    {
        if (m_window != nullptr && total != 0)
        {
            int pos = static_cast<int>((current * 1000) / total);
            ::SendMessage(m_fileProgress, PBM_SETPOS, pos, 0);
            ::UpdateWindow(m_window);
            PumpMessages();
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
            PluginDarkModeColors colors = PluginDarkMode_GetColors();
            ::SendMessage(m_fileProgress, PBM_SETBKCOLOR, 0, colors.background);
            ::SendMessage(m_fileProgress, PBM_SETBARCOLOR, 0, RGB(0x00, 0x78, 0xD7));
            ::SendMessage(m_totalProgress, PBM_SETBKCOLOR, 0, colors.background);
            ::SendMessage(m_totalProgress, PBM_SETBARCOLOR, 0, RGB(0x00, 0x78, 0xD7));
        }
        InvalidateRect(m_window, nullptr, TRUE);
    }

private:
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

    void PumpMessages()
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

    HWND m_window;
    HWND m_text;
    HWND m_fileProgress;
    HWND m_totalProgress;
    HWND m_minimize;
    HWND m_pause;
    HWND m_cancel;
    bool m_canceled;
    PCSTR m_operation;
    int m_totalItems;
    int m_doneItems;
};

static HRESULT WINAPI WpdCopyStream(IStream* source, IStream* target, ULONGLONG size)
{
    _ASSERTE(source != nullptr);
    _ASSERTE(target != nullptr);

    BYTE buffer[64 * 1024];
    ULONGLONG remaining = size;
    for (;;)
    {
        ULONG toRead = sizeof(buffer);
        if (remaining != static_cast<ULONGLONG>(-1) && remaining < toRead)
        {
            toRead = static_cast<ULONG>(remaining);
        }
        if (toRead == 0)
        {
            return S_OK;
        }

        ULONG read = 0;
        HRESULT hr = source->Read(buffer, toRead, &read);
        if (FAILED(hr))
        {
            return hr;
        }
        if (read == 0)
        {
            return S_OK;
        }

        ULONG writtenTotal = 0;
        while (writtenTotal < read)
        {
            ULONG written = 0;
            hr = target->Write(buffer + writtenTotal, read - writtenTotal, &written);
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

        if (WpdActiveOperationProgress != nullptr && size != static_cast<ULONGLONG>(-1))
        {
            WpdActiveOperationProgress->SetFileProgress(size - remaining + read, size);
        }
        if (remaining != static_cast<ULONGLONG>(-1))
        {
            remaining -= read;
        }
        if (hr == S_FALSE)
        {
            return S_OK;
        }
    }
}

static void WINAPI WpdShowOperationError(HWND parent, PCSTR operation, PCSTR name, HRESULT hr)
{
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
                hr = WpdCopyStream(source, target, static_cast<ULONGLONG>(-1));
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

    hr = WpdCopyStream(source, target, static_cast<ULONGLONG>(-1));
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

HRESULT WINAPI CWpdFS::ConfirmAndDeleteExistingWpdObject(HWND parent, CWpdDevice* device, PCWSTR parentObjectId, PCSTR targetName, PCSTR sourceName, _Out_ bool& skip)
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

    char existingInfo[64];
    StringCchCopy(existingInfo, _countof(existingInfo), (existingAttributes & FILE_ATTRIBUTE_DIRECTORY) ? "Folder" : "File");
    char sourceInfo[64];
    StringCchCopy(sourceInfo, _countof(sourceInfo), "File");
    int answer = SalamanderGeneral->DialogOverwrite(parent, BUTTONS_YESALLSKIPCANCEL, targetName, existingInfo, sourceName, sourceInfo);
    if (answer == DIALOG_SKIP || answer == DIALOG_SKIPALL)
    {
        skip = true;
        ::CoTaskMemFree(existingObjectId);
        return S_OK;
    }
    if (answer != DIALOG_YES && answer != DIALOG_ALL)
    {
        ::CoTaskMemFree(existingObjectId);
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
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
        WpdShowOperationError(parent, "Rename", file.Name, hr);
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
        WpdShowOperationError(parent, "Create folder", newName, hr);
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
            WpdShowOperationError(parent, "View", file.Name, hr);
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

BOOL WINAPI CWpdFS::Delete(const char*, int mode, HWND parent, int panel, int selectedFiles, int selectedDirs, BOOL& cancelOrError)
{
    cancelOrError = FALSE;
    if (mode == 1) return FALSE;

    ATL::CComPtr<IPortableDevicePropVariantCollection> objects;
    HRESULT hr = objects.CoCreateInstance(CLSID_PortableDevicePropVariantCollection);
    if (FAILED(hr))
    {
        WpdShowOperationError(parent, "Delete", "", hr);
        cancelOrError = TRUE;
        return TRUE;
    }

    CWpdDevice* device = nullptr;
    BOOL focused = (selectedFiles == 0 && selectedDirs == 0);
    CWpdOperationProgress progress(parent, "Delete", focused ? 1 : selectedFiles + selectedDirs);
    int index = 0;
    bool ok = true;
    for (;;)
    {
        BOOL isDir = FALSE;
        const CFileData* f = focused ? SalamanderGeneral->GetPanelFocusedItem(panel, &isDir) : SalamanderGeneral->GetPanelSelectedItem(panel, &index, &isDir);
        if (f == nullptr) break;
        if (!progress.Step(f->Name))
        {
            ok = false;
            break;
        }

        auto item = static_cast<CWpdBaseContentItem*>(reinterpret_cast<CFxItem*>(f->PluginData));
        if (device == nullptr)
        {
            device = item->GetDeviceNoAddRef();
        }
        hr = AddWpdObjectId(objects, item->GetObjectId());
        if (FAILED(hr))
        {
            WpdShowOperationError(parent, "Delete", f->Name, hr);
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
        progress.Step("Deleting selected items");
        hr = DeleteWpdObjects(device, objects);
        if (FAILED(hr))
        {
            WpdShowOperationError(parent, "Delete", "", hr);
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
            WpdShowOperationError(parent, copy ? "Copy" : "Move", targetPath, hr);
            cancelOrHandlePath = TRUE;
            return TRUE;
        }

        ATL::CA2W wideTargetObjectId(targetObjectId);
        BOOL focused = (selectedFiles == 0 && selectedDirs == 0);
        CWpdOperationProgress progress(parent, copy ? "Copy" : "Move", focused ? 1 : selectedFiles + selectedDirs);
        int index = 0;
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
                    hr = ConfirmAndDeleteExistingWpdObject(parent, targetDevice, wideTargetObjectId, f->Name, f->Name, skipExisting);
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
                WpdShowOperationError(parent, copy ? "Copy" : "Move", f->Name, hr);
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
        CWpdOperationProgress progress(parent, copy ? "Copy" : "Move", focused ? 1 : selectedFiles + selectedDirs);
        int index = 0;
        for (;;)
        {
            BOOL isDir = FALSE;
            const CFileData* f = focused ? SalamanderGeneral->GetPanelFocusedItem(panel, &isDir) : SalamanderGeneral->GetPanelSelectedItem(panel, &index, &isDir);
            if (f == nullptr) break;

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
                int answer = SalamanderGeneral->DialogOverwrite(parent, BUTTONS_YESALLSKIPCANCEL, targetName, "", f->Name, "");
                if (answer == DIALOG_SKIP || answer == DIALOG_SKIPALL)
                {
                    if (focused) break;
                    continue;
                }
                if (answer != DIALOG_YES && answer != DIALOG_ALL)
                {
                    ok = false;
                    break;
                }
            }

            if (!progress.Step(f->Name, targetName))
            {
                ok = false;
                break;
            }

            auto item = static_cast<CWpdBaseContentItem*>(reinterpret_cast<CFxItem*>(f->PluginData));
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
                WpdShowOperationError(parent, copy ? "Copy" : "Move", f->Name, hr);
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
        WpdShowOperationError(parent, copy ? "Copy" : "Move", targetPath, hr);
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
    CWpdOperationProgress progress(parent, copy ? "Copy" : "Move", sourceFiles + sourceDirs);
    while ((name = next(parent, 0, &dosName, &isDir, &size, &attr, &lastWrite, nextParam, &errorOccured)) != nullptr)
    {
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
            hr = ConfirmAndDeleteExistingWpdObject(parent, targetDevice, wideTargetObjectId, targetName, sourceName, skipExisting);
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
            WpdShowOperationError(parent, copy ? "Copy" : "Move", name, hr);
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
