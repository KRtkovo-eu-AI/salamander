// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
	Automation Plugin for Open Salamander
	
	Copyright (c) 2009-2026 Milan Kase <manison@manison.cz>
	Copyright (c) 2010-2026 Open Salamander Authors
	
	itemcoll.cpp
	Panel items automation collection.
*/

#include "precomp.h"
#include "salamander_h.h"
#include "itemaut.h"
#include "itemcoll.h"

extern CSalamanderGeneralAbstract* SalamanderGeneral;

CSalamanderAutomationItemsSnapshot::CSalamanderAutomationItemsSnapshot(int panel, BOOL selectedOnly)
    : Disk(FALSE), Result(S_OK), UpDir(NULL)
{
    int type = 0;
    if (!SalamanderGeneral->GetPanelPath(panel, NULL, 0, &type, NULL))
    {
        Result = E_FAIL;
        return;
    }
    Disk = type == PATH_TYPE_WINDOWS;
    if (!Disk)
        return;
    if (!Selection.Capture(SalamanderGeneral, panel,
                           selectedOnly ? SALDISKSELECTION_SELECTED_ONLY : SALDISKSELECTION_ALL_ITEMS))
    {
        const DWORD error = GetLastError();
        Result = HRESULT_FROM_WIN32(error != ERROR_SUCCESS ? error : ERROR_INVALID_DATA);
        return;
    }
    if (!selectedOnly)
    {
        int index = 0;
        BOOL isDir = FALSE;
        const CFileData* first = SalamanderGeneral->GetPanelItem(panel, &index, &isDir);
        if (first != NULL && isDir && first->Name != NULL && strcmp(first->Name, "..") == 0)
            UpDir = new CSalamanderPanelItemAutomation(first, panel);
    }
}

CSalamanderAutomationItemsSnapshot::~CSalamanderAutomationItemsSnapshot()
{
    if (UpDir != NULL)
        UpDir->Release();
}

int CSalamanderAutomationItemsSnapshot::GetCount() const
{
    return Selection.GetCount() + (UpDir != NULL ? 1 : 0);
}

HRESULT CSalamanderAutomationItemsSnapshot::GetItem(int index, ISalamanderPanelItem** item) const
{
    if (item == NULL)
        return E_POINTER;
    *item = NULL;
    if (FAILED(Result))
        return Result;
    if (index < 0 || index >= GetCount())
        return DISP_E_BADINDEX;
    if (UpDir != NULL && index == 0)
    {
        *item = UpDir;
        (*item)->AddRef();
        return S_OK;
    }
    const CSalamanderDiskSelectionItem* source = Selection.GetItem(index - (UpDir != NULL ? 1 : 0));
    if (source == NULL)
        return E_FAIL;
    try
    {
        *item = new CSalamanderPanelItemAutomation(*source);
        return S_OK;
    }
    catch (const std::bad_alloc&)
    {
        return E_OUTOFMEMORY;
    }
}

CSalamanderPanelItemCollection::CSalamanderPanelItemCollection(
    int nPanel,
    CollectionType type)
{
    m_nPanel = nPanel;
    m_collType = type;
    m_snapshot = std::make_shared<CSalamanderAutomationItemsSnapshot>(nPanel, type == SelectionCollection);
}

