// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later

// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-09 | Remove WPD bulk callback folder enumeration. | The crash dump's worker stack points into the bulk-load path despite null-interface guards. | https://github.com/KRtkovo-eu-AI/salamander/issues/832 | GPT-6 |
// | 2026-11-09 | Guard failed WPD interfaces and malformed bulk results. | A null interface returned by a device driver caused an access violation in the listing worker. | https://github.com/KRtkovo-eu-AI/salamander/issues/832 | GPT-6 |
// | 2026-11-09 | Bind listed items to the panel device when enumerating in a worker session. | Items outlive the worker's WPD connection and need the normal panel device for later actions. | https://github.com/KRtkovo-eu-AI/salamander/issues/832 | GPT-6 |
// | 2026-11-09 | Retrieve folder metadata with WPD bulk queries. | Per-object property requests made large MTP folders much slower than Explorer. | https://github.com/KRtkovo-eu-AI/salamander/issues/832 | GPT-6 |

/*
	Windows Portable Devices Plugin for Open Salamander
	
	Copyright (c) 2015 Milan Kase <manison@manison.cz>
	Copyright (c) 2015 Open Salamander Authors
	
	wpdfscontentlevel.cpp
	Content level enumerator.
*/

#include "precomp.h"
#include "fxfs.h"
#include "wpdfs.h"
#include "wpdfscontentlevel.h"
#include "wpdhelpers.h"
#include "wpddeviceicons.h"

extern CWpdDeviceIcons g_oDeviceIcons;

using namespace ATL;

////////////////////////////////////////////////////////////////////////////////
// CWpdBaseContentItem

CWpdBaseContentItem::CWpdBaseContentItem(
    WPDFS_LEVEL level,
    CWpdDevice* device,
    PCWSTR objectId,
    IPortableDeviceValues* values) : CWpdItem(WPDFS_LEVEL_CONTENT),
                                     m_device(device),
                                     m_objectId(objectId),
                                     m_flags(0U),
                                     m_attributes(0U)
{
    HRESULT hr;
    GUID guid;

    hr = WpdGetStringValue(values, WPD_OBJECT_ORIGINAL_FILE_NAME, m_strName);
    if (FAILED(hr) || m_strName.IsEmpty())
    {
        hr = WpdGetStringValue(values, WPD_OBJECT_NAME, m_strName);
        _ASSERTE(SUCCEEDED(hr));
        if (FAILED(hr))
        {
            TRACE_E("Failed to get WPD object name");
        }
    }

    hr = values->GetGuidValue(WPD_OBJECT_CONTENT_TYPE, &guid);
    _ASSERTE(SUCCEEDED(hr));
    if (SUCCEEDED(hr) && IsFolder(guid))
    {
        m_attributes |= FILE_ATTRIBUTE_DIRECTORY;
    }
}

CWpdBaseContentItem::~CWpdBaseContentItem()
{
    ::CoTaskMemFree(const_cast<PWSTR>(m_objectId));
}

void WINAPI CWpdBaseContentItem::GetName(CFxString& name)
{
    name = m_strName;
}

DWORD WINAPI CWpdBaseContentItem::GetAttributes()
{
    return m_attributes;
}

bool WINAPI CWpdBaseContentItem::IsFolder(const GUID& contentType)
{
    return IsEqualGUID(contentType, WPD_CONTENT_TYPE_FOLDER) ||
           IsEqualGUID(contentType, WPD_CONTENT_TYPE_FUNCTIONAL_OBJECT);
}

////////////////////////////////////////////////////////////////////////////////
// CWpdStorageItem

const PROPERTYKEY* const CWpdStorageItem::s_requiredKeys[] =
    {
        &WPD_OBJECT_ORIGINAL_FILE_NAME,
        &WPD_OBJECT_NAME,
        &WPD_OBJECT_CONTENT_TYPE,
        &WPD_FUNCTIONAL_OBJECT_CATEGORY,
};

CWpdStorageItem::CWpdStorageItem(
    CWpdDevice* device,
    PCWSTR objectId,
    IPortableDeviceValues* values) : CWpdBaseContentItem(WPDFS_LEVEL_STORAGE, device, objectId, values)
{
}

IPortableDeviceKeyCollection* WINAPI CWpdStorageItem::GetRequiredKeys()
{
    return WpdInitKeys(s_requiredKeys, _countof(s_requiredKeys));
}

