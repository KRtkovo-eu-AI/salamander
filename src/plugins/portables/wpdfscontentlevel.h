// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later

// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-09 | Use standard per-object WPD metadata reads in folder listings. | The latest dump points to the bulk callback path as the remaining crash source. | https://github.com/KRtkovo-eu-AI/salamander/issues/832 | GPT-6 |
// | 2026-11-09 | Store the panel device on the content enumerator. | Worker-created items are selected by the enumerator and must use its separate item-device pointer. | https://github.com/KRtkovo-eu-AI/salamander/issues/832 | GPT-6 |
// | 2026-11-09 | Let worker enumerators create items for the panel's device object. | Worker COM sessions are separate, while cached items must retain the normal device identity. | https://github.com/KRtkovo-eu-AI/salamander/issues/832 | GPT-6 |
// | 2026-11-09 | Enable bulk metadata listing for Portable Devices. | Large MTP folders need one bulk WPD query instead of a property request per file. | https://github.com/KRtkovo-eu-AI/salamander/issues/832 | GPT-6 |

/*
	Windows Portable Devices Plugin for Open Salamander
	
	Copyright (c) 2015 Milan Kase <manison@manison.cz>
	Copyright (c) 2015 Open Salamander Authors
	
	wpdfscontentlevel.h
	Content level enumerator.
*/

#pragma once

#include "device.h"

class CWpdBaseContentItem : public CWpdItem
{
private:
    PCWSTR m_objectId;

    CWpdDevice* m_device;

    CFxString m_strName;

protected:
    DWORD m_attributes;

    UINT m_flags;

    static bool WINAPI IsFolder(const GUID& contentType);

    CWpdBaseContentItem(WPDFS_LEVEL level, CWpdDevice* device, PCWSTR objectId, IPortableDeviceValues* values);

public:
    virtual ~CWpdBaseContentItem();

    CWpdDevice* WINAPI GetDeviceNoAddRef()
    {
        return m_device;
    }

    PCWSTR GetObjectId() const
    {
        return m_objectId;
    }

    /* CFxItem Overrides */

    virtual void WINAPI GetName(CFxString& name) override;

    virtual DWORD WINAPI GetAttributes() override;
};

class CWpdStorageItem final : public CWpdBaseContentItem
{
private:
    static const PROPERTYKEY* const s_requiredKeys[];

public:
    CWpdStorageItem(CWpdDevice* device, PCWSTR objectId, IPortableDeviceValues* values);

    static IPortableDeviceKeyCollection* WINAPI GetRequiredKeys();

    /* CFxItem Overrides */

    virtual int WINAPI GetSimpleIconIndex() override;
};

class CWpdContentItem final : public CWpdBaseContentItem
{
private:
    ULONGLONG m_size;

    FILETIME m_lastWrite;

    enum FLAGS
    {
        FLAGS_HaveSize = 1,
        FLAGS_HaveLastWrite = 2,
    };

    static const PROPERTYKEY* const s_requiredKeys[];

public:
    CWpdContentItem(CWpdDevice* device, PCWSTR objectId, IPortableDeviceValues* values, FILETIME now);

    static IPortableDeviceKeyCollection* WINAPI GetRequiredKeys();

    /* CFxItem Overrides */

    virtual HRESULT WINAPI GetSize(ULONGLONG& size) override;

    virtual HRESULT WINAPI GetLastWriteTime(FILETIME& time) override;
};

class CWpdBaseContentEnumerator : public CWpdEnumerator
{
private:
    static const ULONG OBJECT_ID_BATCH_SIZE = 128;

    CWpdDevice* m_device;

    CWpdDevice* m_itemDevice;

    ATL::CComPtr<IEnumPortableDeviceObjectIDs> m_enum;

    PWSTR m_objectIds[OBJECT_ID_BATCH_SIZE];

    ULONG m_objectIdCount;

    ULONG m_nextObjectId;

    bool m_enumFinished;

    CWpdBaseContentItem* m_current;