CSalamanderPanelItemCollection::~CSalamanderPanelItemCollection()
{
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderPanelItemCollection::get_Item(
    /* [in] */ VARIANT key,
    /* [retval][out] */ ISalamanderPanelItem** item)
{
    HRESULT hr = S_OK;
    int iKey = -1;
    const CFileData* pData;
    int i = 0;

    if (item == NULL) return E_POINTER;
    *item = NULL;
    if (FAILED(m_snapshot->Result)) return m_snapshot->Result;

    try
    {
        iKey = _variant_t(key);
    }
    catch (_com_error& e)
    {
        hr = e.Error();
    }

    if (FAILED(hr))
    {
        return hr;
    }

    if (iKey < 0)
    {
        return DISP_E_BADINDEX;
    }

    if (m_snapshot->Disk)
        return m_snapshot->GetItem(iKey, item);

    do
    {
        switch (m_collType)
        {
        case ItemCollection:
            pData = SalamanderGeneral->GetPanelItem(m_nPanel, &i, NULL);
            break;

        case SelectionCollection:
            pData = SalamanderGeneral->GetPanelSelectedItem(m_nPanel, &i, NULL);
            break;

        default:
            _ASSERTE(0);
            return E_FAIL;
        }

        if (pData == NULL)
        {
            return DISP_E_BADINDEX;
        }
    } while (iKey--);

    _ASSERTE(pData);

    *item = new CSalamanderPanelItemAutomation(pData, m_nPanel);

    return S_OK;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderPanelItemCollection::get_Count(
    /* [retval][out] */ long* count)
{
    if (count == NULL) return E_POINTER;
    *count = 0;
    if (FAILED(m_snapshot->Result)) return m_snapshot->Result;
    *count = GetCount();
    return S_OK;
}

/* [hidden][restricted][propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderPanelItemCollection::get__NewEnum(
    /* [retval][out] */ IUnknown** ppenum)
{
    if (ppenum == NULL) return E_POINTER;
    *ppenum = NULL;
    if (FAILED(m_snapshot->Result)) return m_snapshot->Result;
    try
    {
        *ppenum = (IEnumVARIANT*)new CSalamanderPanelItemEnumerator(m_nPanel, m_collType, m_snapshot);
        return S_OK;
    }
    catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
}

/* static */ int CSalamanderPanelItemCollection::GetCount(int nPanel, CollectionType type)
{
    int count = 0;

    switch (type)
    {
    case ItemCollection:
    {
        // TODO: there must be better way to do this
        int i = 0;
        while (SalamanderGeneral->GetPanelItem(nPanel, &i, NULL))
        {
            ++count;
        }
        break;
    }

    case SelectionCollection:
    {
        int files, dirs;
        if (SalamanderGeneral->GetPanelSelection(nPanel, &files, &dirs))
        {
            count = files + dirs;
        }
        break;
    }

    default:
        _ASSERTE(0);
        break;
    }

    return count;
}

////////////////////////////////////////////////////////////////////////////////

CSalamanderPanelItemEnumerator::CSalamanderPanelItemEnumerator(
    int nPanel,
    CSalamanderPanelItemCollection::CollectionType type,
    std::shared_ptr<CSalamanderAutomationItemsSnapshot> snapshot)
{
    m_snapshot = std::move(snapshot);
    m_nPanel = nPanel;
    m_collType = type;
    m_iItem = 0;
}

CSalamanderPanelItemEnumerator::~CSalamanderPanelItemEnumerator()
{
}

STDMETHODIMP CSalamanderPanelItemEnumerator::QueryInterface(REFIID iid, __out void** ppvObject)
{
    if (IsEqualIID(iid, __uuidof(IEnumVARIANT)))
    {
        AddRef();
        *ppvObject = (IEnumVARIANT*)this;
        return S_OK;
    }

    return __super::QueryInterface(iid, ppvObject);
}

/* [local] */ HRESULT STDMETHODCALLTYPE CSalamanderPanelItemEnumerator::Next(
    /* [in] */ ULONG celt,
    /* [length_is][size_is][out] */ VARIANT* rgVar,
    /* [out] */ ULONG* pCeltFetched)
{
    if (pCeltFetched)
    {
        *pCeltFetched = 0;
    }

    for (; celt; celt--)
    {
        const HRESULT result = FetchItem(rgVar);
        if (result == S_OK)
        {
            if (pCeltFetched)
            {
                ++(*pCeltFetched);
            }
            rgVar++;
        }
        else
        {
            return result;
        }
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CSalamanderPanelItemEnumerator::Skip(
    /* [in] */ ULONG celt)
{
    _ASSERTE(0);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSalamanderPanelItemEnumerator::Reset(void)
{
    m_iItem = 0;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CSalamanderPanelItemEnumerator::Clone(
    /* [out] */ __RPC__deref_out_opt IEnumVARIANT** ppenum)
{
    _ASSERTE(0);
    return E_NOTIMPL;
}

HRESULT CSalamanderPanelItemEnumerator::FetchItem(VARIANT* pItem)
{
    if (pItem == NULL) return E_POINTER;
    VariantInit(pItem);
    try
    {
        ISalamanderPanelItem* object = NULL;
        if (m_snapshot->Disk)
        {
            const HRESULT captured = m_snapshot->GetItem(m_iItem, &object);
            if (captured == DISP_E_BADINDEX) return S_FALSE;
            if (FAILED(captured)) return captured;
            ++m_iItem;
        }
        else
        {
            const CFileData* data = NULL;
            if (m_collType == CSalamanderPanelItemCollection::ItemCollection)
                data = SalamanderGeneral->GetPanelItem(m_nPanel, &m_iItem, NULL);
            else if (m_collType == CSalamanderPanelItemCollection::SelectionCollection)
                data = SalamanderGeneral->GetPanelSelectedItem(m_nPanel, &m_iItem, NULL);
            if (data == NULL) return S_FALSE;
            object = new CSalamanderPanelItemAutomation(data, m_nPanel);
        }
        IDispatch* dispatch = NULL;
        const HRESULT result = object->QueryInterface(__uuidof(IDispatch), reinterpret_cast<void**>(&dispatch));
        object->Release();
        if (FAILED(result)) return result;
        V_VT(pItem) = VT_DISPATCH;
        V_DISPATCH(pItem) = dispatch;
        return S_OK;
    }
    catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
}