int WINAPI CWpdStorageItem::GetSimpleIconIndex()
{
    return CWpdDeviceIcons::Storage;
}

////////////////////////////////////////////////////////////////////////////////
// CWpdContentItem

const PROPERTYKEY* const CWpdContentItem::s_requiredKeys[] =
    {
        &WPD_OBJECT_ORIGINAL_FILE_NAME,
        &WPD_OBJECT_NAME,
        &WPD_OBJECT_CONTENT_TYPE,
        &WPD_OBJECT_SIZE,
        &WPD_OBJECT_DATE_MODIFIED,
        &WPD_OBJECT_DATE_CREATED,
        &WPD_OBJECT_ISHIDDEN,
        &WPD_OBJECT_ISSYSTEM,
};

CWpdContentItem::CWpdContentItem(
    CWpdDevice* device,
    PCWSTR objectId,
    IPortableDeviceValues* values,
    FILETIME now) : CWpdBaseContentItem(WPDFS_LEVEL_CONTENT, device, objectId, values),
                    m_size(0ULL),
                    m_lastWrite(FILETIME_Nul)
{
    HRESULT hr;
    ULONGLONG ui8;
    BOOL b;

    hr = values->GetBoolValue(WPD_OBJECT_ISHIDDEN, &b);
    if (SUCCEEDED(hr) && b)
    {
        m_attributes |= FILE_ATTRIBUTE_HIDDEN;
    }

    hr = values->GetBoolValue(WPD_OBJECT_ISSYSTEM, &b);
    if (SUCCEEDED(hr) && b)
    {
        m_attributes |= FILE_ATTRIBUTE_SYSTEM;
    }

    hr = values->GetUnsignedLargeIntegerValue(WPD_OBJECT_SIZE, &ui8);
    if (SUCCEEDED(hr))
    {
        m_flags |= FLAGS_HaveSize;
        m_size = ui8;
    }

    hr = WpdGetDateTimeValue(values, WPD_OBJECT_DATE_MODIFIED, m_lastWrite);
    if (FAILED(hr))
    {
        hr = WpdGetDateTimeValue(values, WPD_OBJECT_DATE_CREATED, m_lastWrite);
        if (FAILED(hr))
        {
            // If we could not obtain object's date, use the
            // current time instead.
            m_lastWrite = now;
        }
    }

    if (SUCCEEDED(hr))
    {
        m_flags |= FLAGS_HaveLastWrite;
    }
}

HRESULT WINAPI CWpdContentItem::GetSize(ULONGLONG& size)
{
    size = m_size;

    // Only consider FLAGS_HaveSize if CWpdContentEnumerator::GetValidData
    // includes VALID_DATA_PL_SIZE. Otherwise this method is not supposed
    // to fail.
    ////return (m_flags & FLAGS_HaveSize) ? S_OK : FX_E_NO_DATA;

    return S_OK;
}

HRESULT WINAPI CWpdContentItem::GetLastWriteTime(FILETIME& time)
{
    time = m_lastWrite;

    // Only consider FLAGS_HaveLastWrite if CWpdContentEnumerator::GetValidData
    // includes VALID_DATA_PL_DATE/TIME. Otherwise this method is not supposed
    // to fail.
    ////return (m_flags & FLAGS_HaveLastWrite) ? S_OK : FX_E_NO_DATA;

    return S_OK;
}

IPortableDeviceKeyCollection* WINAPI CWpdContentItem::GetRequiredKeys()
{
    return WpdInitKeys(s_requiredKeys, _countof(s_requiredKeys));
}

////////////////////////////////////////////////////////////////////////////////
// CWpdBaseContentEnumerator

CWpdBaseContentEnumerator::CWpdBaseContentEnumerator(WPDFS_LEVEL level)
    : CWpdEnumerator(level),
      m_device(nullptr),
      m_itemDevice(nullptr),
      m_objectIdCount(0),
      m_nextObjectId(0),
      m_enumFinished(false),
      m_current(nullptr)
{
    memset(m_objectIds, 0, sizeof(m_objectIds));
}

void WINAPI CWpdBaseContentEnumerator::ReleasePendingObjectIds()
{
    for (ULONG i = 0; i < m_objectIdCount; ++i)
    {
        if (i >= m_nextObjectId)
        {
            ::CoTaskMemFree(m_objectIds[i]);
            m_objectIds[i] = nullptr;
        }
    }
    m_objectIdCount = 0;
    m_nextObjectId = 0;
}

