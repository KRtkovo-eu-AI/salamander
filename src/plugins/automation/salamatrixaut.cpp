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
    ::RaiseError(L"This script requires Salamatrix Framework to be installed and loaded.", __uuidof(ISalamander), objectName);
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
    case Salamatrix::Runtime::OperationResultNotAvailable:
        return SysAllocString(L"not_available");
    default:
        return SysAllocString(L"error");
    }
}

static Salamatrix::Sides::ISidesService* GetSidesService()
{
    g_oAutomationPlugin.RefreshSalamatrixServices();
    return g_oAutomationPlugin.GetSalamatrixBridge()->GetSidesService();
}

static Salamatrix::Storage::IStorageService* GetStorageService()
{
    g_oAutomationPlugin.RefreshSalamatrixServices();
    return g_oAutomationPlugin.GetSalamatrixBridge()->GetStorageService();
}

static HRESULT BstrToUtf8Buffer(
    BSTR value,
    char* buffer,
    int bufferSize)
{
    if (value == NULL || buffer == NULL || bufferSize <= 0)
        return E_INVALIDARG;
    UINT length = SysStringLen(value);
    if (wcslen(value) != length)
        return E_INVALIDARG;

    int converted = WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        value,
        -1,
        buffer,
        bufferSize,
        NULL,
        NULL);
    return converted > 0 ? S_OK : E_INVALIDARG;
}

static HRESULT BstrToUtf8Allocated(BSTR value, char** utf8)
{
    if (utf8 == NULL)
        return E_POINTER;
    *utf8 = NULL;
    if (value == NULL)
        return E_INVALIDARG;
    UINT length = SysStringLen(value);
    if (wcslen(value) != length)
        return E_INVALIDARG;

    int required = WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        value,
        -1,
        NULL,
        0,
        NULL,
        NULL);
    if (required <= 0)
        return E_INVALIDARG;

    char* converted =
        static_cast<char*>(malloc(required));
    if (converted == NULL)
        return E_OUTOFMEMORY;
    if (WideCharToMultiByte(
            CP_UTF8,
            WC_ERR_INVALID_CHARS,
            value,
            -1,
            converted,
            required,
            NULL,
            NULL) == 0)
    {
        free(converted);
        return E_INVALIDARG;
    }
    *utf8 = converted;
    return S_OK;
}

static BSTR Utf8ToBstr(const char* value)
{
    if (value == NULL)
        return NULL;
    int required = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        value,
        -1,
        NULL,
        0);
    if (required <= 0)
        return NULL;

    BSTR converted =
        SysAllocStringLen(NULL, required - 1);
    if (converted == NULL)
        return NULL;
    if (MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            value,
            -1,
            converted,
            required) == 0)
    {
        SysFreeString(converted);
        return NULL;
    }
    return converted;
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

static Salamatrix::UI::IControl* AddControlsShowcaseControl(
    Salamatrix::UI::IDialog* dialog,
    Salamatrix::UI::ControlKind kind,
    const char* id,
    const char* text,
    int x,
    int y,
    int width,
    int height,
    BOOL readOnly = FALSE,
    BOOL checked = FALSE,
    int dialogResult = 0,
    BOOL multiline = FALSE,
    const char* fileFilter = NULL)
{
    Salamatrix::UI::ControlOptions options;
    options.Id = id;
    options.Text = text;
    options.ReadOnly = readOnly;
    options.Checked = checked;
    options.DialogResult = dialogResult;
    options.Multiline = multiline;
    options.FileFilter = fileFilter;

    Salamatrix::UI::ControlLayout layout;
    layout.HasBounds = TRUE;
    layout.X = x;
    layout.Y = y;
    layout.Width = width;
    layout.Height = height;
    return dialog->AddControlEx(kind, options, layout);
}

