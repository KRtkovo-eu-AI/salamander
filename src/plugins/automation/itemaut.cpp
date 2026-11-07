// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
	Automation Plugin for Open Salamander
	
	Copyright (c) 2009-2026 Milan Kase <manison@manison.cz>
	Copyright (c) 2010-2026 Open Salamander Authors
	
	itemaut.cpp
	Panel item automation object.
*/

#include "precomp.h"
#include "salamander_h.h"
#include "itemaut.h"
#include "aututils.h"

extern CSalamanderGeneralAbstract* SalamanderGeneral;

CSalamanderPanelItemAutomation::CSalamanderPanelItemAutomation()
{
    _ctor();
}

CSalamanderPanelItemAutomation::CSalamanderPanelItemAutomation(const CFileData* pData, PCTSTR pszPath)
{
    _ctor();
    Set(pData, pszPath);
}

CSalamanderPanelItemAutomation::CSalamanderPanelItemAutomation(const CFileData* pData, int nPanel)
{
    _ctor();
    Set(pData, nPanel);
}

CSalamanderPanelItemAutomation::~CSalamanderPanelItemAutomation()
{

}

void CSalamanderPanelItemAutomation::_ctor()
{
    m_fullPath.clear();
    m_name.clear();
    m_size.QuadPart = 0;
    m_dwAttributes = INVALID_FILE_ATTRIBUTES;
    m_dateLastModified = 0;
}

static std::wstring AutomationItemPathToWide(const char* path)
{
    if (path == NULL || *path == 0)
        return std::wstring();
    UINT codePage = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    int length = MultiByteToWideChar(codePage, flags, path, -1, NULL, 0);
    if (length == 0)
    {
        codePage = CP_ACP;
        flags = 0;
        length = MultiByteToWideChar(codePage, flags, path, -1, NULL, 0);
    }
    if (length <= 0)
        return std::wstring();
    std::wstring result(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(codePage, flags, path, -1, &result[0], length);
    result.resize(static_cast<size_t>(length - 1));
    return result;
}

CSalamanderPanelItemAutomation::CSalamanderPanelItemAutomation(const CSalamanderDiskSelectionItem& item)
{
    _ctor();
    Set(item);
}

void CSalamanderPanelItemAutomation::Set(const CSalamanderDiskSelectionItem& item)
{
    // Copy before publishing: COM item objects outlive the panel and snapshot.
    std::wstring fullPath(item.FullPathW != NULL ? item.FullPathW : L"");
    std::wstring name(item.NameW != NULL ? item.NameW : L"");
    m_fullPath.swap(fullPath);
    m_name.swap(name);
    m_dwAttributes = item.Attr;
    m_size.QuadPart = item.Size.Value;
    m_dateLastModified = 0;
    FILETIME local;
    SYSTEMTIME system;
    if (FileTimeToLocalFileTime(&item.LastWrite, &local) && FileTimeToSystemTime(&local, &system))
        SystemTimeToVariantTime(&system, &m_dateLastModified);
}

void CSalamanderPanelItemAutomation::Set(const CFileData* pData, PCTSTR pszPath)
{
    // Archive/FS and the virtual '..' UI item retain their original semantics.
    // Disk files enter through the immutable full-path snapshot overload above.
    std::wstring path = AutomationItemPathToWide(pszPath);
    std::wstring name = pData->UseWideName() ? pData->NameW : AutomationItemPathToWide(pData->Name);
    if (!path.empty() && path.back() != L'\\')
        path += L'\\';
    path += name;
    CSalamanderDiskSelectionItem item = {};
    item.FullPathW = path.c_str();
    item.NameW = name.c_str();
    item.Attr = pData->Attr;
    item.Size = pData->Size;
    item.LastWrite = pData->LastWrite;
    Set(item);
}

void CSalamanderPanelItemAutomation::Set(const CFileData* pData, int nPanel)
{
    std::vector<char> path(SAL_MAX_PATH, '\0');
    if (SalamanderGeneral->GetPanelPath(nPanel, path.data(), static_cast<int>(path.size()), NULL, NULL))
        Set(pData, path.data());
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderPanelItemAutomation::get_Path(
    /* [retval][out] */ BSTR* path)
{
    if (path == NULL) return E_POINTER;
    *path = SysAllocString(m_fullPath.c_str());
    return *path != NULL ? S_OK : E_OUTOFMEMORY;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderPanelItemAutomation::get_Name(
    /* [retval][out] */ BSTR* name)
{
    if (name == NULL) return E_POINTER;
    *name = SysAllocString(m_name.c_str());
    return *name != NULL ? S_OK : E_OUTOFMEMORY;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderPanelItemAutomation::get_Size(
    /* [retval][out] */ VARIANT* size)
{
    QuadWordToVariant(m_size, size);
    return S_OK;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderPanelItemAutomation::get_DateLastModified(
    /* [retval][out] */ DATE* date)
{
    *date = m_dateLastModified;
    return S_OK;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderPanelItemAutomation::get_Attributes(
    /* [retval][out] */ int* attrs)
{
    *attrs = m_dwAttributes;
    return S_OK;
}