    ATL::CComPtr<IPortableDeviceKeyCollection> m_keys;

    std::wstring m_parentObjectId;

    void WINAPI ReleaseCurrent(void)
    {
        if (m_current != nullptr)
        {
            m_current->Release();
            m_current = nullptr;
        }
    }

    void WINAPI ReleasePendingObjectIds(void);

protected:
    CWpdBaseContentEnumerator(WPDFS_LEVEL level);

    virtual bool WINAPI ShouldSkip(IPortableDeviceValues* values) = 0;

    virtual CWpdBaseContentItem* WINAPI CreateItem(
        CWpdDevice* device,
        PCWSTR objectId,
        IPortableDeviceValues* values) = 0;

    virtual IPortableDeviceKeyCollection* WINAPI GetRequiredKeys() = 0;

    virtual void WINAPI FinalRelease() override;

public:
    virtual HRESULT WINAPI Initialize(CWpdDevice* device, PCWSTR parentObjectId);

    void WINAPI SetItemDevice(CWpdDevice* device)
    {
        m_itemDevice = device;
    }

    CWpdDevice* WINAPI GetDeviceNoAddRef() const
    {
        return m_device;
    }

    PCWSTR WINAPI GetParentObjectId() const
    {
        return m_parentObjectId.c_str();
    }

    /* CFxEnumerator Overrides */

    virtual HRESULT WINAPI MoveNext() override;

    virtual void WINAPI Reset() override;

    virtual CFxItem* WINAPI GetCurrent() const override;
};

class CWpdStorageEnumerator final : public CWpdBaseContentEnumerator
{
protected:
    virtual bool WINAPI ShouldSkip(IPortableDeviceValues* values) override;

    virtual CWpdBaseContentItem* WINAPI CreateItem(
        CWpdDevice* device,
        PCWSTR objectId,
        IPortableDeviceValues* values) override;

    virtual IPortableDeviceKeyCollection* WINAPI GetRequiredKeys() override;

public:
    CWpdStorageEnumerator();

    /* CFxEnumerator Overrides */

    virtual DWORD WINAPI GetValidData() const override;

    /* CWpdEnumerator Overrides */

    virtual CWpdPluginDataInterface* WINAPI CreatePluginData(CFxPluginFSInterface& owner) override;
};

class CWpdContentEnumerator final : public CWpdBaseContentEnumerator
{
private:
    FILETIME m_now;

protected:
    virtual bool WINAPI ShouldSkip(IPortableDeviceValues* values) override;

    virtual CWpdBaseContentItem* WINAPI CreateItem(
        CWpdDevice* device,
        PCWSTR objectId,
        IPortableDeviceValues* values) override;

    virtual IPortableDeviceKeyCollection* WINAPI GetRequiredKeys() override;

public:
    CWpdContentEnumerator();

    /* CFxEnumerator Overrides */

    virtual DWORD WINAPI GetValidData() const override;

    /* CWpdEnumerator Overrides */

    virtual CWpdPluginDataInterface* WINAPI CreatePluginData(CFxPluginFSInterface& owner) override;
};

class CWpdBaseContentPluginDataInterface : public CWpdPluginDataInterface
{
protected:
    CWpdBaseContentPluginDataInterface(CFxPluginFSInterface& owner, WPDFS_LEVEL level);
};

class CWpdStoragePluginDataInterface final : public CWpdBaseContentPluginDataInterface
{
public:
    CWpdStoragePluginDataInterface(CFxPluginFSInterface& owner);

    /* CPluginDataInterfaceAbstract Overrides */

    virtual HIMAGELIST WINAPI GetSimplePluginIcons(int iconSize) override;
};

class CWpdContentPluginDataInterface final : public CWpdBaseContentPluginDataInterface
{
public:
    CWpdContentPluginDataInterface(CFxPluginFSInterface& owner);

    /* CFxPluginDataInterface Overrides */

    virtual int WINAPI GetIconsType() const override;

    /* CPluginDataInterfaceAbstract Overrides */
};