/* [id] */ HRESULT STDMETHODCALLTYPE CSalamanderUINamespaceAutomation::controls(void)
{
    g_oAutomationPlugin.RefreshSalamatrixServices();
    Salamatrix::UI::IUIService* uiService =
        g_oAutomationPlugin.GetSalamatrixBridge()->GetUIService();
    if (uiService == NULL)
        return RaiseMissingRuntime(GetProgId());

    Salamatrix::UI::DialogOptions options;
    options.Title = "Salamatrix UI capabilities";
    options.Parent = SalamanderGeneral->GetMsgBoxParent();
    options.Width = 520;
    options.Height = 315;
    Salamatrix::UI::IDialog* dialog =
        uiService->CreateSalamatrixDialog(options);
    if (dialog == NULL)
        return RaiseMissingRuntime(GetProgId());

    bool complete = true;
    complete = AddControlsShowcaseControl(
                   dialog, Salamatrix::UI::ControlKindLabel,
                   "intro", "Controls provided by Salamatrix",
                   10, 8, 500, 12) != NULL && complete;
    complete = AddControlsShowcaseControl(
                   dialog, Salamatrix::UI::ControlKindLabel,
                   "text-heading", "Text and picker controls",
                   10, 28, 240, 12) != NULL && complete;
    complete = AddControlsShowcaseControl(
                   dialog, Salamatrix::UI::ControlKindTextBox,
                   "description",
                   "Native controls are shared by every Salamatrix runtime.\r\n"
                   "The dialog follows the current Salamander theme and DPI.",
                   10, 42, 240, 42, TRUE, FALSE, 0, TRUE) != NULL && complete;
    complete = AddControlsShowcaseControl(
                   dialog, Salamatrix::UI::ControlKindFilePicker,
                   "file", "C:\\Example\\document.txt",
                   10, 94, 240, 18, FALSE, FALSE, 0, FALSE,
                   "Text files|*.txt|All files|*.*") != NULL && complete;
    complete = AddControlsShowcaseControl(
                   dialog, Salamatrix::UI::ControlKindFolderPicker,
                   "folder", "Choose a folder...",
                   10, 118, 240, 18) != NULL && complete;
    complete = AddControlsShowcaseControl(
                   dialog, Salamatrix::UI::ControlKindCheckBox,
                   "checkbox", "Check box",
                   10, 146, 110, 14, FALSE, TRUE) != NULL && complete;
    complete = AddControlsShowcaseControl(
                   dialog, Salamatrix::UI::ControlKindRadioButton,
                   "radio", "Radio button",
                   130, 146, 120, 14, FALSE, TRUE) != NULL && complete;

    Salamatrix::UI::IControl* tabs = AddControlsShowcaseControl(
        dialog, Salamatrix::UI::ControlKindTabControl,
        "tabs", "", 10, 174, 240, 70);
    complete = tabs != NULL && tabs->AddItem("Overview") &&
               tabs->AddItem("Details") && tabs->SetSelectedIndex(0) && complete;

    complete = AddControlsShowcaseControl(
                   dialog, Salamatrix::UI::ControlKindLabel,
                   "collection-heading", "Choice and collection controls",
                   270, 28, 240, 12) != NULL && complete;
    Salamatrix::UI::IControl* choice = AddControlsShowcaseControl(
        dialog, Salamatrix::UI::ControlKindComboBox,
        "choice", "", 270, 42, 240, 80);
    complete = choice != NULL && choice->AddItem("Salamatrix UI") &&
               choice->AddItem("Native Win32 controls") &&
               choice->AddItem("Runtime-neutral API") &&
               choice->SetSelectedIndex(0) && complete;

    Salamatrix::UI::IControl* list = AddControlsShowcaseControl(
        dialog, Salamatrix::UI::ControlKindListView,
        "list", "", 270, 70, 240, 78);
    complete = list != NULL && list->AddColumn("Capability", 210) &&
               list->AddItem("Explicit layouts") &&
               list->AddItem("Validation and events") &&
               list->AddItem("Accessible metadata") &&
               list->SetSelectedIndex(0) && complete;

    Salamatrix::UI::IControl* tree = AddControlsShowcaseControl(
        dialog, Salamatrix::UI::ControlKindTreeView,
        "tree", "", 270, 158, 240, 86);
    complete = tree != NULL && tree->AddItem("Salamatrix UI") &&
               tree->AddItem("Dialogs", 0) && tree->AddItem("Controls", 0) &&
               complete;
    complete = AddControlsShowcaseControl(
                   dialog, Salamatrix::UI::ControlKindButton,
                   "close", "Close", 440, 276, 70, 22,
                   FALSE, FALSE, 1) != NULL && complete;

    if (complete)
        dialog->ShowModal();
    uiService->DestroyDialog(dialog);
    return complete ? S_OK : E_FAIL;
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

CSalamanderTabAutomation::CSalamanderTabAutomation(ULONGLONG tabId)
    : m_tabId(tabId)
{
}

HRESULT CSalamanderTabAutomation::ReadInfo(Salamatrix::Sides::TabInfo& info)
{
    Salamatrix::Sides::ISidesService* sides = GetSidesService();
    if (sides == NULL)
        return RaiseMissingRuntime(GetProgId());
    if (!sides->GetTabInfoById(m_tabId, &info))
    {
        ::RaiseError(L"The panel tab no longer exists.", __uuidof(ISalamanderTab), GetProgId());
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }
    return S_OK;
}

HRESULT CSalamanderTabAutomation::ReadFlag(DWORD flag, VARIANT_BOOL* value)
{
    if (value == NULL)
        return E_POINTER;
    Salamatrix::Sides::TabInfo info;
    HRESULT hr = ReadInfo(info);
    if (FAILED(hr))
        return hr;
    *value = (info.Flags & flag) != 0 ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderTabAutomation::get_Id(BSTR* id)
{
    if (id == NULL)
        return E_POINTER;
    wchar_t text[32];
    _ui64tow_s(m_tabId, text, _countof(text), 10);
    *id = SysAllocString(text);
    return *id != NULL ? S_OK : E_OUTOFMEMORY;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderTabAutomation::get_Index(long* index)
{
    if (index == NULL)
        return E_POINTER;
    Salamatrix::Sides::TabInfo info;
    HRESULT hr = ReadInfo(info);
    if (FAILED(hr))
        return hr;
    *index = info.Index;
    return S_OK;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderTabAutomation::get_Path(BSTR* path)
{
    if (path == NULL)
        return E_POINTER;
    *path = NULL;

    Salamatrix::Sides::ISidesService* sides = GetSidesService();
    if (sides == NULL)
        return RaiseMissingRuntime(GetProgId());

    char nativePath[32768];
    if (!sides->GetTabPath(m_tabId, nativePath, _countof(nativePath), NULL))
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);

    _bstr_t converted(nativePath);
    *path = converted.copy();
    return *path != NULL ? S_OK : E_OUTOFMEMORY;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderTabAutomation::get_PathType(long* pathType)
{
    if (pathType == NULL)
        return E_POINTER;
    Salamatrix::Sides::TabInfo info;
    HRESULT hr = ReadInfo(info);
    if (FAILED(hr))
        return hr;
    *pathType = info.PathType;
    return S_OK;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderTabAutomation::get_IsActive(VARIANT_BOOL* value)
{
    return ReadFlag(Salamatrix::Sides::TabFlagActiveOnSide, value);
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderTabAutomation::get_IsSource(VARIANT_BOOL* value)
{
    return ReadFlag(Salamatrix::Sides::TabFlagSource, value);
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderTabAutomation::get_IsTarget(VARIANT_BOOL* value)
{
    return ReadFlag(Salamatrix::Sides::TabFlagTarget, value);
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderTabAutomation::get_IsLocked(VARIANT_BOOL* value)
{
    return ReadFlag(Salamatrix::Sides::TabFlagLocked, value);
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderTabAutomation::get_IsDetached(VARIANT_BOOL* value)
{
    return ReadFlag(Salamatrix::Sides::TabFlagDetached, value);
}

/* [id] */ HRESULT STDMETHODCALLTYPE CSalamanderTabAutomation::Activate(VARIANT* focus)
{
    bool shouldFocus = true;
    if (IsArgumentPresent(focus))
    {
        try
        {
            shouldFocus = static_cast<bool>(_variant_t(focus));
        }
        catch (_com_error& error)
        {
            return error.Error();
        }
    }

    Salamatrix::Sides::ISidesService* sides = GetSidesService();
    if (sides == NULL)
        return RaiseMissingRuntime(GetProgId());
    return sides->ActivateTab(m_tabId, shouldFocus ? TRUE : FALSE)
               ? S_OK
               : HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
}

CSalamanderSideAutomation::CSalamanderSideAutomation(
    Salamatrix::Sides::SideReference side)
    : m_side(side)
{
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderSideAutomation::get_Name(BSTR* name)
{
    if (name == NULL)
        return E_POINTER;
    Salamatrix::Sides::ISidesService* sides = GetSidesService();
    if (sides == NULL)
        return RaiseMissingRuntime(GetProgId());
    *name = SysAllocString(
        sides->ResolveSide(m_side) == Salamatrix::Sides::SideReferenceRight
            ? L"Right"
            : L"Left");
    return *name != NULL ? S_OK : E_OUTOFMEMORY;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderSideAutomation::get_TabCount(long* count)
{
    if (count == NULL)
        return E_POINTER;
    Salamatrix::Sides::ISidesService* sides = GetSidesService();
    if (sides == NULL)
        return RaiseMissingRuntime(GetProgId());
    *count = sides->GetTabCount(m_side);
    return S_OK;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderSideAutomation::get_ActiveTab(ISalamanderTab** tab)
{
    if (tab == NULL)
        return E_POINTER;
    *tab = NULL;
    Salamatrix::Sides::ISidesService* sides = GetSidesService();
    if (sides == NULL)
        return RaiseMissingRuntime(GetProgId());

    Salamatrix::Sides::TabInfo info;
    if (!sides->GetActiveTabInfo(m_side, &info))
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    *tab = new CSalamanderTabAutomation(info.TabId);
    return S_OK;
}

/* [id] */ HRESULT STDMETHODCALLTYPE CSalamanderSideAutomation::tab(
    long index,
    ISalamanderTab** result)
{
    if (result == NULL)
        return E_POINTER;
    *result = NULL;
    if (index < 0)
        return E_INVALIDARG;

    Salamatrix::Sides::ISidesService* sides = GetSidesService();
    if (sides == NULL)
        return RaiseMissingRuntime(GetProgId());

    Salamatrix::Sides::TabInfo info;
    if (!sides->GetTabInfo(m_side, index, &info))
        return DISP_E_BADINDEX;
    *result = new CSalamanderTabAutomation(info.TabId);
    return S_OK;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderSideAutomation::get_Path(BSTR* path)
{
    if (path == NULL)
        return E_POINTER;
    *path = NULL;
    Salamatrix::Sides::ISidesService* sides = GetSidesService();
    if (sides == NULL)
        return RaiseMissingRuntime(GetProgId());

    Salamatrix::Sides::TabInfo info;
    char nativePath[32768];
    if (!sides->GetActiveTabInfo(m_side, &info) ||
        !sides->GetTabPath(info.TabId, nativePath, _countof(nativePath), NULL))
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }
    _bstr_t converted(nativePath);
    *path = converted.copy();
    return *path != NULL ? S_OK : E_OUTOFMEMORY;
}

/* [propput][id] */ HRESULT STDMETHODCALLTYPE CSalamanderSideAutomation::put_Path(BSTR path)
{
    if (path == NULL)
        return E_INVALIDARG;
    Salamatrix::Sides::ISidesService* sides = GetSidesService();
    if (sides == NULL)
        return RaiseMissingRuntime(GetProgId());

    _bstr_t nativePath(path);
    int failReason = 0;
    return sides->ChangeActiveTabPath(
               m_side, static_cast<const char*>(nativePath), &failReason)
               ? S_OK
               : E_FAIL;
}

HRESULT CSalamanderSidesAutomation::CreateSide(
    Salamatrix::Sides::SideReference side,
    ISalamanderSide** result)
{
    if (result == NULL)
        return E_POINTER;
    *result = new CSalamanderSideAutomation(side);
    return S_OK;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderSidesAutomation::get_Left(ISalamanderSide** side)
{
    return CreateSide(Salamatrix::Sides::SideReferenceLeft, side);
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderSidesAutomation::get_Right(ISalamanderSide** side)
{
    return CreateSide(Salamatrix::Sides::SideReferenceRight, side);
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderSidesAutomation::get_Source(ISalamanderSide** side)
{
    return CreateSide(Salamatrix::Sides::SideReferenceSource, side);
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE CSalamanderSidesAutomation::get_Target(ISalamanderSide** side)
{
    return CreateSide(Salamatrix::Sides::SideReferenceTarget, side);
}

CSalamanderStorageAutomation::CSalamanderStorageAutomation(
    const char* extensionId)
{
    strcpy_s(
        m_extensionId,
        _countof(m_extensionId),
        extensionId != NULL ? extensionId : "");
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE
CSalamanderStorageAutomation::get_Namespace(BSTR* extensionId)
{
    if (extensionId == NULL)
        return E_POINTER;
    *extensionId = Utf8ToBstr(m_extensionId);
    return *extensionId != NULL ? S_OK : E_OUTOFMEMORY;
}

/* [id] */ HRESULT STDMETHODCALLTYPE
CSalamanderStorageAutomation::has(
    BSTR key,
    VARIANT_BOOL* value)
{
    if (value == NULL)
        return E_POINTER;
    *value = VARIANT_FALSE;

    char nativeKey[768];
    HRESULT hr =
        BstrToUtf8Buffer(key, nativeKey, _countof(nativeKey));
    if (FAILED(hr))
        return hr;

    Salamatrix::Storage::IStorageService* storage =
        GetStorageService();
    if (storage == NULL)
        return RaiseMissingRuntime(GetProgId());
    *value =
        storage->GetValueType(m_extensionId, nativeKey) !=
                Salamatrix::Storage::StorageValueMissing
            ? VARIANT_TRUE
            : VARIANT_FALSE;
    return S_OK;
}

/* [id] */ HRESULT STDMETHODCALLTYPE
CSalamanderStorageAutomation::get(
    BSTR key,
    VARIANT* defaultValue,
    VARIANT* value)
{
    if (value == NULL)
        return E_POINTER;
    VariantInit(value);

    char nativeKey[768];
    HRESULT hr =
        BstrToUtf8Buffer(key, nativeKey, _countof(nativeKey));
    if (FAILED(hr))
        return hr;

    Salamatrix::Storage::IStorageService* storage =
        GetStorageService();
    if (storage == NULL)
        return RaiseMissingRuntime(GetProgId());

    Salamatrix::Storage::StorageValueType type =
        storage->GetValueType(m_extensionId, nativeKey);
    if (type == Salamatrix::Storage::StorageValueMissing)
    {
        if (IsArgumentPresent(defaultValue))
            return VariantCopy(value, defaultValue);
        V_VT(value) = VT_NULL;
        return S_OK;
    }

    if (type == Salamatrix::Storage::StorageValueString)
    {
        int required = 0;
        storage->GetString(
            m_extensionId,
            nativeKey,
            NULL,
            0,
            &required);
        if (required <= 0)
            return E_FAIL;

        char* text = static_cast<char*>(malloc(required));
        if (text == NULL)
            return E_OUTOFMEMORY;
        BOOL read = storage->GetString(
            m_extensionId,
            nativeKey,
            text,
            required,
            NULL);
        if (read)
        {
            V_BSTR(value) = Utf8ToBstr(text);
            V_VT(value) = VT_BSTR;
        }
        free(text);
        if (!read)
            return E_FAIL;
        return V_BSTR(value) != NULL ? S_OK : E_OUTOFMEMORY;
    }

    if (type == Salamatrix::Storage::StorageValueInteger)
    {
        LONGLONG integerValue = 0;
        if (!storage->GetInteger(
                m_extensionId,
                nativeKey,
                &integerValue))
        {
            return E_FAIL;
        }
        V_VT(value) = VT_I8;
        V_I8(value) = integerValue;
        return S_OK;
    }

    BOOL booleanValue = FALSE;
    if (!storage->GetBoolean(
            m_extensionId,
            nativeKey,
            &booleanValue))
    {
        return E_FAIL;
    }
    V_VT(value) = VT_BOOL;
    V_BOOL(value) =
        booleanValue ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

/* [id] */ HRESULT STDMETHODCALLTYPE
CSalamanderStorageAutomation::set(
    BSTR key,
    VARIANT* value)
{
    if (value == NULL)
        return E_POINTER;

    char nativeKey[768];
    HRESULT hr =
        BstrToUtf8Buffer(key, nativeKey, _countof(nativeKey));
    if (FAILED(hr))
        return hr;

    Salamatrix::Storage::IStorageService* storage =
        GetStorageService();
    if (storage == NULL)
        return RaiseMissingRuntime(GetProgId());

    VARIANT normalized;
    VariantInit(&normalized);
    hr = VariantCopyInd(&normalized, value);
    if (FAILED(hr))
        return hr;

    BOOL stored = FALSE;
    if (V_VT(&normalized) == VT_EMPTY ||
        V_VT(&normalized) == VT_NULL)
    {
        storage->DeleteValue(m_extensionId, nativeKey);
        stored = TRUE;
    }
    else if (V_VT(&normalized) == VT_BSTR)
    {
        char* text = NULL;
        hr = BstrToUtf8Allocated(V_BSTR(&normalized), &text);
        if (SUCCEEDED(hr))
        {
            stored = storage->SetString(
                m_extensionId,
                nativeKey,
                text);
            free(text);
        }
    }
    else if (V_VT(&normalized) == VT_BOOL)
    {
        stored = storage->SetBoolean(
            m_extensionId,
            nativeKey,
            V_BOOL(&normalized) == VARIANT_TRUE);
    }
    else
    {
        VARIANT integerValue;
        VariantInit(&integerValue);
        hr = VariantChangeType(
            &integerValue,
            &normalized,
            0,
            VT_I8);
        if (SUCCEEDED(hr))
        {
            stored = storage->SetInteger(
                m_extensionId,
                nativeKey,
                V_I8(&integerValue));
        }
        VariantClear(&integerValue);
    }
    VariantClear(&normalized);

    if (FAILED(hr))
        return hr;
    if (!stored)
    {
        ::RaiseError(
            L"Storage keys may contain A-Z, 0-9, dot, underscore, dash, and colon; strings are limited to 16 KiB.",
            __uuidof(ISalamanderStorage),
            GetProgId());
        return E_INVALIDARG;
    }
    return S_OK;
}

/* [id] */ HRESULT STDMETHODCALLTYPE
CSalamanderStorageAutomation::remove(
    BSTR key,
    VARIANT_BOOL* removed)
{
    if (removed == NULL)
        return E_POINTER;
    *removed = VARIANT_FALSE;

    char nativeKey[768];
    HRESULT hr =
        BstrToUtf8Buffer(key, nativeKey, _countof(nativeKey));
    if (FAILED(hr))
        return hr;

    Salamatrix::Storage::IStorageService* storage =
        GetStorageService();
    if (storage == NULL)
        return RaiseMissingRuntime(GetProgId());
    *removed =
        storage->DeleteValue(m_extensionId, nativeKey)
            ? VARIANT_TRUE
            : VARIANT_FALSE;
    return S_OK;
}

/* [id] */ HRESULT STDMETHODCALLTYPE
CSalamanderStorageAutomation::clear()
{
    Salamatrix::Storage::IStorageService* storage =
        GetStorageService();
    if (storage == NULL)
        return RaiseMissingRuntime(GetProgId());
    return storage->ClearExtension(m_extensionId)
               ? S_OK
               : E_INVALIDARG;
}

bool CSalamanderEventsAutomation::ParseEventName(
    BSTR name,
    Salamatrix::Events::EventKind* kind)
{
    if (name == NULL || kind == NULL)
        return false;
    if (_wcsicmp(name, L"startup") == 0 ||
        _wcsicmp(name, L"hostStartup") == 0)
    {
        *kind = Salamatrix::Events::EventKindHostStartup;
    }
    else if (_wcsicmp(name, L"shutdown") == 0 ||
             _wcsicmp(name, L"hostShutdown") == 0)
    {
        *kind = Salamatrix::Events::EventKindHostShutdown;
    }
    else if (_wcsicmp(name, L"settingsChanged") == 0 ||
             _wcsicmp(name, L"settings_changed") == 0)
    {
        *kind = Salamatrix::Events::EventKindSettingsChanged;
    }
    else if (_wcsicmp(name, L"configurationChanged") == 0 ||
             _wcsicmp(name, L"configuration_changed") == 0)
    {
        *kind = Salamatrix::Events::EventKindConfigurationChanged;
    }
    else if (_wcsicmp(name, L"colorsChanged") == 0 ||
             _wcsicmp(name, L"colors_changed") == 0)
    {
        *kind = Salamatrix::Events::EventKindColorsChanged;
    }
    else if (_wcsicmp(name, L"panelsSwapped") == 0 ||
             _wcsicmp(name, L"panels_swapped") == 0)
    {
        *kind = Salamatrix::Events::EventKindPanelsSwapped;
    }
    else if (_wcsicmp(name, L"activePanelChanged") == 0 ||
             _wcsicmp(name, L"active_panel_changed") == 0)
    {
        *kind = Salamatrix::Events::EventKindActivePanelChanged;
    }
    else if (_wcsicmp(name, L"fileChanged") == 0 ||
             _wcsicmp(name, L"file_changed") == 0)
    {
        *kind = Salamatrix::Events::EventKindFileChanged;
    }
    else
    {
        return false;
    }
    return true;
}

BSTR CSalamanderEventsAutomation::EventKindToBSTR(
    Salamatrix::Events::EventKind kind)
{
    switch (kind)
    {
    case Salamatrix::Events::EventKindHostStartup:
        return SysAllocString(L"startup");
    case Salamatrix::Events::EventKindHostShutdown:
        return SysAllocString(L"shutdown");
    case Salamatrix::Events::EventKindSettingsChanged:
        return SysAllocString(L"settingsChanged");
    case Salamatrix::Events::EventKindConfigurationChanged:
        return SysAllocString(L"configurationChanged");
    case Salamatrix::Events::EventKindColorsChanged:
        return SysAllocString(L"colorsChanged");
    case Salamatrix::Events::EventKindPanelsSwapped:
        return SysAllocString(L"panelsSwapped");
    case Salamatrix::Events::EventKindActivePanelChanged:
        return SysAllocString(L"activePanelChanged");
    case Salamatrix::Events::EventKindFileChanged:
        return SysAllocString(L"fileChanged");
    default:
        return NULL;
    }
}

BSTR CSalamanderEventsAutomation::UInt64ToBSTR(ULONGLONG value)
{
    wchar_t text[32];
    if (_ui64tow_s(value, text, _countof(text), 10) != 0)
        return NULL;
    return SysAllocString(text);
}

BOOL WINAPI CSalamanderEventsAutomation::DispatchEvent(
    void* context,
    const Salamatrix::Events::EventPayload* payload)
{
    Subscription* subscription =
        static_cast<Subscription*>(context);
    if (subscription == NULL ||
        !subscription->Active ||
        subscription->Owner == NULL)
    {
        return FALSE;
    }
    return SUCCEEDED(subscription->Owner->DispatchSubscription(
        subscription,
        payload))
               ? TRUE
               : FALSE;
}

HRESULT CSalamanderEventsAutomation::DispatchSubscription(
    Subscription* subscription,
    const Salamatrix::Events::EventPayload* payload)
{
    if (subscription == NULL ||
        !subscription->Active ||
        subscription->Callback == NULL ||
        payload == NULL)
    {
        return E_INVALIDARG;
    }

    BSTR name = EventKindToBSTR(payload->Kind);
    BSTR tabId = UInt64ToBSTR(payload->ActiveTabId);
    BSTR path = SysAllocString(A2OLE(payload->Path));
    if (name == NULL || tabId == NULL || path == NULL)
    {
        SysFreeString(name);
        SysFreeString(tabId);
        SysFreeString(path);
        return E_OUTOFMEMORY;
    }

    VARIANT arguments[4];
    for (int index = 0; index < _countof(arguments); ++index)
        VariantInit(&arguments[index]);
    V_VT(&arguments[0]) = VT_BSTR;
    V_BSTR(&arguments[0]) = tabId;
    V_VT(&arguments[1]) = VT_BSTR;
    V_BSTR(&arguments[1]) = path;
    V_VT(&arguments[2]) = VT_UI4;
    V_UI4(&arguments[2]) = payload->Parameter;
    V_VT(&arguments[3]) = VT_BSTR;
    V_BSTR(&arguments[3]) = name;

    DISPPARAMS parameters;
    parameters.rgvarg = arguments;
    parameters.rgdispidNamedArgs = NULL;
    parameters.cArgs = _countof(arguments);
    parameters.cNamedArgs = 0;
    EXCEPINFO exceptionInfo;
    memset(&exceptionInfo, 0, sizeof(exceptionInfo));
    UINT argumentError = 0;
    HRESULT hr = subscription->Callback->Invoke(
        DISPID_VALUE,
        IID_NULL,
        LOCALE_USER_DEFAULT,
        DISPATCH_METHOD,
        &parameters,
        NULL,
        &exceptionInfo,
        &argumentError);

    for (int index = 0; index < _countof(arguments); ++index)
        VariantClear(&arguments[index]);
    SysFreeString(name);
    SysFreeString(tabId);
    SysFreeString(path);
    SysFreeString(exceptionInfo.bstrSource);
    SysFreeString(exceptionInfo.bstrDescription);
    SysFreeString(exceptionInfo.bstrHelpFile);
    return hr;
}

CSalamanderEventsAutomation::~CSalamanderEventsAutomation()
{
    for (int index = 0; index < _countof(m_subscriptions); ++index)
    {
        Subscription& subscription = m_subscriptions[index];
        if (!subscription.Active)
            continue;
        if (subscription.Service != NULL)
            subscription.Service->Unsubscribe(subscription.Id);
        if (subscription.Callback != NULL)
            subscription.Callback->Release();
        subscription = Subscription();
    }
}

/* [id] */ HRESULT STDMETHODCALLTYPE CSalamanderEventsAutomation::subscribe(
    BSTR eventName,
    IDispatch* callback,
    BSTR* subscriptionId)
{
    if (callback == NULL || subscriptionId == NULL)
        return E_POINTER;
    *subscriptionId = NULL;

    Salamatrix::Events::EventKind kind;
    if (!ParseEventName(eventName, &kind))
    {
        ::RaiseError(
            L"Unknown Salamander event name.",
            __uuidof(ISalamanderEvents),
            GetProgId());
        return E_INVALIDARG;
    }

    g_oAutomationPlugin.RefreshSalamatrixServices();
    Salamatrix::Events::IEventsService* service =
        g_oAutomationPlugin.GetSalamatrixBridge()->GetEventsService();
    if (service == NULL)
        return RaiseMissingRuntime(GetProgId());

    Subscription* slot = NULL;
    for (int index = 0; index < _countof(m_subscriptions); ++index)
    {
        if (!m_subscriptions[index].Active)
        {
            slot = &m_subscriptions[index];
            break;
        }
    }
    if (slot == NULL)
        return E_OUTOFMEMORY;

    slot->Owner = this;
    slot->Service = service;
    slot->Callback = callback;
    slot->Callback->AddRef();
    ULONGLONG id = 0;
    if (!service->Subscribe(
            kind,
            DispatchEvent,
            slot,
            &id))
    {
        slot->Callback->Release();
        *slot = Subscription();
        return E_FAIL;
    }
    slot->Id = id;
    slot->Active = true;
    *subscriptionId = UInt64ToBSTR(id);
    if (*subscriptionId == NULL)
    {
        service->Unsubscribe(id);
        slot->Callback->Release();
        *slot = Subscription();
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

/* [id] */ HRESULT STDMETHODCALLTYPE CSalamanderEventsAutomation::unsubscribe(
    BSTR subscriptionId)
{
    if (subscriptionId == NULL || subscriptionId[0] == 0)
        return E_INVALIDARG;
    wchar_t* end = NULL;
    unsigned __int64 parsed = _wcstoui64(subscriptionId, &end, 10);
    if (end == subscriptionId || *end != 0 || parsed == 0)
        return E_INVALIDARG;

    for (int index = 0; index < _countof(m_subscriptions); ++index)
    {
        Subscription& subscription = m_subscriptions[index];
        if (subscription.Active && subscription.Id == parsed)
        {
            if (subscription.Service != NULL)
                subscription.Service->Unsubscribe(subscription.Id);
            if (subscription.Callback != NULL)
                subscription.Callback->Release();
            subscription = Subscription();
            return S_OK;
        }
    }
    return S_FALSE;
}

/* [propget][id] */ HRESULT STDMETHODCALLTYPE
CSalamanderEventsAutomation::get_SubscriptionCount(long* count)
{
    if (count == NULL)
        return E_POINTER;
    long active = 0;
    for (int index = 0; index < _countof(m_subscriptions); ++index)
    {
        if (m_subscriptions[index].Active)
            ++active;
    }
    *count = active;
    return S_OK;
}
