// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Automation Plugin for Open Salamander

    salamatrixaut.h
    Script-facing Automation wrappers backed by Salamatrix runtime services.
*/

#pragma once

#include "dispimpl.h"
#include "../salamatrix/salamatrix_commands.h"
#include "../salamatrix/salamatrix_ui.h"

class CSalamatrixProgressAutomation : public CDispatchImpl<CSalamatrixProgressAutomation, ISalamanderProgressDialog>
{
public:
    enum ProgressStyle
    {
        StyleOneBar = 1,
        StyleTwoBar = 2
    };

private:
    Salamatrix::UI::IUIService* m_pUIService;
    CSalamanderForOperationsAbstract* m_pOperation;
    Salamatrix::UI::IProgressDialog* m_pProgress;
    ProgressStyle m_style;
    bool m_bShown;
    bool m_bCancelEnabled;
    _bstr_t m_strTitle;
    CQuadWord m_nMax;
    CQuadWord m_nTotalMax;
    CQuadWord m_nPos;
    CQuadWord m_nTotalPos;

    HRESULT RequireShown(LPCOLESTR propertyName);
    CQuadWord VariantToQuadWord(VARIANT* value, HRESULT* result);
    void ApplyTotals();
    void ApplyPositions();

public:
    DECLARE_DISPOBJ_NAME(L"Salamander.UI.ProgressDialog")

    CSalamatrixProgressAutomation(Salamatrix::UI::IUIService* uiService,
                                  CSalamanderForOperationsAbstract* operation,
                                  BSTR title);
    ~CSalamatrixProgressAutomation();

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE Show(void);
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE Hide(void);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Title(/* [retval][out] */ BSTR* title);
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Title(/* [in] */ BSTR title);
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE AddText(/* [in] */ BSTR text);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_IsCancelled(/* [retval][out] */ VARIANT_BOOL* cancelled);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Position(/* [retval][out] */ VARIANT* progress);
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Position(/* [in] */ VARIANT* progress);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_TotalPosition(/* [retval][out] */ VARIANT* progress);
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_TotalPosition(/* [in] */ VARIANT* progress);
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE Step(/* [in] */ int step);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_CanCancel(/* [retval][out] */ VARIANT_BOOL* enabled);
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_CanCancel(/* [in] */ VARIANT_BOOL enabled);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Style(/* [retval][out] */ int* barcount);
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Style(/* [in] */ int barcount);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Maximum(/* [retval][out] */ VARIANT* max);
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Maximum(/* [in] */ VARIANT* max);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_TotalMaximum(/* [retval][out] */ VARIANT* max);
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_TotalMaximum(/* [in] */ VARIANT* max);
};

class CSalamanderUINamespaceAutomation : public CDispatchImpl<CSalamanderUINamespaceAutomation, ISalamanderUI>
{
private:
    CSalamanderForOperationsAbstract* m_pOperation;

public:
    DECLARE_DISPOBJ_NAME(L"Salamander.UI")

    explicit CSalamanderUINamespaceAutomation(CSalamanderForOperationsAbstract* operation);

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE progress(/* [optional][in] */ VARIANT* title,
                                                          /* [retval][out] */ ISalamanderProgressDialog** dialog);
};

class CSalamanderCommandsAutomation : public CDispatchImpl<CSalamanderCommandsAutomation, ISalamanderCommands>
{
public:
    DECLARE_DISPOBJ_NAME(L"Salamander.Commands")

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE execute(/* [in] */ BSTR commandId,
                                                         /* [retval][out] */ BSTR* result);
};

class CSalamanderFileOperationsAutomation : public CDispatchImpl<CSalamanderFileOperationsAutomation, ISalamanderFileOperations>
{
public:
    DECLARE_DISPOBJ_NAME(L"Salamander.FileOperations")

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE rename_interactive(/* [retval][out] */ BSTR* result);
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE copy_interactive(/* [retval][out] */ BSTR* result);
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE move_interactive(/* [retval][out] */ BSTR* result);
};
