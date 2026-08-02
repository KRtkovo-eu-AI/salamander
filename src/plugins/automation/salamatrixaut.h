// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Automation Plugin for Open Salamander

    salamatrixaut.h
    Script-facing Automation wrappers backed by Salamatrix runtime services.
*/

#pragma once

#include "dispimpl.h"
#include "../salamatrix/salamatrix_events.h"
#include "../salamatrix/salamatrix_commands.h"
#include "../salamatrix/salamatrix_sides.h"
#include "../salamatrix/salamatrix_storage.h"
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
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE controls(void);
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE dialog(BSTR title, long width, long height,
                                                         ISalamanderDialog** dialog);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_uptime(BSTR* milliseconds);
};

class CSalamatrixDialogAutomation : public CDispatchImpl<CSalamatrixDialogAutomation, ISalamanderDialog>
{
private:
    Salamatrix::UI::IUIService* m_pUIService;
    Salamatrix::UI::IDialog* m_pDialog;

public:
    DECLARE_DISPOBJ_NAME(L"Salamander.UI.Dialog")
    CSalamatrixDialogAutomation(Salamatrix::UI::IUIService* uiService,
                                Salamatrix::UI::IDialog* dialog);
    virtual ~CSalamatrixDialogAutomation();
    virtual HRESULT STDMETHODCALLTYPE add(BSTR kind, BSTR controlId, BSTR text,
                                           long x, long y, long width, long height,
                                           VARIANT* styleFlags, VARIANT* dialogResult);
    virtual HRESULT STDMETHODCALLTYPE set(BSTR controlId, BSTR property,
                                           VARIANT* value, VARIANT* value2);
    virtual HRESULT STDMETHODCALLTYPE show(long* result);
    virtual HRESULT STDMETHODCALLTYPE close();
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

class CSalamanderTabAutomation : public CDispatchImpl<CSalamanderTabAutomation, ISalamanderTab>
{
private:
    ULONGLONG m_tabId;

    HRESULT ReadInfo(Salamatrix::Sides::TabInfo& info);
    HRESULT ReadFlag(DWORD flag, VARIANT_BOOL* value);

public:
    DECLARE_DISPOBJ_NAME(L"Salamander.Tab")

    explicit CSalamanderTabAutomation(ULONGLONG tabId);

    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Id(BSTR* id);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Index(long* index);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Path(BSTR* path);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_PathType(long* pathType);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_IsActive(VARIANT_BOOL* value);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_IsSource(VARIANT_BOOL* value);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_IsTarget(VARIANT_BOOL* value);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_IsLocked(VARIANT_BOOL* value);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_IsDetached(VARIANT_BOOL* value);
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE Activate(VARIANT* focus);
};

class CSalamanderSideAutomation : public CDispatchImpl<CSalamanderSideAutomation, ISalamanderSide>
{
private:
    Salamatrix::Sides::SideReference m_side;

public:
    DECLARE_DISPOBJ_NAME(L"Salamander.Side")

    explicit CSalamanderSideAutomation(Salamatrix::Sides::SideReference side);

    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Name(BSTR* name);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_TabCount(long* count);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ActiveTab(ISalamanderTab** tab);
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE tab(long index, ISalamanderTab** result);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Path(BSTR* path);
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Path(BSTR path);
};

class CSalamanderSidesAutomation : public CDispatchImpl<CSalamanderSidesAutomation, ISalamanderSides>
{
private:
    HRESULT CreateSide(
        Salamatrix::Sides::SideReference side,
        ISalamanderSide** result);

public:
    DECLARE_DISPOBJ_NAME(L"Salamander.Sides")

    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Left(ISalamanderSide** side);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Right(ISalamanderSide** side);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Source(ISalamanderSide** side);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Target(ISalamanderSide** side);
};

class CSalamanderStorageAutomation : public CDispatchImpl<CSalamanderStorageAutomation, ISalamanderStorage>
{
private:
    char m_extensionId[128];

public:
    DECLARE_DISPOBJ_NAME(L"Salamander.Storage")

    explicit CSalamanderStorageAutomation(const char* extensionId);

    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Namespace(BSTR* extensionId);
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE has(BSTR key, VARIANT_BOOL* value);
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE get(BSTR key, VARIANT* defaultValue, VARIANT* value);
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE set(BSTR key, VARIANT* value);
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE remove(BSTR key, VARIANT_BOOL* removed);
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE clear();
};

class CSalamanderEventsAutomation : public CDispatchImpl<CSalamanderEventsAutomation, ISalamanderEvents>
{
private:
    struct Subscription
    {
        CSalamanderEventsAutomation* Owner;
        Salamatrix::Events::IEventsService* Service;
        ULONGLONG Id;
        IDispatch* Callback;
        bool Active;

        Subscription()
            : Owner(NULL),
              Service(NULL),
              Id(0),
              Callback(NULL),
              Active(false)
        {
        }
    };

    enum
    {
        MaxSubscriptions = 32
    };

    Subscription m_subscriptions[MaxSubscriptions];

    static BOOL WINAPI DispatchEvent(
        void* context,
        const Salamatrix::Events::EventPayload* payload);
    static bool ParseEventName(BSTR name, Salamatrix::Events::EventKind* kind);
    static BSTR EventKindToBSTR(Salamatrix::Events::EventKind kind);
    static BSTR UInt64ToBSTR(ULONGLONG value);
    HRESULT DispatchSubscription(
        Subscription* subscription,
        const Salamatrix::Events::EventPayload* payload);

public:
    DECLARE_DISPOBJ_NAME(L"Salamander.Events")

    virtual ~CSalamanderEventsAutomation();

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE subscribe(
        BSTR eventName,
        IDispatch* callback,
        BSTR* subscriptionId);
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE unsubscribe(BSTR subscriptionId);
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_SubscriptionCount(long* count);
};
