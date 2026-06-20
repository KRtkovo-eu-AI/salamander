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
        if (SUCCEEDED(hr) && results != nullptr)
        {
            HRESULT propertyHr;
            if (SUCCEEDED(results->GetErrorValue(WPD_OBJECT_ORIGINAL_FILE_NAME, &propertyHr)) && FAILED(propertyHr))
            {
                hr = propertyHr;
            }
            else if (SUCCEEDED(results->GetErrorValue(WPD_OBJECT_NAME, &propertyHr)) && FAILED(propertyHr))
            {
                hr = propertyHr;
            }
        }
    }
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
                ULARGE_INTEGER size;
                size.QuadPart = static_cast<ULONGLONG>(-1);
                hr = source->CopyTo(target, size, nullptr, nullptr);
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

    ULARGE_INTEGER size;
    size.QuadPart = static_cast<ULONGLONG>(-1);
    hr = source->CopyTo(target, size, nullptr, nullptr);
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
        ULARGE_INTEGER size = stat.cbSize;
        hr = source->CopyTo(target, size, nullptr, nullptr);
        if (SUCCEEDED(hr))
        {
            hr = target->Commit(STGC_DEFAULT);
        }
    }
    ::CoTaskMemFree(newObjectId);
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
        lstrcpyn(targetUserPart, targetPath + fsNameLen + 1, _countof(targetUserPart));
        char* lastSlash = strrchr(targetUserPart, '\\');
        char* lastComponent = lastSlash != nullptr ? lastSlash + 1 : targetUserPart;
        if (strchr(lastComponent, '*') != nullptr || strchr(lastComponent, '?') != nullptr)
        {
            if (lastSlash != nullptr)
            {
                *lastSlash = '\0';
            }
            else
            {
                lstrcpyn(targetUserPart, "\\", _countof(targetUserPart));
            }
        }

        HRESULT hr = GetContentLocationForPath(targetUserPart, targetDevice, targetObjectId);
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

    if (mode == 3)
    {
        bool ok = true;
        BOOL focused = (selectedFiles == 0 && selectedDirs == 0);
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

            if (focused) break;
        }
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
    int,
    int,
    char* targetPath,
    BOOL* invalidPathOrCancel)
{
    if (invalidPathOrCancel != nullptr)
    {
        *invalidPathOrCancel = FALSE;
    }

    if (mode == 1)
    {
        GetCurrentPath(targetPath);
        return TRUE;
    }

    if (mode != 2 && mode != 3)
    {
        return FALSE;
    }

    int fsNameLen = lstrlen(fsName);
    if (!(SalamanderGeneral->StrNICmp(targetPath, fsName, fsNameLen) == 0 && targetPath[fsNameLen] == ':'))
    {
        return FALSE;
    }

    char targetUserPart[2 * MAX_PATH];
    lstrcpyn(targetUserPart, targetPath + fsNameLen + 1, _countof(targetUserPart));
    char* lastSlash = strrchr(targetUserPart, '\\');
    char* lastComponent = lastSlash != nullptr ? lastSlash + 1 : targetUserPart;
    if (strchr(lastComponent, '*') != nullptr || strchr(lastComponent, '?') != nullptr)
    {
        if (lastSlash != nullptr)
        {
            *lastSlash = '\0';
        }
        else
        {
            lstrcpyn(targetUserPart, "\\", _countof(targetUserPart));
        }
    }

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
    while ((name = next(parent, 0, &dosName, &isDir, &size, &attr, &lastWrite, nextParam, &errorOccured)) != nullptr)
    {
        char sourceName[MAX_PATH];
        lstrcpyn(sourceName, sourcePath, _countof(sourceName));
        if (!SalamanderGeneral->SalPathAppend(sourceName, name, _countof(sourceName)))
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
        }
        else if (isDir)
        {
            hr = CreateWpdFolder(targetDevice, wideTargetObjectId, name);
        }
        else
        {
            const char* targetName = strrchr(name, '\\');
            targetName = targetName != nullptr ? targetName + 1 : name;
            hr = UploadDiskFile(targetDevice, wideTargetObjectId, sourceName, targetName);
            if (SUCCEEDED(hr) && !copy)
            {
                if (!::DeleteFile(sourceName))
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
    }

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