void CWpdBaseContentEnumerator::FinalRelease()
{
    ReleaseCurrent();
    ReleasePendingObjectIds();
    if (m_device != nullptr)
    {
        m_device->Close();
        m_device->Release();
    }

    m_keys.Release();
    m_enum.Release();
}

HRESULT WINAPI CWpdBaseContentEnumerator::Initialize(CWpdDevice* device, PCWSTR parentObjectId)
{
    _ASSERTE(m_device == nullptr);
    _ASSERTE(device != nullptr);
    _ASSERTE(parentObjectId != nullptr && parentObjectId[0] != L'\0');

    if (device == nullptr || parentObjectId == nullptr || parentObjectId[0] == L'\0')
    {
        return E_INVALIDARG;
    }

    HRESULT hr;

    try
    {
        m_parentObjectId = parentObjectId;
    }
    catch (...)
    {
        return E_OUTOFMEMORY;
    }

    hr = device->Open(GENERIC_READ);
    if (SUCCEEDED(hr))
    {
        auto* content = device->GetContentNoAddRef();
        hr = content != nullptr ? content->EnumObjects(0U, parentObjectId, nullptr, &m_enum) : E_POINTER;
        if (SUCCEEDED(hr) && m_enum == nullptr)
        {
            hr = E_UNEXPECTED;
        }
        if (SUCCEEDED(hr))
        {
            m_keys.Attach(GetRequiredKeys());
            if (m_keys == nullptr)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                m_device = device;
                device->AddRef();
            }
        }

        if (FAILED(hr))
        {
            device->Close();
        }
    }

    return hr;
}

HRESULT WINAPI CWpdBaseContentEnumerator::MoveNext()
{
    if (m_device == nullptr || m_keys == nullptr)
    {
        return E_POINTER;
    }

    HRESULT hr;

    ReleaseCurrent();
    while (true)
    {
        if (m_nextObjectId == m_objectIdCount)
        {
            ReleasePendingObjectIds();
            if (m_enumFinished)
            {
                return S_FALSE;
            }

            ULONG fetched = 0;
            if (m_enum == nullptr)
            {
                return E_POINTER;
            }
            hr = m_enum->Next(OBJECT_ID_BATCH_SIZE, m_objectIds, &fetched);
            _ASSERTE(fetched <= OBJECT_ID_BATCH_SIZE);
            if (FAILED(hr))
            {
                m_objectIdCount = fetched;
                ReleasePendingObjectIds();
                return hr;
            }
            if (fetched == 0)
            {
                m_enumFinished = true;
                return hr;
            }

            m_objectIdCount = fetched;
            m_enumFinished = hr == S_FALSE;

            for (ULONG i = 0; i < m_objectIdCount; ++i)
            {
                if (m_objectIds[i] == nullptr)
                {
                    ReleasePendingObjectIds();
                    return E_UNEXPECTED;
                }
            }

        }

        const ULONG objectIndex = m_nextObjectId++;
        PWSTR objectId = m_objectIds[objectIndex];
        m_objectIds[objectIndex] = nullptr;
        CComPtr<IPortableDeviceValues> values;
        IPortableDeviceProperties* properties = m_device != nullptr ? m_device->GetPropertiesNoAddRef() : nullptr;
        if (properties == nullptr)
        {
            ::CoTaskMemFree(objectId);
            return E_POINTER;
        }
        hr = properties->GetValues(objectId, m_keys, &values);
        if (FAILED(hr))
        {
            ::CoTaskMemFree(objectId);
            return hr;
        }

        if (values == nullptr)
        {
            ::CoTaskMemFree(objectId);
            return E_UNEXPECTED;
        }

        if (!ShouldSkip(values))
        {
            // GetValues may return S_FALSE when optional properties are unavailable.
            m_current = CreateItem(
                m_itemDevice != nullptr ? m_itemDevice : m_device, objectId, values);
            return S_OK;
        }

        ::CoTaskMemFree(objectId);
    }
}

CFxItem* WINAPI CWpdBaseContentEnumerator::GetCurrent() const
{
    _ASSERTE(m_current != nullptr);
    m_current->AddRef();
    return m_current;
}

