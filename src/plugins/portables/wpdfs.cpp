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

HRESULT WINAPI CWpdFS::GetCurrentContentLocation(_Out_ CWpdDevice*& device, _Out_ CFxString& objectId)
{
    device = nullptr;
    objectId.Empty();

    CFxPath* path = CreatePath(GetCurrentPath().GetString());
    CFxPathComponentToken token;
    CFxItemEnumerator* parentEnumerator = nullptr;
    HRESULT hr = S_OK;
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
                    objectId = CW2A(contentItem->GetObjectId());
                    if (device != nullptr)
                    {
                        device->Release();
                    }
                    device = contentItem->GetDeviceNoAddRef();
                    device->AddRef();
                }
                item->Release();
                break;
            }
            item->Release();
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
        if (SUCCEEDED(hr)) hr = device->GetPropertiesNoAddRef()->SetValues(item->GetObjectId(), values, nullptr);
    }
    device->Close();
    return hr;
}

HRESULT WINAPI CWpdFS::DeleteWpdObject(CWpdBaseContentItem* item)
{
    CWpdDevice* device = item->GetDeviceNoAddRef();
    HRESULT hr = device->Open(GENERIC_READ | GENERIC_WRITE);
    if (FAILED(hr)) return hr;

    ATL::CComPtr<IPortableDevicePropVariantCollection> objects;
    hr = objects.CoCreateInstance(CLSID_PortableDevicePropVariantCollection);
    if (SUCCEEDED(hr))
    {
        PROPVARIANT pv;
        PropVariantInit(&pv);
        pv.vt = VT_LPWSTR;
        pv.pwszVal = const_cast<PWSTR>(item->GetObjectId());
        hr = objects->Add(&pv);
        if (SUCCEEDED(hr))
        {
            ATL::CComPtr<IPortableDevicePropVariantCollection> results;
            hr = device->GetContentNoAddRef()->Delete(PORTABLE_DEVICE_DELETE_WITH_RECURSION, objects, &results);
        }
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

    BOOL focused = (selectedFiles == 0 && selectedDirs == 0);
    int index = 0;
    bool ok = true;
    for (;;)
    {
        BOOL isDir = FALSE;
        const CFileData* f = focused ? SalamanderGeneral->GetPanelFocusedItem(panel, &isDir) : SalamanderGeneral->GetPanelSelectedItem(panel, &index, &isDir);
        if (f == nullptr) break;
        auto item = static_cast<CWpdBaseContentItem*>(reinterpret_cast<CFxItem*>(f->PluginData));
        HRESULT hr = DeleteWpdObject(item);
        if (FAILED(hr))
        {
            WpdShowOperationError(parent, "Delete", f->Name, hr);
            ok = false;
            break;
        }
        if (focused) break;
    }
    cancelOrError = !ok;
    SalamanderGeneral->PostRefreshPanelFS(this);
    return TRUE;
}
