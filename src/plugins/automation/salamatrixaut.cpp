// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Automation Plugin for Open Salamander

    salamatrixaut.cpp
    Script-facing Automation wrappers backed by Salamatrix runtime services.
*/

#include "precomp.h"
#include "salamander_h.h"
#include "salamatrixaut.h"
#include "automationplug.h"
#include "aututils.h"
#include "lang\lang.rh"

extern CAutomationPluginInterface g_oAutomationPlugin;
extern CSalamanderGeneralAbstract* SalamanderGeneral;
extern HINSTANCE g_hLangInst;

static HRESULT RaiseMissingRuntime(LPCOLESTR objectName)
{
    ::RaiseError(L"This script requires Salamatrix Runtime to be installed and loaded.", __uuidof(ISalamander), objectName);
    return E_FAIL;
}

static BSTR ResultToBSTR(Salamatrix::Runtime::OperationResult result)
{
    switch (result)
    {
    case Salamatrix::Runtime::OperationResultOk:
        return SysAllocString(L"ok");
    case Salamatrix::Runtime::OperationResultCancel:
        return SysAllocString(L"cancel");
    default:
        return SysAllocString(L"error");
    }
}

CSalamatrixProgressAutomation::CSalamatrixProgressAutomation(Salamatrix::UI::IUIService* uiService,
                                                             CSalamanderForOperationsAbstract* operation,
                                                             BSTR title)
{
    m_pUIService = uiService;
    m_pOperation = operation;
    m_pProgress = NULL;
    m_style = StyleOneBar;
    m_bShown = false;
    m_bCancelEnabled = true;
    m_nMax.SetUI64(100);
    m_nTotalMax.SetUI64(100);
    m_nPos.SetUI64(0);
    m_nTotalPos.SetUI64(0);

    if (title != NULL && title[0] != 0)
    {
        m_strTitle = title;
    }
    else
    {
        WCHAR szDefTitle[64];
        LoadStringW(g_hLangInst, IDS_PROGRESSTITLE, szDefTitle, _countof(szDefTitle));
        m_strTitle = szDefTitle;
    }
}

CSalamatrixProgressAutomation::~CSalamatrixProgressAutomation()
{
    Hide();
}

HRESULT CSalamatrixProgressAutomation::RequireShown(LPCOLESTR propertyName)
{
    if (m_bShown && m_pProgress != NULL)
    {
        return S_OK;
    }

    RaiseErrorFmt(IDS_E_READONLYWHILEPROGRESSHIDDEN, propertyName);
    return SALAUT_E_READONLYWHILEPROGRESSSHOWN;
}

CQuadWord CSalamatrixProgressAutomation::VariantToQuadWord(VARIANT* value, HRESULT* result)
{
    CQuadWord converted;
    converted.SetUI64(0);
    *result = S_OK;

    if (value == NULL)
    {
        *result = E_POINTER;
        return converted;
    }

    VARIANT coerced;
    VariantInit(&coerced);
    *result = VariantChangeType(&coerced, value, 0, VT_R8);
    if (FAILED(*result))
        return converted;

    converted.SetUI64(static_cast<unsigned __int64>(V_R8(&coerced)));
    VariantClear(&coerced);
    return converted;
}

void CSalamatrixProgressAutomation::ApplyTotals()
{
    if (m_bShown && m_pProgress != NULL)
    {
        if (m_style == StyleTwoBar)
            m_pProgress->SetTotals(m_nMax, m_nTotalMax);
        else
            m_pProgress->SetTotal(m_nMax);
    }
}

void CSalamatrixProgressAutomation::ApplyPositions()
{
    if (m_bShown && m_pProgress != NULL)
    {
        if (m_style == StyleTwoBar)
            m_pProgress->SetPositions(m_nPos, m_nTotalPos, FALSE);
        else
            m_pProgress->SetPosition(m_nPos, FALSE);
    }
}

/* [id] */ HRESULT STDMETHODCALLTYPE CSalamatrixProgressAutomation::Show(void)
{
    if (m_bShown)
        return S_OK;

    if (m_pUIService == NULL || m_pOperation == NULL)
        return RaiseMissingRuntime(GetProgId());

    m_pProgress = m_pUIService->CreateProgressDialog(m_pOperation);
    if (m_pProgress == NULL)
        return RaiseMissingRuntime(GetProgId());

    Salamatrix::UI::ProgressDialogOptions options;
    _bstr_t titleA(m_strTitle);
    options.Title = static_cast<const char*>(titleA);
    options.TwoProgressBars = m_style == StyleTwoBar;
    options.CancelEnabled = m_bCancelEnabled;
    m_pProgress->Open(options);
    m_bShown = true;
    ApplyTotals();
    ApplyPositions();
    return S_OK;
}

