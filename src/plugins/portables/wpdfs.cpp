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
#include "config.h"

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
        if (SUCCEEDED(hr)) hr = device->GetPropertiesNoAddRef()->SetValues(item->GetObjectId(), values, nullptr);
    }
    device->Close();
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
        }
        hr = AddWpdObjectId(objects, item->GetObjectId());
        if (FAILED(hr))
        {
            WpdShowOperationError(parent, "Delete", f->Name, hr);
            ok = false;
            break;
        }
        if (focused) break;
    }
    if (ok && device != nullptr)
    {
        hr = DeleteWpdObjects(device, objects);
        if (FAILED(hr))
        {
            WpdShowOperationError(parent, "Delete", "", hr);
            ok = false;
        }
    }
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

    if (mode == 1 || mode == 4)
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
        HRESULT hr = GetContentLocationForPath(targetPath + fsNameLen + 1, targetDevice, targetObjectId);
        if (FAILED(hr))
        {
            WpdShowOperationError(parent, copy ? "Copy" : "Move", targetPath, hr);
            cancelOrHandlePath = TRUE;
            return TRUE;
        }

        ATL::CComPtr<IPortableDevicePropVariantCollection> objects;
        hr = objects.CoCreateInstance(CLSID_PortableDevicePropVariantCollection);
        if (FAILED(hr))
        {
            targetDevice->Release();
            WpdShowOperationError(parent, copy ? "Copy" : "Move", targetPath, hr);
            cancelOrHandlePath = TRUE;
            return TRUE;
        }

        BOOL focused = (selectedFiles == 0 && selectedDirs == 0);
        int index = 0;
        for (;;)
        {
            BOOL isDir = FALSE;
            const CFileData* f = focused ? SalamanderGeneral->GetPanelFocusedItem(panel, &isDir) : SalamanderGeneral->GetPanelSelectedItem(panel, &index, &isDir);
            if (f == nullptr) break;

            auto item = static_cast<CWpdBaseContentItem*>(reinterpret_cast<CFxItem*>(f->PluginData));
            hr = AddWpdObjectId(objects, item->GetObjectId());
            if (FAILED(hr))
            {
                WpdShowOperationError(parent, copy ? "Copy" : "Move", f->Name, hr);
                break;
            }

            if (focused) break;
        }

        if (SUCCEEDED(hr))
        {
            ATL::CA2W wideTargetObjectId(targetObjectId);
            hr = CopyOrMoveWpdObjects(targetDevice, objects, wideTargetObjectId, !!copy);
            if (FAILED(hr))
            {
                WpdShowOperationError(parent, copy ? "Copy" : "Move", targetPath, hr);
            }
        }

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

    // Salamander calls mode 3 after its standard target-path handling.  The
    // framework will continue with its own fallback if we return FALSE here.
    targetPath[0] = '\0';
    return FALSE;
}

BOOL WINAPI CWpdFS::CopyOrMoveFromDiskToFS(
    BOOL copy,
    int mode,
    const char*,
    HWND,
    const char*,
    SalEnumSelection2,
    void*,
    int,
    int,
    char* targetPath,
    BOOL* invalidPathOrCancel)
{
    UNREFERENCED_PARAMETER(copy);

    if (invalidPathOrCancel != nullptr)
    {
        *invalidPathOrCancel = FALSE;
    }

    if (mode == 1)
    {
        GetCurrentPath(targetPath);
        return TRUE;
    }

    // Returning FALSE with invalidPathOrCancel == FALSE tells Salamander this
    // FS instance cannot process the transfer and allows the host fallback path.
    return FALSE;
}