void WINAPI CWpdBaseContentEnumerator::Reset()
{
    ReleaseCurrent();
    ReleasePendingObjectIds();
    m_enumFinished = false;
    if (m_enum != nullptr)
    {
        m_enum->Reset();
    }
}

////////////////////////////////////////////////////////////////////////////////
// CWpdStorageEnumerator

CWpdStorageEnumerator::CWpdStorageEnumerator()
    : CWpdBaseContentEnumerator(WPDFS_LEVEL_STORAGE)
{
}

CWpdPluginDataInterface* WINAPI CWpdStorageEnumerator::CreatePluginData(CFxPluginFSInterface& owner)
{
    return new CWpdStoragePluginDataInterface(owner);
}

IPortableDeviceKeyCollection* WINAPI CWpdStorageEnumerator::GetRequiredKeys()
{
    return CWpdStorageItem::GetRequiredKeys();
}

bool WINAPI CWpdStorageEnumerator::ShouldSkip(IPortableDeviceValues* values)
{
    HRESULT hr;
    GUID guid;

    hr = values->GetGuidValue(WPD_FUNCTIONAL_OBJECT_CATEGORY, &guid);
    bool isStorage = SUCCEEDED(hr) && IsEqualGUID(guid, WPD_FUNCTIONAL_CATEGORY_STORAGE);

    return !isStorage;
}

CWpdBaseContentItem* WINAPI CWpdStorageEnumerator::CreateItem(CWpdDevice* device, PCWSTR objectId, IPortableDeviceValues* values)
{
    return new CWpdStorageItem(device, objectId, values);
}

DWORD WINAPI CWpdStorageEnumerator::GetValidData() const
{
    // We do not have any extra data but the name for the storage items.
    return VALID_DATA_NONE;
}

////////////////////////////////////////////////////////////////////////////////
// CWpdContentEnumerator

CWpdContentEnumerator::CWpdContentEnumerator()
    : CWpdBaseContentEnumerator(WPDFS_LEVEL_CONTENT)
{
    ::GetSystemTimeAsFileTime(&m_now);
}

CWpdPluginDataInterface* WINAPI CWpdContentEnumerator::CreatePluginData(CFxPluginFSInterface& owner)
{
    return new CWpdContentPluginDataInterface(owner);
}

IPortableDeviceKeyCollection* WINAPI CWpdContentEnumerator::GetRequiredKeys()
{
    return CWpdContentItem::GetRequiredKeys();
}

bool WINAPI CWpdContentEnumerator::ShouldSkip(IPortableDeviceValues* values)
{
    return false;
}

CWpdBaseContentItem* WINAPI CWpdContentEnumerator::CreateItem(CWpdDevice* device, PCWSTR objectId, IPortableDeviceValues* values)
{
    return new CWpdContentItem(device, objectId, values, m_now);
}

DWORD WINAPI CWpdContentEnumerator::GetValidData() const
{
    DWORD mask = __super::GetValidData();
    ////mask |= VALID_DATA_PL_SIZE;
    mask |= VALID_DATA_SIZE | VALID_DATA_DATE | VALID_DATA_TIME;
    return mask;
}

////////////////////////////////////////////////////////////////////////////////
// CWpdBaseContentPluginDataInterface

CWpdBaseContentPluginDataInterface::CWpdBaseContentPluginDataInterface(
    CFxPluginFSInterface& owner,
    WPDFS_LEVEL level) : CWpdPluginDataInterface(owner, level)
{
}

////////////////////////////////////////////////////////////////////////////////
// CWpdStoragePluginDataInterface

CWpdStoragePluginDataInterface::CWpdStoragePluginDataInterface(CFxPluginFSInterface& owner)
    : CWpdBaseContentPluginDataInterface(owner, WPDFS_LEVEL_STORAGE)
{
}

HIMAGELIST WINAPI CWpdStoragePluginDataInterface::GetSimplePluginIcons(int iconSize)
{
    return g_oDeviceIcons.GetSimpleDeviceIcons(iconSize);
}

////////////////////////////////////////////////////////////////////////////////
// CWpdContentPluginDataInterface

CWpdContentPluginDataInterface::CWpdContentPluginDataInterface(CFxPluginFSInterface& owner)
    : CWpdBaseContentPluginDataInterface(owner, WPDFS_LEVEL_CONTENT)
{
}

int WINAPI CWpdContentPluginDataInterface::GetIconsType() const
{
    return pitFromRegistry;
}