/* [id] */ HRESULT STDMETHODCALLTYPE CSalamatrixProgressAutomation::Hide(void)
{
    if (m_pProgress != NULL)
    {
        m_pProgress->Close();
        if (m_pUIService != NULL)
            m_pUIService->DestroyProgressDialog(m_pProgress);
        m_pProgress = NULL;
    }
    m_bShown = false;
    return S_OK;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamatrixProgressAutomation::get_Title(BSTR* title)
{
    if (title == NULL)
        return E_POINTER;
    *title = m_strTitle.copy();
    return S_OK;
}

/* [propput][id] */ HRESULT STDMETHODCALLTYPE CSalamatrixProgressAutomation::put_Title(BSTR title)
{
    if (m_bShown)
    {
        RaiseErrorFmt(IDS_E_READONLYWHILEPROGRESSSHOWN, L"Title");
        return SALAUT_E_READONLYWHILEPROGRESSSHOWN;
    }
    m_strTitle = title;
    return S_OK;
}

/* [id] */ HRESULT STDMETHODCALLTYPE CSalamatrixProgressAutomation::AddText(BSTR text)
{
    HRESULT hr = RequireShown(L"AddText");
    if (FAILED(hr))
        return hr;

    _bstr_t textA(text);
    m_pProgress->AddText(static_cast<const char*>(textA), FALSE);
    return S_OK;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamatrixProgressAutomation::get_IsCancelled(VARIANT_BOOL* cancelled)
{
    if (cancelled == NULL)
        return E_POINTER;
    *cancelled = m_pProgress != NULL && m_pProgress->IsCancelled() ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamatrixProgressAutomation::get_Position(VARIANT* progress)
{
    if (progress == NULL)
        return E_POINTER;
    QuadWordToVariant(m_nPos, progress);
    return S_OK;
}

/* [propput][id] */ HRESULT STDMETHODCALLTYPE CSalamatrixProgressAutomation::put_Position(VARIANT* progress)
{
    HRESULT hr;
    m_nPos = VariantToQuadWord(progress, &hr);
    if (FAILED(hr))
        return hr;
    ApplyPositions();
    return S_OK;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamatrixProgressAutomation::get_TotalPosition(VARIANT* progress)
{
    if (progress == NULL)
        return E_POINTER;
    QuadWordToVariant(m_nTotalPos, progress);
    return S_OK;
}

/* [propput][id] */ HRESULT STDMETHODCALLTYPE CSalamatrixProgressAutomation::put_TotalPosition(VARIANT* progress)
{
    HRESULT hr;
    m_nTotalPos = VariantToQuadWord(progress, &hr);
    if (FAILED(hr))
        return hr;
    ApplyPositions();
    return S_OK;
}

/* [id] */ HRESULT STDMETHODCALLTYPE CSalamatrixProgressAutomation::Step(int step)
{
    HRESULT hr = RequireShown(L"Step");
    if (FAILED(hr))
        return hr;
    m_pProgress->Step(step, FALSE);
    return S_OK;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamatrixProgressAutomation::get_CanCancel(VARIANT_BOOL* enabled)
{
    if (enabled == NULL)
        return E_POINTER;
    *enabled = m_bCancelEnabled ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

/* [propput][id] */ HRESULT STDMETHODCALLTYPE CSalamatrixProgressAutomation::put_CanCancel(VARIANT_BOOL enabled)
{
    m_bCancelEnabled = enabled == VARIANT_TRUE;
    if (m_pProgress != NULL)
        m_pProgress->SetCancelEnabled(m_bCancelEnabled);
    return S_OK;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamatrixProgressAutomation::get_Style(int* barcount)
{
    if (barcount == NULL)
        return E_POINTER;
    *barcount = m_style;
    return S_OK;
}

/* [propput][id] */ HRESULT STDMETHODCALLTYPE CSalamatrixProgressAutomation::put_Style(int barcount)
{
    if (barcount != StyleOneBar && barcount != StyleTwoBar)
        return E_INVALIDARG;
    if (m_bShown)
    {
        RaiseErrorFmt(IDS_E_READONLYWHILEPROGRESSSHOWN, L"Style");
        return SALAUT_E_READONLYWHILEPROGRESSSHOWN;
    }
    m_style = static_cast<ProgressStyle>(barcount);
    return S_OK;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamatrixProgressAutomation::get_Maximum(VARIANT* max)
{
    if (max == NULL)
        return E_POINTER;
    QuadWordToVariant(m_nMax, max);
    return S_OK;
}

/* [propput][id] */ HRESULT STDMETHODCALLTYPE CSalamatrixProgressAutomation::put_Maximum(VARIANT* max)
{
    HRESULT hr;
    m_nMax = VariantToQuadWord(max, &hr);
    if (FAILED(hr))
        return hr;
    ApplyTotals();
    return S_OK;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamatrixProgressAutomation::get_TotalMaximum(VARIANT* max)
{
    if (max == NULL)
        return E_POINTER;
    QuadWordToVariant(m_nTotalMax, max);
    return S_OK;
}

/* [propput][id] */ HRESULT STDMETHODCALLTYPE CSalamatrixProgressAutomation::put_TotalMaximum(VARIANT* max)
{
    HRESULT hr;
    m_nTotalMax = VariantToQuadWord(max, &hr);
    if (FAILED(hr))
        return hr;
    ApplyTotals();
    return S_OK;
}

CSalamanderUINamespaceAutomation::CSalamanderUINamespaceAutomation(CSalamanderForOperationsAbstract* operation)
{
    m_pOperation = operation;
}

/* [id] */ HRESULT STDMETHODCALLTYPE CSalamanderUINamespaceAutomation::progress(VARIANT* title,
                                                                                 ISalamanderProgressDialog** dialog)
{
    if (dialog == NULL)
        return E_POINTER;
    *dialog = NULL;

    g_oAutomationPlugin.RefreshSalamatrixServices();
    Salamatrix::UI::IUIService* uiService = g_oAutomationPlugin.GetSalamatrixBridge()->GetUIService();
    if (uiService == NULL)
        return RaiseMissingRuntime(GetProgId());

    BSTR titleBstr = NULL;
    VARIANT titleVariant;
    VariantInit(&titleVariant);
    if (title != NULL && V_VT(title) != VT_ERROR && V_VT(title) != VT_EMPTY && V_VT(title) != VT_NULL)
    {
        HRESULT hr = VariantChangeType(&titleVariant, title, 0, VT_BSTR);
        if (FAILED(hr))
            return hr;
        titleBstr = V_BSTR(&titleVariant);
    }

    CSalamatrixProgressAutomation* progress = new CSalamatrixProgressAutomation(uiService, m_pOperation, titleBstr);
    VariantClear(&titleVariant);
    *dialog = progress;
    return S_OK;
}

/* [id] */ HRESULT STDMETHODCALLTYPE CSalamanderCommandsAutomation::execute(BSTR commandId, BSTR* result)
{
    if (result == NULL)
        return E_POINTER;
    *result = NULL;

    g_oAutomationPlugin.RefreshSalamatrixServices();
    Salamatrix::Commands::ICommandService* commands = g_oAutomationPlugin.GetSalamatrixBridge()->GetCommandService();
    if (commands == NULL)
        return RaiseMissingRuntime(GetProgId());

    Salamatrix::Commands::ExecuteOptions options;
    _bstr_t commandIdA(commandId);
    *result = ResultToBSTR(commands->Execute(static_cast<const char*>(commandIdA), options));
    return S_OK;
}

/* [id] */ HRESULT STDMETHODCALLTYPE CSalamanderFileOperationsAutomation::rename_interactive(BSTR* result)
{
    if (result == NULL)
        return E_POINTER;
    *result = NULL;

    g_oAutomationPlugin.RefreshSalamatrixServices();
    Salamatrix::FileOperations::IFileOperationsService* fileOperations = g_oAutomationPlugin.GetSalamatrixBridge()->GetFileOperationsService();
    if (fileOperations == NULL)
        return RaiseMissingRuntime(GetProgId());

    Salamatrix::FileOperations::InteractiveOptions options;
    *result = ResultToBSTR(fileOperations->RenameInteractive(options));
    return S_OK;
}

/* [id] */ HRESULT STDMETHODCALLTYPE CSalamanderFileOperationsAutomation::copy_interactive(BSTR* result)
{
    if (result == NULL)
        return E_POINTER;
    *result = NULL;

    g_oAutomationPlugin.RefreshSalamatrixServices();
    Salamatrix::FileOperations::IFileOperationsService* fileOperations = g_oAutomationPlugin.GetSalamatrixBridge()->GetFileOperationsService();
    if (fileOperations == NULL)
        return RaiseMissingRuntime(GetProgId());

    Salamatrix::FileOperations::InteractiveOptions options;
    *result = ResultToBSTR(fileOperations->CopyInteractive(options));
    return S_OK;
}

/* [id] */ HRESULT STDMETHODCALLTYPE CSalamanderFileOperationsAutomation::move_interactive(BSTR* result)
{
    if (result == NULL)
        return E_POINTER;
    *result = NULL;

    g_oAutomationPlugin.RefreshSalamatrixServices();
    Salamatrix::FileOperations::IFileOperationsService* fileOperations = g_oAutomationPlugin.GetSalamatrixBridge()->GetFileOperationsService();
    if (fileOperations == NULL)
        return RaiseMissingRuntime(GetProgId());

    Salamatrix::FileOperations::InteractiveOptions options;
    *result = ResultToBSTR(fileOperations->MoveInteractive(options));
    return S_OK;
}
