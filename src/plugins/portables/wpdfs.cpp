// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later

// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-04 | Fix rename progress title and Total bar by summing sizes before upload. | english.slg lacked IDS_OPERATION_RENAME, and AddTotal during the copy made Total jump ahead of File. | | Grok 4.7 |
// | 2026-11-04 | Open Salamander progress without Salamatrix, match folder renames on either name, and find children in one bulk property pass. | Overwrite waited on one GetValues per sibling, folder rename fell into a full download after ORIGINAL_FILE_NAME stayed old, and progress never appeared when Salamatrix.UI was missing. | | Grok 4.7 |
// | 2026-11-04 | Skip a missing host operations method, refresh panels on device change, and keep the copy buffer off the stack. | Calling the new slot in an older salamand.exe executes an RTTI record and kills the process. The device list stayed cached until a manual refresh. | | Grok 4.7 |
// | 2026-11-04 | Include the standard containers through precomp.h. | Debug builds with a precompiled header never saw the <vector> include above it. | | Grok 4.7 |
// | 2026-11-04 | Overwrite, merge, and rename portable items through Salamander progress. | Creating an object whose name is still present wedges Android MTP, and folder renames must survive devices that ignore WPD_OBJECT_NAME. | | Grok 4.7 |

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
#include "globals.h"
#include "lang\lang.rh"
#include "wpdnamepolicy.h"

////////////////////////////////////////////////////////////////////////////////
// CWpdFS

extern CWpdDeviceList g_oDeviceList;

static std::vector<CWpdFS*> WpdOpenPanels;

void WINAPI WpdRefreshPanelsForDeviceChange()
{
    g_oDeviceList.SetForceUpdate();
    if (SalamanderGeneral == nullptr)
    {
        return;
    }

    std::vector<CWpdFS*> panels = WpdOpenPanels;
    for (size_t i = 0; i < panels.size(); ++i)
    {
        SalamanderGeneral->PostRefreshPanelFS(panels[i], FALSE);
    }
}

PCSTR CWpdFS::SUGGESTED_NAME = SuggestedFSName;

CWpdFS::CWpdFS(CFxPluginInterfaceForFS& owner)
    : TFxPluginFSInterface(owner)
{
    WpdOpenPanels.push_back(this);
}

CWpdFS::~CWpdFS()
{
    for (size_t i = 0; i < WpdOpenPanels.size(); ++i)
    {
        if (WpdOpenPanels[i] == this)
        {
            WpdOpenPanels.erase(WpdOpenPanels.begin() + static_cast<std::ptrdiff_t>(i));
            break;
        }
    }
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


static PCSTR WpdLoadStr(UINT id)
{
    return SalamanderGeneral->LoadStr(Fx::FxGetLangInstance(), id);
}

static std::string WpdText(UINT id)
{
    const char* text = WpdLoadStr(id);
    if (text != nullptr && lstrcmpA(text, "ERROR LOADING STRING") != 0)
    {
        return std::string(text);
    }
    // Fallback when the staged .slg is older than the plug-in and is missing
    // the new string IDs (rename/overwrite titles showed ERROR LOADING STRING).
    switch (id)
    {
    case IDS_OPERATION_FAILED_FMT:
        return "%s '%s' failed (0x%08X).";
    case IDS_OPERATION_RENAME:
        return "Rename";
    case IDS_OPERATION_CREATEFOLDER:
        return "Create Folder";
    case IDS_OPERATION_VIEW:
        return "View";
    case IDS_OPERATION_NAME_STILL_EXISTS:
        return "The name is still on the device. The item was not created again.";
    case IDS_OPERATION_TYPE_MISMATCH_FILE:
        return "A file cannot replace a folder.";
    case IDS_OPERATION_TYPE_MISMATCH_DIR:
        return "A folder cannot replace a file.";
    case IDS_OPERATION_ALREADY_EXISTS:
        return "An item with this name already exists.";
    case IDS_OPERATION_DETAILS_UNKNOWN:
        return "?";
    case IDS_OPERATIONPROGRESS_COPY:
        return "Copy";
    case IDS_OPERATIONPROGRESS_MOVE:
        return "Move";
    case IDS_OPERATIONPROGRESS_DELETE:
        return "Delete";
    case IDS_OPERATIONPROGRESS_TITLE:
        return "Portable Devices";
    default:
        return text != nullptr ? std::string(text) : std::string();
    }
}

static HWND WpdDialogOwner = nullptr;
static bool WpdReportedError = false;
class CWpdStandardProgress;
static CWpdStandardProgress* WpdActiveProgress = nullptr;

// MSVC turns a pointer to a virtual method into a thunk that jumps through
// the vtable slot. An older salamand.exe does not have GetSalamanderForOperations,
// so that slot is the next object's RTTI record. Executing it is the access
// violation from the rename and overwrite dumps. Read the slot and accept it
// only when it points at executable code.
static size_t WpdOperationsVtableOffset()
{
    using Method = CSalamanderForOperationsAbstract*(WINAPI CSalamanderGeneralAbstract::*)(HWND);
    Method method = &CSalamanderGeneralAbstract::GetSalamanderForOperations;
    void* thunk = nullptr;
    memcpy(&thunk, &method, sizeof(thunk));

    MEMORY_BASIC_INFORMATION info;
    if (thunk == nullptr || ::VirtualQuery(thunk, &info, sizeof(info)) == 0)
    {
        return static_cast<size_t>(-1);
    }
    const DWORD protect = info.Protect & 0xFF;
    if (protect != PAGE_EXECUTE && protect != PAGE_EXECUTE_READ &&
        protect != PAGE_EXECUTE_READWRITE && protect != PAGE_EXECUTE_WRITECOPY)
    {
        return static_cast<size_t>(-1);
    }

    const unsigned char* code = static_cast<const unsigned char*>(thunk);
    size_t opcode = static_cast<size_t>(-1);
    // x64: mov rax, [rcx] / jmp|call [rax+slot]
    if (code[0] == 0x48 && code[1] == 0x8B && code[2] == 0x01 && code[3] == 0xFF)
    {
        opcode = 4;
    }
    // x86: mov eax, [ecx] / jmp|call [eax+slot]
    else if (code[0] == 0x8B && code[1] == 0x01 && code[2] == 0xFF)
    {
        opcode = 3;
    }
    if (opcode == static_cast<size_t>(-1))
    {
        return static_cast<size_t>(-1);
    }
    if (code[opcode] == 0x60)
    {
        return code[opcode + 1];
    }
    if (code[opcode] == 0xA0 || code[opcode] == 0x90)
    {
        int displacement = 0;
        memcpy(&displacement, code + opcode + 1, sizeof(displacement));
        if (displacement >= 0)
        {
            return static_cast<size_t>(displacement);
        }
    }
    return static_cast<size_t>(-1);
}

static bool WpdHostExportsOperations()
{
    const size_t offset = WpdOperationsVtableOffset();
    if (offset == static_cast<size_t>(-1) || SalamanderGeneral == nullptr)
    {
        return false;
    }

    void** vtable = *reinterpret_cast<void***>(SalamanderGeneral);
    if (vtable == nullptr)
    {
        return false;
    }

    MEMORY_BASIC_INFORMATION info;
    void* slot = reinterpret_cast<unsigned char*>(vtable) + offset;
    if (::VirtualQuery(slot, &info, sizeof(info)) == 0)
    {
        return false;
    }
    const DWORD slotProtect = info.Protect & 0xFF;
    const bool slotReadable = slotProtect == PAGE_READONLY || slotProtect == PAGE_READWRITE ||
                              slotProtect == PAGE_WRITECOPY || slotProtect == PAGE_EXECUTE_READ ||
                              slotProtect == PAGE_EXECUTE_READWRITE || slotProtect == PAGE_EXECUTE_WRITECOPY;
    if (!slotReadable)
    {
        return false;
    }

    void* target = *reinterpret_cast<void**>(slot);
    if (::VirtualQuery(target, &info, sizeof(info)) == 0)
    {
        return false;
    }
    const DWORD targetProtect = info.Protect & 0xFF;
    return targetProtect == PAGE_EXECUTE || targetProtect == PAGE_EXECUTE_READ ||
           targetProtect == PAGE_EXECUTE_READWRITE || targetProtect == PAGE_EXECUTE_WRITECOPY;
}

static HWND WpdDialogParent(HWND fallback)
{
    return WpdDialogOwner != nullptr ? WpdDialogOwner : fallback;
}

class CWpdStandardProgress
{
public:
    CWpdStandardProgress()
        : m_operations(nullptr),
          m_previousOwner(nullptr),
          m_totalBytes(0),
          m_doneBytes(0),
          m_opened(false),
          m_opsDialog(false),
          m_waitWindow(false)
    {
    }

    ~CWpdStandardProgress()
    {
        Close();
    }

    void AddTotal(ULONGLONG bytes)
    {
        if (bytes != 0 && bytes != static_cast<ULONGLONG>(-1))
        {
            m_totalBytes += bytes;
        }
    }

    // Push the summed Total size into the dialog before the first file starts.
    // Calling this after every AddTotal during the copy made Total race ahead
    // of File because the denominator was still only the files seen so far.
    void CommitTotals()
    {
        if (!m_opsDialog || m_operations == nullptr)
        {
            return;
        }
        CQuadWord unchanged(-1, -1);
        CQuadWord allTotal;
        allTotal.SetUI64(m_totalBytes == 0 ? 1 : m_totalBytes);
        m_operations->ProgressSetTotalSize(unchanged, allTotal);
        CQuadWord zero(0, 0);
        CQuadWord done;
        done.SetUI64(m_doneBytes);
        m_operations->ProgressSetSize(unchanged, done, FALSE);
    }

    // Prefer the host progress dialog. When this host build has no
    // GetSalamanderForOperations slot, show the Safe Wait window so the user
    // still sees that the plugin is working.
    bool Open(HWND panel, const std::string& title)
    {
        Close();
        m_previousOwner = WpdDialogOwner;
        WpdDialogOwner = panel;
        WpdActiveProgress = this;
        m_opened = true;

        if (SalamanderGeneral != nullptr && WpdHostExportsOperations())
        {
            m_operations = SalamanderGeneral->GetSalamanderForOperations(panel);
            if (m_operations != nullptr)
            {
                m_operations->OpenProgressDialog(title.c_str(), TRUE, panel, TRUE);
                m_opsDialog = true;
                HWND progressWindow = m_operations->ProgressGetHWND();
                if (progressWindow != nullptr)
                {
                    WpdDialogOwner = progressWindow;
                }
                m_operations->ProgressEnableCancel(TRUE);
                return true;
            }
        }

        if (SalamanderGeneral != nullptr)
        {
            SalamanderGeneral->CreateSafeWaitWindow(title.c_str(), nullptr, 0, TRUE, panel);
            m_waitWindow = true;
        }
        return true;
    }

    bool BeginItem(const char* name, ULONGLONG fileBytes)
    {
        if (m_opsDialog && m_operations != nullptr)
        {
            if (name != nullptr && name[0] != '\0')
            {
                m_operations->ProgressDialogAddText(name, FALSE);
            }

            CQuadWord unchanged(-1, -1);
            CQuadWord fileTotal;
            if (fileBytes == static_cast<ULONGLONG>(-1))
            {
                // Keep the existing File total; only refresh the Total bar.
                fileTotal = unchanged;
            }
            else
            {
                fileTotal.SetUI64(fileBytes == 0 ? 1 : fileBytes);
            }
            CQuadWord allTotal;
            if (m_totalBytes == 0)
            {
                allTotal = unchanged;
            }
            else
            {
                allTotal.SetUI64(m_totalBytes);
            }
            m_operations->ProgressSetTotalSize(fileTotal, allTotal);

            CQuadWord filePos(0, 0);
            CQuadWord done;
            done.SetUI64(m_doneBytes);
            return m_operations->ProgressSetSize(filePos, done, FALSE);
        }

        if (m_waitWindow && SalamanderGeneral != nullptr)
        {
            if (name != nullptr && name[0] != '\0')
            {
                SalamanderGeneral->SetSafeWaitWindowText(name);
            }
            return !SalamanderGeneral->GetSafeWaitWindowClosePressed();
        }
        return true;
    }

    bool OnBytes(ULONGLONG copied, ULONGLONG total)
    {
        UNREFERENCED_PARAMETER(total);
        if (m_opsDialog && m_operations != nullptr)
        {
            CQuadWord filePos;
            CQuadWord done;
            filePos.SetUI64(copied);
            done.SetUI64(m_doneBytes + copied);
            return m_operations->ProgressSetSize(filePos, done, TRUE);
        }
        if (m_waitWindow && SalamanderGeneral != nullptr)
        {
            return !SalamanderGeneral->GetSafeWaitWindowClosePressed();
        }
        return true;
    }

    void EndItem(ULONGLONG fileBytes)
    {
        if (fileBytes != static_cast<ULONGLONG>(-1))
        {
            m_doneBytes += fileBytes;
        }
        if (m_opsDialog && m_operations != nullptr)
        {
            CQuadWord filePos;
            CQuadWord done;
            filePos.SetUI64(fileBytes == static_cast<ULONGLONG>(-1) ? 0 : fileBytes);
            done.SetUI64(m_doneBytes);
            m_operations->ProgressSetSize(filePos, done, FALSE);
        }
    }

    void Close()
    {
        if (WpdActiveProgress == this)
        {
            WpdActiveProgress = nullptr;
        }
        if (m_opsDialog && m_operations != nullptr)
        {
            m_operations->CloseProgressDialog();
        }
        if (m_waitWindow && SalamanderGeneral != nullptr)
        {
            SalamanderGeneral->DestroySafeWaitWindow();
        }
        m_operations = nullptr;
        m_opsDialog = false;
        m_waitWindow = false;
        if (m_opened)
        {
            WpdDialogOwner = m_previousOwner;
            m_opened = false;
        }
    }

private:
    CSalamanderForOperationsAbstract* m_operations;
    HWND m_previousOwner;
    ULONGLONG m_totalBytes;
    ULONGLONG m_doneBytes;
    bool m_opened;
    bool m_opsDialog;
    bool m_waitWindow;
};

static HRESULT WINAPI WpdCopyStream(IStream* source, IStream* target, ULONGLONG size)
{
    _ASSERTE(source != nullptr);
    _ASSERTE(target != nullptr);

    // The copy runs on the UI thread under the progress dialog. A 64 KB stack
    // buffer there is large enough to overflow a deep call.
    std::vector<BYTE> buffer(64 * 1024);
    ULONGLONG copied = 0;
    for (;;)
    {
        if (size != static_cast<ULONGLONG>(-1) && copied >= size)
        {
            return S_OK;
        }

        ULONG read = 0;
        HRESULT hr = source->Read(buffer.data(), static_cast<ULONG>(buffer.size()), &read);
        if (FAILED(hr))
        {
            return hr;
        }
        if (read == 0)
        {
            if (WpdActiveProgress != nullptr)
            {
                WpdActiveProgress->OnBytes(copied, copied != 0 ? copied : 1);
            }
            return S_OK;
        }

        ULONG toWrite = read;
        if (size != static_cast<ULONGLONG>(-1) && copied + toWrite > size)
        {
            toWrite = static_cast<ULONG>(size - copied);
        }

        ULONG writtenTotal = 0;
        while (writtenTotal < toWrite)
        {
            ULONG written = 0;
            hr = target->Write(buffer.data() + writtenTotal, toWrite - writtenTotal, &written);
            if (FAILED(hr))
            {
                return hr;
            }
            if (written == 0)
            {
                return STG_E_MEDIUMFULL;
            }
            writtenTotal += written;
        }

        copied += toWrite;
        if (WpdActiveProgress != nullptr && !WpdActiveProgress->OnBytes(copied, size))
        {
            return HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
        if (hr == S_FALSE)
        {
            if (WpdActiveProgress != nullptr)
            {
                WpdActiveProgress->OnBytes(copied, copied != 0 ? copied : 1);
            }
            return S_OK;
        }
    }
}

static bool WINAPI WpdIsOperationCancelled(HRESULT hr)
{
    return hr == HRESULT_FROM_WIN32(ERROR_CANCELLED);
}

static bool WINAPI WpdRequiresDeviceReconnect(HRESULT hr)
{
    return hr == HRESULT_FROM_WIN32(ERROR_SEM_TIMEOUT) ||
           hr == HRESULT_FROM_WIN32(ERROR_GEN_FAILURE) ||
           hr == HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_CONNECTED) ||
           hr == HRESULT_FROM_WIN32(ERROR_DEV_NOT_EXIST)
#ifdef ERROR_DEVICE_REINITIALIZATION_NEEDED
           || hr == HRESULT_FROM_WIN32(ERROR_DEVICE_REINITIALIZATION_NEEDED)
#endif
        ;
}

static bool WINAPI WpdIsMoveUnsupported(HRESULT hr)
{
    return hr == E_NOTIMPL ||
           hr == E_NOINTERFACE ||
           hr == HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED) ||
           hr == HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED) ||
           hr == HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
}

static std::string WpdFormatOperationError(UINT operationId, const char* name, HRESULT hr)
{
    std::string format = WpdText(IDS_OPERATION_FAILED_FMT);
    std::string operation = WpdText(operationId);
    const char* shown = name != nullptr ? name : "";
    std::string message(format.size() + operation.size() + strlen(shown) + 48, '\0');
    if (FAILED(StringCchPrintfA(&message[0], message.size(), format.c_str(), operation.c_str(), shown, static_cast<unsigned>(hr))))
    {
        return operation;
    }
    message.resize(strlen(message.c_str()));
    return message;
}

static void WINAPI WpdShowOperationError(HWND parent, UINT operationId, const char* name, HRESULT hr)
{
    if (WpdIsOperationCancelled(hr))
    {
        return;
    }

    WpdReportedError = true;
    std::string message = WpdFormatOperationError(operationId, name, hr);
    std::string title = WpdText(IDS_OPERATIONPROGRESS_TITLE);
    SalamanderGeneral->DialogError(WpdDialogParent(parent), BUTTONS_OK, name != nullptr ? name : "", message.c_str(), title.c_str());
}

static int WpdStrICmp(const char* left, const char* right)
{
    if (SalamanderGeneral == nullptr)
    {
        return lstrcmpiA(left, right);
    }
    return SalamanderGeneral->StrICmp(left, right);
}

static bool WpdUtf8ToWide(const char* text, std::wstring& wide)
{
    wide.clear();
    if (text == nullptr)
    {
        return false;
    }
    if (text[0] == '\0')
    {
        return true;
    }

    int length = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, nullptr, 0);
    if (length <= 1)
    {
        return false;
    }
    wide.resize(static_cast<size_t>(length - 1));
    return ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, &wide[0], length) > 0;
}

static bool WpdWideToUtf8(const wchar_t* text, std::string& utf8)
{
    utf8.clear();
    if (text == nullptr)
    {
        return false;
    }
    if (text[0] == L'\0')
    {
        return true;
    }

    int length = ::WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (length <= 1)
    {
        return false;
    }
    utf8.resize(static_cast<size_t>(length - 1));
    return ::WideCharToMultiByte(CP_UTF8, 0, text, -1, &utf8[0], length, nullptr, nullptr) > 0;
}

static HRESULT WpdPanelTextToWide(const char* text, std::wstring& wide)
{
    if (text == nullptr)
    {
        return E_INVALIDARG;
    }
    if (WpdUtf8ToWide(text, wide))
    {
        return S_OK;
    }

    int length = ::MultiByteToWideChar(CP_ACP, 0, text, -1, nullptr, 0);
    if (length <= 0)
    {
        return HRESULT_FROM_WIN32(::GetLastError());
    }
    wide.assign(static_cast<size_t>(length > 0 ? length - 1 : 0), L'\0');
    if (length > 1 && ::MultiByteToWideChar(CP_ACP, 0, text, -1, &wide[0], length) <= 0)
    {
        return HRESULT_FROM_WIN32(::GetLastError());
    }
    return S_OK;
}

static bool WpdWideNameEqualsPanelName(const wchar_t* wideName, const char* panelName)
{
    std::wstring panelWide;
    std::string left;
    std::string right;
    if (FAILED(WpdPanelTextToWide(panelName, panelWide)) ||
        !WpdWideToUtf8(wideName, left) ||
        !WpdWideToUtf8(panelWide.c_str(), right))
    {
        return false;
    }
    return WpdNamesEqualInsensitive(left.c_str(), right.c_str(), WpdStrICmp);
}

static bool WpdStoredNamesMatch(const std::wstring& originalName, bool hasOriginal,
                                const std::wstring& objectName, bool hasObject,
                                const char* panelName)
{
    std::wstring panelWide;
    std::string wanted;
    if (FAILED(WpdPanelTextToWide(panelName, panelWide)) || !WpdWideToUtf8(panelWide.c_str(), wanted))
    {
        return false;
    }

    std::string originalUtf8;
    std::string objectUtf8;
    bool originalOk = hasOriginal && WpdWideToUtf8(originalName.c_str(), originalUtf8);
    bool objectOk = hasObject && WpdWideToUtf8(objectName.c_str(), objectUtf8);
    return WpdChildMatchesEitherName(originalOk ? originalUtf8.c_str() : "", originalOk,
                                     objectOk ? objectUtf8.c_str() : "", objectOk,
                                     wanted.c_str(), WpdStrICmp);
}

static bool WpdDisplayNameMatches(const std::wstring& originalName, bool hasOriginal,
                                  const std::wstring& objectName, bool hasObject,
                                  const char* wantedName)
{
    if (hasOriginal)
    {
        return WpdWideNameEqualsPanelName(originalName.c_str(), wantedName);
    }
    if (hasObject)
    {
        return WpdWideNameEqualsPanelName(objectName.c_str(), wantedName);
    }
    return false;
}

static std::wstring WpdForWin32(const std::wstring& path)
{
    if (path.size() < MAX_PATH || path.compare(0, 4, L"\\\\?\\") == 0)
    {
        return path;
    }
    if (path.size() >= 2 && path[0] == L'\\' && path[1] == L'\\')
    {
        return L"\\\\?\\UNC\\" + path.substr(2);
    }
    return L"\\\\?\\" + path;
}

static void WpdAppendComponent(std::wstring& path, const wchar_t* name)
{
    if (!path.empty() && path.back() != L'\\')
    {
        path.push_back(L'\\');
    }
    if (name != nullptr)
    {
        path += name;
    }
}

static std::wstring WpdSafeComponent(const std::wstring& name)
{
    std::wstring safe = name;
    for (size_t i = 0; i < safe.size(); ++i)
    {
        wchar_t ch = safe[i];
        if (ch == L'\\' || ch == L'/' || ch == L':' || ch == L'*' || ch == L'?' ||
            ch == L'"' || ch == L'<' || ch == L'>' || ch == L'|')
        {
            safe[i] = L'_';
        }
    }
    if (safe.empty() || safe == L"." || safe == L"..")
    {
        safe = L"_";
    }
    return safe;
}

static ULONGLONG WpdDiskTreeBytes(const std::wstring& path)
{
    std::wstring native = WpdForWin32(path);
    DWORD attributes = ::GetFileAttributesW(native.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        return 0;
    }
    if ((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
        WIN32_FILE_ATTRIBUTE_DATA info;
        if (!::GetFileAttributesExW(native.c_str(), GetFileExInfoStandard, &info))
        {
            return 0;
        }
        return (static_cast<ULONGLONG>(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
    }

    ULONGLONG total = 0;
    std::wstring mask = native;
    WpdAppendComponent(mask, L"*");
    WIN32_FIND_DATAW data;
    HANDLE find = ::FindFirstFileW(mask.c_str(), &data);
    if (find == INVALID_HANDLE_VALUE)
    {
        return 0;
    }
    do
    {
        if (lstrcmpW(data.cFileName, L".") == 0 || lstrcmpW(data.cFileName, L"..") == 0)
        {
            continue;
        }
        std::wstring child = path;
        WpdAppendComponent(child, data.cFileName);
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            total += WpdDiskTreeBytes(child);
        }
        else
        {
            total += (static_cast<ULONGLONG>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
        }
    } while (::FindNextFileW(find, &data));
    ::FindClose(find);
    return total;
}

static void WpdDeleteDiskTree(const std::wstring& path)
{
    std::wstring native = WpdForWin32(path);
    DWORD attributes = ::GetFileAttributesW(native.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        return;
    }
    if ((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
        ::SetFileAttributesW(native.c_str(), FILE_ATTRIBUTE_NORMAL);
        ::DeleteFileW(native.c_str());
        return;
    }

    std::wstring mask = native;
    WpdAppendComponent(mask, L"*");
    WIN32_FIND_DATAW data;
    HANDLE find = ::FindFirstFileW(mask.c_str(), &data);
    if (find != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (lstrcmpW(data.cFileName, L".") == 0 || lstrcmpW(data.cFileName, L"..") == 0)
            {
                continue;
            }
            std::wstring child = path;
            WpdAppendComponent(child, data.cFileName);
            WpdDeleteDiskTree(child);
        } while (::FindNextFileW(find, &data));
        ::FindClose(find);
    }
    ::SetFileAttributesW(native.c_str(), FILE_ATTRIBUTE_NORMAL);
    ::RemoveDirectoryW(native.c_str());
}

static HRESULT WpdMakeTempDirectory(std::wstring& path)
{
    DWORD length = ::GetTempPathW(0, nullptr);
    if (length == 0)
    {
        return HRESULT_FROM_WIN32(::GetLastError());
    }
    std::wstring root(static_cast<size_t>(length), L'\0');
    DWORD written = ::GetTempPathW(length, &root[0]);
    if (written == 0)
    {
        return HRESULT_FROM_WIN32(::GetLastError());
    }
    root.resize(written);

    for (int attempt = 0; attempt < 100; ++attempt)
    {
        wchar_t suffix[32];
        StringCchPrintfW(suffix, 32, L"wpd%08lX%08lX", ::GetCurrentProcessId(), ::GetTickCount() + attempt);
        path = root;
        WpdAppendComponent(path, suffix);
        if (::CreateDirectoryW(WpdForWin32(path).c_str(), nullptr))
        {
            return S_OK;
        }
        DWORD error = ::GetLastError();
        if (error != ERROR_ALREADY_EXISTS)
        {
            return HRESULT_FROM_WIN32(error);
        }
    }
    return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
}

static HRESULT WpdJoinSalPath(const char* directory, const char* name, std::string& joined)
{
    if (directory == nullptr || name == nullptr)
    {
        return E_INVALIDARG;
    }
    size_t capacity = strlen(directory) + strlen(name) + 8;
    std::vector<char> buffer(capacity);
    lstrcpynA(buffer.data(), directory, static_cast<int>(buffer.size()));
    if (!SalamanderGeneral->SalPathAppend(buffer.data(), name, static_cast<int>(buffer.size())))
    {
        return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
    }
    joined = buffer.data();
    return S_OK;
}

static void WpdRewindStream(IStream* stream)
{
    if (stream == nullptr)
    {
        return;
    }
    LARGE_INTEGER zero = {};
    stream->Seek(zero, STREAM_SEEK_SET, nullptr);
}

static void WpdCancelDataStream(IStream* stream)
{
    if (stream == nullptr)
    {
        return;
    }
    ATL::CComPtr<IPortableDeviceDataStream> dataStream;
    if (SUCCEEDED(stream->QueryInterface(__uuidof(IPortableDeviceDataStream), reinterpret_cast<void**>(&dataStream))))
    {
        dataStream->Cancel();
    }
}

static std::string WpdFormatDetails(ULONGLONG size, bool hasSize, const FILETIME* time, bool hasTime)
{
    std::string text;
    if (hasSize)
    {
        CQuadWord value;
        value.SetUI64(size);
        char number[64];
        SalamanderGeneral->NumberToStr(number, value);
        text = number;
    }
    if (hasTime && time != nullptr && (time->dwLowDateTime != 0 || time->dwHighDateTime != 0))
    {
        FILETIME localTime;
        SYSTEMTIME systemTime;
        if (::FileTimeToLocalFileTime(time, &localTime) && ::FileTimeToSystemTime(&localTime, &systemTime))
        {
            char date[64];
            char clock[64];
            if (::GetDateFormatA(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systemTime, nullptr, date, 64) == 0)
            {
                StringCchPrintfA(date, 64, "%u.%u.%u", systemTime.wDay, systemTime.wMonth, systemTime.wYear);
            }
            if (::GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &systemTime, nullptr, clock, 64) == 0)
            {
                StringCchPrintfA(clock, 64, "%u:%u:%u", systemTime.wHour, systemTime.wMinute, systemTime.wSecond);
            }
            if (!text.empty())
            {
                text += ", ";
            }
            text += date;
            text += ", ";
            text += clock;
        }
    }
    if (text.empty())
    {
        text = WpdText(IDS_OPERATION_DETAILS_UNKNOWN);
    }
    return text;
}

struct WpdOutcome
{
    HRESULT hr;
    bool skipped;

    bool Failed() const
    {
        return FAILED(hr);
    }

    bool Completed() const
    {
        return SUCCEEDED(hr) && !skipped;
    }
};

static WpdOutcome WpdOk()
{
    WpdOutcome outcome = {S_OK, false};
    return outcome;
}

static WpdOutcome WpdSkipped()
{
    WpdOutcome outcome = {S_OK, true};
    return outcome;
}

static WpdOutcome WpdFail(HRESULT hr)
{
    WpdOutcome outcome = {hr, false};
    return outcome;
}

struct WpdConflictState
{
    bool overwriteAll;
    bool skipAll;

    WpdConflictState()
        : overwriteAll(false),
          skipAll(false)
    {
    }
};

struct WpdCopyContext
{
    WpdConflictState conflict;
    bool stop;
    HWND parent;
    UINT operationId;
    CWpdStandardProgress* progress;
    CWpdDevice* sourceDevice;
    CWpdDevice* targetDevice;

    WpdCopyContext()
        : stop(false),
          parent(nullptr),
          operationId(IDS_OPERATIONPROGRESS_COPY),
          progress(nullptr),
          sourceDevice(nullptr),
          targetDevice(nullptr)
    {
    }
};

enum WpdNameChoice
{
    wpdNameFree = 0,
    wpdNameSameObject,
    wpdNameExistingFile,
    wpdNameExistingFolder,
    wpdNameSkip,
    wpdNameCancel
};
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

static PCSTR WINAPI WpdGetUserPartFromFSPath(PCSTR fsName, PCSTR path)
{
    int fsNameLen = lstrlen(fsName);
    if (SalamanderGeneral->StrNICmp(path, fsName, fsNameLen) == 0 && path[fsNameLen] == ':')
    {
        return path + fsNameLen + 1;
    }

    return path;
}

static void WINAPI WpdStripOperationMask(PSTR userPart, int userPartSize)
{
    char* lastSlash = strrchr(userPart, '\\');
    char* lastComponent = lastSlash != nullptr ? lastSlash + 1 : userPart;
    if (strchr(lastComponent, '*') != nullptr || strchr(lastComponent, '?') != nullptr)
    {
        if (lastSlash != nullptr)
        {
            *lastSlash = '\0';
        }
        else
        {
            lstrcpyn(userPart, "\\", userPartSize);
        }
    }
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

static HRESULT WINAPI WpdAddObjectId(IPortableDevicePropVariantCollection* objects, PCWSTR objectId)
{
    _ASSERTE(objects != nullptr);
    _ASSERTE(objectId != nullptr);

    PROPVARIANT value;
    PropVariantInit(&value);
    value.vt = VT_LPWSTR;
    value.pwszVal = const_cast<PWSTR>(objectId);
    return objects->Add(&value);
}

static HRESULT WpdReadResultCollection(IPortableDevicePropVariantCollection* results)
{
    if (results == nullptr)
    {
        return S_OK;
    }

    DWORD count = 0;
    HRESULT hr = results->GetCount(&count);
    if (FAILED(hr))
    {
        return hr;
    }

    for (DWORD i = 0; i < count; ++i)
    {
        PROPVARIANT value;
        PropVariantInit(&value);
        hr = results->GetAt(i, &value);
        if (FAILED(hr))
        {
            PropVariantClear(&value);
            return hr;
        }

        HRESULT itemHr = S_OK;
        if (value.vt == VT_ERROR)
        {
            itemHr = value.scode;
        }
        else if (value.vt == VT_I4)
        {
            itemHr = static_cast<HRESULT>(value.lVal);
        }
        else if (value.vt == VT_UI4)
        {
            itemHr = static_cast<HRESULT>(value.ulVal);
        }
        PropVariantClear(&value);
        if (FAILED(itemHr))
        {
            return itemHr;
        }
    }
    return S_OK;
}

static HRESULT WpdEnsureDeviceOpen(CWpdDevice* device)
{
    if (device == nullptr)
    {
        return E_POINTER;
    }
    if (device->IsOpen())
    {
        return S_OK;
    }
    return device->Open(GENERIC_READ | GENERIC_WRITE);
}

static HRESULT WpdReadStoredNames(CWpdDevice* device, PCWSTR objectId, std::wstring& originalName, bool& hasOriginal,
                                  std::wstring& objectName, bool& hasObject, DWORD& attributes)
{
    hasOriginal = false;
    hasObject = false;
    originalName.clear();
    objectName.clear();
    attributes = 0;

    bool hold = !device->IsOpen();
    HRESULT hr = S_OK;
    if (hold)
    {
        hr = device->Open(GENERIC_READ | GENERIC_WRITE);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    static const PROPERTYKEY* const keys[] =
        {
            &WPD_OBJECT_ORIGINAL_FILE_NAME,
            &WPD_OBJECT_NAME,
            &WPD_OBJECT_CONTENT_TYPE,
        };
    ATL::CComPtr<IPortableDeviceKeyCollection> keyCollection;
    keyCollection.Attach(WpdInitKeys(keys, _countof(keys)));
    ATL::CComPtr<IPortableDeviceValues> values;
    hr = device->GetPropertiesNoAddRef()->GetValues(objectId, keyCollection, &values);
    if (SUCCEEDED(hr))
    {
        PWSTR text = nullptr;
        if (SUCCEEDED(values->GetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, &text)) && text != nullptr && text[0] != L'\0')
        {
            originalName = text;
            hasOriginal = true;
        }
        ::CoTaskMemFree(text);
        text = nullptr;
        if (SUCCEEDED(values->GetStringValue(WPD_OBJECT_NAME, &text)) && text != nullptr && text[0] != L'\0')
        {
            objectName = text;
            hasObject = true;
        }
        ::CoTaskMemFree(text);

        GUID contentType;
        if (SUCCEEDED(values->GetGuidValue(WPD_OBJECT_CONTENT_TYPE, &contentType)) && WpdIsFolderContentType(contentType))
        {
            attributes = FILE_ATTRIBUTE_DIRECTORY;
        }
        if (!hasOriginal && !hasObject)
        {
            hr = E_FAIL;
        }
    }

    if (hold)
    {
        device->Close();
    }
    return hr;
}

static void WpdReadSizeAndTime(CWpdDevice* device, PCWSTR objectId, ULONGLONG& size, bool& hasSize, FILETIME& time, bool& hasTime)
{
    size = 0;
    hasSize = false;
    time.dwLowDateTime = 0;
    time.dwHighDateTime = 0;
    hasTime = false;
    if (device == nullptr || objectId == nullptr || !device->IsOpen())
    {
        return;
    }

    static const PROPERTYKEY* const keys[] =
        {
            &WPD_OBJECT_SIZE,
            &WPD_OBJECT_DATE_MODIFIED,
            &WPD_OBJECT_DATE_CREATED,
        };
    ATL::CComPtr<IPortableDeviceKeyCollection> keyCollection;
    keyCollection.Attach(WpdInitKeys(keys, _countof(keys)));
    ATL::CComPtr<IPortableDeviceValues> values;
    if (FAILED(device->GetPropertiesNoAddRef()->GetValues(objectId, keyCollection, &values)) || values == nullptr)
    {
        return;
    }

    ULONGLONG objectSize = 0;
    if (SUCCEEDED(values->GetUnsignedLargeIntegerValue(WPD_OBJECT_SIZE, &objectSize)))
    {
        size = objectSize;
        hasSize = true;
    }
    if (SUCCEEDED(WpdGetDateTimeValue(values, WPD_OBJECT_DATE_MODIFIED, time)) ||
        SUCCEEDED(WpdGetDateTimeValue(values, WPD_OBJECT_DATE_CREATED, time)))
    {
        hasTime = time.dwLowDateTime != 0 || time.dwHighDateTime != 0;
    }
}

class CWpdBulkGetCallback : public IPortableDevicePropertiesBulkCallback
{
public:
    CWpdBulkGetCallback()
        : m_refs(1),
          m_done(::CreateEventW(nullptr, TRUE, FALSE, nullptr)),
          m_status(E_FAIL)
    {
    }

    ~CWpdBulkGetCallback()
    {
        if (m_done != nullptr)
        {
            ::CloseHandle(m_done);
        }
    }

    HRESULT Wait()
    {
        if (m_done == nullptr)
        {
            return E_OUTOFMEMORY;
        }
        for (;;)
        {
            DWORD wait = ::MsgWaitForMultipleObjects(1, &m_done, FALSE, INFINITE, QS_ALLINPUT);
            if (wait == WAIT_OBJECT_0)
            {
                return m_status;
            }
            if (wait != WAIT_OBJECT_0 + 1)
            {
                return HRESULT_FROM_WIN32(::GetLastError());
            }
            MSG msg;
            while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessageW(&msg);
            }
        }
    }

    const std::vector<ATL::CComPtr<IPortableDeviceValues>>& Results() const
    {
        return m_results;
    }

    STDMETHODIMP QueryInterface(REFIID riid, void** object)
    {
        if (object == nullptr)
        {
            return E_POINTER;
        }
        if (riid == IID_IUnknown || riid == IID_IPortableDevicePropertiesBulkCallback)
        {
            *object = static_cast<IPortableDevicePropertiesBulkCallback*>(this);
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef()
    {
        return static_cast<ULONG>(::InterlockedIncrement(&m_refs));
    }

    STDMETHODIMP_(ULONG) Release()
    {
        LONG refs = ::InterlockedDecrement(&m_refs);
        if (refs == 0)
        {
            delete this;
        }
        return static_cast<ULONG>(refs);
    }

    STDMETHODIMP OnStart(REFGUID)
    {
        return S_OK;
    }

    STDMETHODIMP OnProgress(REFGUID, IPortableDeviceValuesCollection* results)
    {
        if (results == nullptr)
        {
            return S_OK;
        }
        DWORD count = 0;
        if (FAILED(results->GetCount(&count)))
        {
            return S_OK;
        }
        for (DWORD i = 0; i < count; ++i)
        {
            ATL::CComPtr<IPortableDeviceValues> values;
            if (SUCCEEDED(results->GetAt(i, &values)) && values != nullptr)
            {
                m_results.push_back(values);
            }
        }
        return S_OK;
    }

    STDMETHODIMP OnEnd(REFGUID, HRESULT status)
    {
        m_status = status;
        if (m_done != nullptr)
        {
            ::SetEvent(m_done);
        }
        return S_OK;
    }

private:
    LONG m_refs;
    HANDLE m_done;
    HRESULT m_status;
    std::vector<ATL::CComPtr<IPortableDeviceValues>> m_results;
};

static bool WpdValuesMatchChildName(IPortableDeviceValues* values, const char* childName,
                                    std::wstring& objectId, DWORD* attributes)
{
    if (values == nullptr || childName == nullptr)
    {
        return false;
    }

    PWSTR id = nullptr;
    if (FAILED(values->GetStringValue(WPD_OBJECT_ID, &id)) || id == nullptr || id[0] == L'\0')
    {
        ::CoTaskMemFree(id);
        return false;
    }
    objectId = id;
    ::CoTaskMemFree(id);

    std::wstring originalName;
    std::wstring objectName;
    bool hasOriginal = false;
    bool hasObject = false;
    PWSTR text = nullptr;
    if (SUCCEEDED(values->GetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, &text)) && text != nullptr && text[0] != L'\0')
    {
        originalName = text;
        hasOriginal = true;
    }
    ::CoTaskMemFree(text);
    text = nullptr;
    if (SUCCEEDED(values->GetStringValue(WPD_OBJECT_NAME, &text)) && text != nullptr && text[0] != L'\0')
    {
        objectName = text;
        hasObject = true;
    }
    ::CoTaskMemFree(text);

    if (attributes != nullptr)
    {
        *attributes = 0;
        GUID contentType;
        if (SUCCEEDED(values->GetGuidValue(WPD_OBJECT_CONTENT_TYPE, &contentType)) && WpdIsFolderContentType(contentType))
        {
            *attributes = FILE_ATTRIBUTE_DIRECTORY;
        }
    }

    return WpdStoredNamesMatch(originalName, hasOriginal, objectName, hasObject, childName);
}

static HRESULT WpdFindChild(CWpdDevice* device, PCWSTR parentObjectId, const char* childName, PCWSTR ignoreObjectId,
                            PWSTR* childObjectId, DWORD* attributes)
{
    _ASSERTE(childObjectId != nullptr);
    *childObjectId = nullptr;
    if (attributes != nullptr)
    {
        *attributes = 0;
    }
    if (childName == nullptr)
    {
        return E_INVALIDARG;
    }

    bool hold = !device->IsOpen();
    HRESULT hr = S_OK;
    if (hold)
    {
        hr = device->Open(GENERIC_READ | GENERIC_WRITE);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    static const PROPERTYKEY* const keys[] =
        {
            &WPD_OBJECT_ID,
            &WPD_OBJECT_ORIGINAL_FILE_NAME,
            &WPD_OBJECT_NAME,
            &WPD_OBJECT_CONTENT_TYPE,
        };
    ATL::CComPtr<IPortableDeviceKeyCollection> keyCollection;
    keyCollection.Attach(WpdInitKeys(keys, _countof(keys)));

    ATL::CComPtr<IPortableDevicePropertiesBulk> bulk;
    hr = device->GetPropertiesNoAddRef()->QueryInterface(IID_PPV_ARGS(&bulk));
    if (SUCCEEDED(hr) && bulk != nullptr && keyCollection != nullptr)
    {
#ifdef new
#undef new
#define RESTORE_WPD_BULK_DEBUG_NEW
#endif
        CWpdBulkGetCallback* callback = new (std::nothrow) CWpdBulkGetCallback();
#ifdef RESTORE_WPD_BULK_DEBUG_NEW
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef RESTORE_WPD_BULK_DEBUG_NEW
#endif
        if (callback == nullptr)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            GUID context = GUID_NULL;
            hr = bulk->QueueGetValuesByObjectFormat(WPD_OBJECT_FORMAT_ALL, parentObjectId, 1, keyCollection, callback, &context);
            if (SUCCEEDED(hr))
            {
                hr = bulk->Start(context);
            }
            if (SUCCEEDED(hr))
            {
                hr = callback->Wait();
            }
            if (SUCCEEDED(hr))
            {
                const std::vector<ATL::CComPtr<IPortableDeviceValues>>& results = callback->Results();
                for (size_t i = 0; i < results.size(); ++i)
                {
                    std::wstring objectId;
                    DWORD enumAttributes = 0;
                    if (!WpdValuesMatchChildName(results[i], childName, objectId, &enumAttributes))
                    {
                        continue;
                    }
                    if (ignoreObjectId != nullptr && lstrcmpW(objectId.c_str(), ignoreObjectId) == 0)
                    {
                        continue;
                    }
                    size_t bytes = (objectId.size() + 1) * sizeof(wchar_t);
                    PWSTR copy = static_cast<PWSTR>(::CoTaskMemAlloc(bytes));
                    if (copy == nullptr)
                    {
                        callback->Release();
                        if (hold)
                        {
                            device->Close();
                        }
                        return E_OUTOFMEMORY;
                    }
                    memcpy(copy, objectId.c_str(), bytes);
                    *childObjectId = copy;
                    if (attributes != nullptr)
                    {
                        *attributes = enumAttributes;
                    }
                    callback->Release();
                    if (hold)
                    {
                        device->Close();
                    }
                    return S_OK;
                }
                callback->Release();
                if (hold)
                {
                    device->Close();
                }
                return S_FALSE;
            }
            callback->Release();
        }
    }

    // Older drivers without bulk properties still need a correct answer.
    ATL::CComPtr<IEnumPortableDeviceObjectIDs> enumerator;
    hr = device->GetContentNoAddRef()->EnumObjects(0U, parentObjectId, nullptr, &enumerator);
    if (FAILED(hr))
    {
        if (hold)
        {
            device->Close();
        }
        return hr;
    }

    for (;;)
    {
        PWSTR enumObjectId = nullptr;
        ULONG fetched = 0;
        hr = enumerator->Next(1, &enumObjectId, &fetched);
        if (hr != S_OK)
        {
            if (hold)
            {
                device->Close();
            }
            return hr == S_FALSE ? S_FALSE : hr;
        }

        if (ignoreObjectId != nullptr && enumObjectId != nullptr && lstrcmpW(enumObjectId, ignoreObjectId) == 0)
        {
            ::CoTaskMemFree(enumObjectId);
            continue;
        }

        std::wstring originalName;
        std::wstring objectName;
        bool hasOriginal = false;
        bool hasObject = false;
        DWORD enumAttributes = 0;
        hr = WpdReadStoredNames(device, enumObjectId, originalName, hasOriginal, objectName, hasObject, enumAttributes);
        if (SUCCEEDED(hr) && WpdStoredNamesMatch(originalName, hasOriginal, objectName, hasObject, childName))
        {
            *childObjectId = enumObjectId;
            if (attributes != nullptr)
            {
                *attributes = enumAttributes;
            }
            if (hold)
            {
                device->Close();
            }
            return S_OK;
        }

        ::CoTaskMemFree(enumObjectId);
        if (FAILED(hr))
        {
            if (hold)
            {
                device->Close();
            }
            return hr;
        }
    }
}

static HRESULT WpdAskTypeMismatch(HWND parent, const char* name, bool sourceIsDir, WpdConflictState& state, bool& skip)
{
    skip = false;
    std::string message = WpdText(sourceIsDir ? IDS_OPERATION_TYPE_MISMATCH_DIR : IDS_OPERATION_TYPE_MISMATCH_FILE);
    std::string title = WpdText(IDS_OPERATIONPROGRESS_TITLE);
    WpdReportedError = true;
    int answer = SalamanderGeneral->DialogError(WpdDialogParent(parent), BUTTONS_SKIPCANCEL, name != nullptr ? name : "", message.c_str(), title.c_str());
    if (answer == DIALOG_SKIPALL)
    {
        state.skipAll = true;
        skip = true;
        return S_OK;
    }
    if (answer == DIALOG_SKIP)
    {
        skip = true;
        return S_OK;
    }
    return HRESULT_FROM_WIN32(ERROR_CANCELLED);
}

static HRESULT WpdReportNameRemains(HWND parent, const char* name, WpdConflictState& state, bool& skip)
{
    skip = false;
    std::string message = WpdText(IDS_OPERATION_NAME_STILL_EXISTS);
    std::string title = WpdText(IDS_OPERATIONPROGRESS_TITLE);
    WpdReportedError = true;
    int answer = SalamanderGeneral->DialogError(WpdDialogParent(parent), BUTTONS_SKIPCANCEL, name != nullptr ? name : "", message.c_str(), title.c_str());
    if (answer == DIALOG_SKIPALL)
    {
        state.skipAll = true;
        skip = true;
        return S_OK;
    }
    if (answer == DIALOG_SKIP)
    {
        skip = true;
        return S_OK;
    }
    return HRESULT_FROM_WIN32(ERROR_CANCELLED);
}

static HRESULT WpdResolveName(WpdCopyContext& context, CWpdDevice* device, PCWSTR parentObjectId, const char* targetName,
                              const char* sourceName, bool sourceIsDir, ULONGLONG sourceSize, bool sourceHasSize,
                              const FILETIME* sourceTime, bool sourceHasTime, PCWSTR ignoreObjectId, WpdNameChoice& choice, PWSTR* existingObjectId)
{
    choice = wpdNameFree;
    *existingObjectId = nullptr;
    DWORD existingAttributes = 0;
    PWSTR foundId = nullptr;
    HRESULT hr = WpdFindChild(device, parentObjectId, targetName, nullptr, &foundId, &existingAttributes);
    if (hr == S_FALSE)
    {
        return S_OK;
    }
    if (FAILED(hr))
    {
        return hr;
    }

    if (ignoreObjectId != nullptr && foundId != nullptr && lstrcmpW(foundId, ignoreObjectId) == 0)
    {
        choice = wpdNameSameObject;
        *existingObjectId = foundId;
        return S_OK;
    }

    bool destinationIsDir = (existingAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    if (destinationIsDir != sourceIsDir)
    {
        ::CoTaskMemFree(foundId);
        bool skip = false;
        hr = WpdAskTypeMismatch(context.parent, targetName, sourceIsDir, context.conflict, skip);
        if (FAILED(hr))
        {
            choice = wpdNameCancel;
            return hr;
        }
        choice = wpdNameSkip;
        return S_OK;
    }

    if (context.conflict.skipAll)
    {
        ::CoTaskMemFree(foundId);
        choice = wpdNameSkip;
        return S_OK;
    }

    if (!context.conflict.overwriteAll)
    {
        ULONGLONG targetSize = 0;
        bool targetHasSize = false;
        FILETIME targetTime = {};
        bool targetHasTime = false;
        WpdReadSizeAndTime(device, foundId, targetSize, targetHasSize, targetTime, targetHasTime);
        if (destinationIsDir)
        {
            targetHasSize = false;
            sourceHasSize = false;
        }
        std::string targetInfo = WpdFormatDetails(targetSize, targetHasSize, &targetTime, targetHasTime);
        std::string sourceInfo = WpdFormatDetails(sourceSize, sourceHasSize, sourceTime, sourceHasTime);
        int answer = SalamanderGeneral->DialogOverwrite(WpdDialogParent(context.parent), BUTTONS_YESALLSKIPCANCEL,
                                                        targetName != nullptr ? targetName : "", targetInfo.c_str(),
                                                        sourceName != nullptr ? sourceName : "", sourceInfo.c_str());
        if (answer == DIALOG_ALL)
        {
            context.conflict.overwriteAll = true;
        }
        else if (answer == DIALOG_SKIPALL)
        {
            context.conflict.skipAll = true;
            ::CoTaskMemFree(foundId);
            choice = wpdNameSkip;
            return S_OK;
        }
        else if (answer == DIALOG_SKIP)
        {
            ::CoTaskMemFree(foundId);
            choice = wpdNameSkip;
            return S_OK;
        }
        else if (answer != DIALOG_YES)
        {
            ::CoTaskMemFree(foundId);
            choice = wpdNameCancel;
            return HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
    }

    *existingObjectId = foundId;
    choice = destinationIsDir ? wpdNameExistingFolder : wpdNameExistingFile;
    return S_OK;
}

static HRESULT WpdDeleteObject(CWpdDevice* device, PCWSTR objectId)
{
    ATL::CComPtr<IPortableDevicePropVariantCollection> objects;
    HRESULT hr = objects.CoCreateInstance(CLSID_PortableDevicePropVariantCollection);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = WpdAddObjectId(objects, objectId);
    if (FAILED(hr))
    {
        return hr;
    }

    bool hold = !device->IsOpen();
    if (hold)
    {
        hr = device->Open(GENERIC_READ | GENERIC_WRITE);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    ATL::CComPtr<IPortableDevicePropVariantCollection> results;
    hr = device->GetContentNoAddRef()->Delete(PORTABLE_DEVICE_DELETE_WITH_RECURSION, objects, &results);
    HRESULT itemsHr = SUCCEEDED(hr) ? WpdReadResultCollection(results) : S_OK;
    if (hold)
    {
        device->Close();
    }
    if (FAILED(hr))
    {
        return hr;
    }
    return itemsHr;
}

static HRESULT WpdDeleteAndConfirmGone(WpdCopyContext& context, CWpdDevice* device, PCWSTR parentObjectId, PCWSTR objectId, const char* name, bool& gone)
{
    gone = false;
    HRESULT hr = WpdDeleteObject(device, objectId);
    if (WpdRequiresDeviceReconnect(hr))
    {
        return hr;
    }

    PWSTR stillThere = nullptr;
    HRESULT findHr = WpdFindChild(device, parentObjectId, name, nullptr, &stillThere, nullptr);
    if (findHr == S_OK)
    {
        ::CoTaskMemFree(stillThere);
        gone = false;
        return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
    }
    if (FAILED(findHr))
    {
        return findHr;
    }
    gone = true;
    if (FAILED(hr))
    {
        // The driver reported a per-item failure, but the name is gone.
        // Creating the replacement is safe.
        return S_OK;
    }
    UNREFERENCED_PARAMETER(context);
    return S_OK;
}

static HRESULT WpdCreateFolderChecked(CWpdDevice* device, PCWSTR parentObjectId, const wchar_t* wideName, const char* panelName)
{
    PWSTR existing = nullptr;
    HRESULT findHr = WpdFindChild(device, parentObjectId, panelName, nullptr, &existing, nullptr);
    bool present = findHr == S_OK;
    ::CoTaskMemFree(existing);
    if (FAILED(findHr))
    {
        return findHr;
    }
    if (!WpdMayCreateWhenNamePresent(present))
    {
        return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
    }

    bool hold = !device->IsOpen();
    HRESULT hr = S_OK;
    if (hold)
    {
        hr = device->Open(GENERIC_READ | GENERIC_WRITE);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    ATL::CComPtr<IPortableDeviceValues> values;
    hr = values.CoCreateInstance(CLSID_PortableDeviceValues);
    if (SUCCEEDED(hr))
    {
        hr = values->SetStringValue(WPD_OBJECT_PARENT_ID, parentObjectId);
    }
    if (SUCCEEDED(hr))
    {
        hr = values->SetStringValue(WPD_OBJECT_NAME, wideName);
    }
    if (SUCCEEDED(hr))
    {
        hr = values->SetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, wideName);
    }
    if (SUCCEEDED(hr))
    {
        hr = values->SetGuidValue(WPD_OBJECT_CONTENT_TYPE, WPD_CONTENT_TYPE_FOLDER);
    }
    PWSTR newObjectId = nullptr;
    if (SUCCEEDED(hr))
    {
        hr = device->GetContentNoAddRef()->CreateObjectWithPropertiesOnly(values, &newObjectId);
    }
    ::CoTaskMemFree(newObjectId);
    if (hold)
    {
        device->Close();
    }
    return hr;
}

static HRESULT WpdCreateFileWithData(CWpdDevice* device, PCWSTR parentObjectId, const wchar_t* wideName, const char* panelName,
                                     IStream* source, ULONGLONG size, const GUID* contentType, const GUID* format)
{
    WpdRewindStream(source);
    PWSTR existing = nullptr;
    HRESULT findHr = WpdFindChild(device, parentObjectId, panelName, nullptr, &existing, nullptr);
    bool present = findHr == S_OK;
    ::CoTaskMemFree(existing);
    if (FAILED(findHr))
    {
        return findHr;
    }
    if (!WpdMayCreateWhenNamePresent(present))
    {
        return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
    }

    bool hold = !device->IsOpen();
    HRESULT hr = S_OK;
    if (hold)
    {
        hr = device->Open(GENERIC_READ | GENERIC_WRITE);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    ATL::CComPtr<IPortableDeviceValues> values;
    hr = values.CoCreateInstance(CLSID_PortableDeviceValues);
    if (SUCCEEDED(hr))
    {
        hr = values->SetStringValue(WPD_OBJECT_PARENT_ID, parentObjectId);
    }
    if (SUCCEEDED(hr))
    {
        hr = values->SetStringValue(WPD_OBJECT_NAME, wideName);
    }
    if (SUCCEEDED(hr))
    {
        hr = values->SetStringValue(WPD_OBJECT_ORIGINAL_FILE_NAME, wideName);
    }
    if (SUCCEEDED(hr))
    {
        hr = values->SetGuidValue(WPD_OBJECT_CONTENT_TYPE, contentType != nullptr ? *contentType : WPD_CONTENT_TYPE_GENERIC_FILE);
    }
    if (SUCCEEDED(hr))
    {
        hr = values->SetGuidValue(WPD_OBJECT_FORMAT, format != nullptr ? *format : WPD_OBJECT_FORMAT_UNSPECIFIED);
    }
    if (SUCCEEDED(hr) && size != static_cast<ULONGLONG>(-1))
    {
        hr = values->SetUnsignedLargeIntegerValue(WPD_OBJECT_SIZE, size);
    }

    ATL::CComPtr<IStream> target;
    DWORD optimalBufferSize = 0;
    PWSTR newObjectId = nullptr;
    if (SUCCEEDED(hr))
    {
        hr = device->GetContentNoAddRef()->CreateObjectWithPropertiesAndData(values, &target, &optimalBufferSize, &newObjectId);
        if (SUCCEEDED(hr))
        {
            HRESULT copyHr = WpdCopyStream(source, target, size);
            if (SUCCEEDED(copyHr))
            {
                copyHr = target->Commit(STGC_DEFAULT);
            }
            if (FAILED(copyHr))
            {
                // An unfinished create must be cancelled before the stream is
                // released, or Android MTP keeps the session busy.
                WpdCancelDataStream(target);
                target.Release();
            }
            hr = copyHr;
        }
    }
    ::CoTaskMemFree(newObjectId);
    if (hold)
    {
        device->Close();
    }
    return hr;
}

static HRESULT WpdTryWriteExisting(CWpdDevice* device, PCWSTR objectId, IStream* source, ULONGLONG size)
{
    WpdRewindStream(source);
    bool hold = !device->IsOpen();
    HRESULT hr = S_OK;
    if (hold)
    {
        hr = device->Open(GENERIC_READ | GENERIC_WRITE);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    ATL::CComPtr<IPortableDeviceResources> resources;
    hr = device->GetContentNoAddRef()->Transfer(&resources);
    ATL::CComPtr<IStream> target;
    DWORD optimalBufferSize = 0;
    if (SUCCEEDED(hr))
    {
        hr = resources->GetStream(objectId, WPD_RESOURCE_DEFAULT, STGM_WRITE, &optimalBufferSize, &target);
    }
    if (SUCCEEDED(hr))
    {
        if (size != static_cast<ULONGLONG>(-1))
        {
            ULARGE_INTEGER newSize;
            newSize.QuadPart = size;
            target->SetSize(newSize);
        }
        LARGE_INTEGER zero = {};
        target->Seek(zero, STREAM_SEEK_SET, nullptr);
        hr = WpdCopyStream(source, target, size);
        if (SUCCEEDED(hr))
        {
            hr = target->Commit(STGC_DEFAULT);
        }
        if (FAILED(hr))
        {
            WpdCancelDataStream(target);
            target.Release();
        }
    }
    if (hold)
    {
        device->Close();
    }
    return hr;
}

static HRESULT WpdPutFileStream(WpdCopyContext& context, CWpdDevice* device, PCWSTR parentObjectId, const wchar_t* wideName,
                                const char* panelName, const char* sourceDisplay, IStream* source, ULONGLONG size, bool hasSize,
                                const FILETIME* time, bool hasTime, const GUID* contentType, const GUID* format, PCWSTR ignoreObjectId)
{
    if (context.stop)
    {
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }
    if (context.progress != nullptr && !context.progress->BeginItem(sourceDisplay != nullptr ? sourceDisplay : panelName, hasSize ? size : static_cast<ULONGLONG>(-1)))
    {
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }

    WpdNameChoice choice = wpdNameFree;
    PWSTR existingId = nullptr;
    HRESULT hr = WpdResolveName(context, device, parentObjectId, panelName, sourceDisplay, false, size, hasSize, time, hasTime, ignoreObjectId, choice, &existingId);
    if (choice == wpdNameSkip)
    {
        ::CoTaskMemFree(existingId);
        return S_FALSE;
    }
    if (FAILED(hr) || choice == wpdNameCancel)
    {
        ::CoTaskMemFree(existingId);
        return FAILED(hr) ? hr : HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }

    if (choice == wpdNameExistingFolder)
    {
        ::CoTaskMemFree(existingId);
        bool skip = false;
        HRESULT askHr = WpdAskTypeMismatch(context.parent, panelName, false, context.conflict, skip);
        if (SUCCEEDED(askHr) && skip)
        {
            return S_FALSE;
        }
        return FAILED(askHr) ? askHr : HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }

    if (choice == wpdNameExistingFile || choice == wpdNameSameObject)
    {
        hr = WpdTryWriteExisting(device, existingId, source, size);
        if (SUCCEEDED(hr))
        {
            ::CoTaskMemFree(existingId);
            if (context.progress != nullptr)
            {
                context.progress->EndItem(hasSize ? size : 0);
            }
            return S_OK;
        }
        if (WpdRequiresDeviceReconnect(hr) || WpdIsOperationCancelled(hr))
        {
            ::CoTaskMemFree(existingId);
            return hr;
        }

        bool gone = false;
        HRESULT deleteHr = WpdDeleteAndConfirmGone(context, device, parentObjectId, existingId, panelName, gone);
        ::CoTaskMemFree(existingId);
        if (FAILED(deleteHr) || !gone)
        {
            bool skip = false;
            HRESULT askHr = WpdReportNameRemains(context.parent, panelName, context.conflict, skip);
            if (SUCCEEDED(askHr) && skip)
            {
                return S_FALSE;
            }
            return FAILED(askHr) ? askHr : HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
    }
    else
    {
        ::CoTaskMemFree(existingId);
    }

    hr = WpdCreateFileWithData(device, parentObjectId, wideName, panelName, source, size, contentType, format);
    if (hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
    {
        bool skip = false;
        HRESULT askHr = WpdReportNameRemains(context.parent, panelName, context.conflict, skip);
        if (SUCCEEDED(askHr) && skip)
        {
            return S_FALSE;
        }
        return FAILED(askHr) ? askHr : HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }
    if (SUCCEEDED(hr) && context.progress != nullptr)
    {
        context.progress->EndItem(hasSize ? size : 0);
    }
    return hr;
}

static WpdOutcome WpdUploadDisk(WpdCopyContext& context, PCWSTR parentObjectId, const std::wstring& sourcePath, const wchar_t* targetWide,
                                const char* panelName, const char* sourceDisplay, ULONGLONG sourceSize, bool sourceHasSize,
                                const FILETIME* sourceTime, bool sourceHasTime);

static WpdOutcome WpdUploadChildren(WpdCopyContext& context, PCWSTR parentObjectId, const std::wstring& sourceDirectory)
{
    std::wstring mask = WpdForWin32(sourceDirectory);
    WpdAppendComponent(mask, L"*");
    WIN32_FIND_DATAW data;
    HANDLE find = ::FindFirstFileW(mask.c_str(), &data);
    if (find == INVALID_HANDLE_VALUE)
    {
        DWORD error = ::GetLastError();
        if (error == ERROR_FILE_NOT_FOUND)
        {
            return WpdOk();
        }
        return WpdFail(HRESULT_FROM_WIN32(error));
    }

    WpdOutcome outcome = WpdOk();
    bool anySkip = false;
    do
    {
        if (lstrcmpW(data.cFileName, L".") == 0 || lstrcmpW(data.cFileName, L"..") == 0)
        {
            continue;
        }
        if (context.stop)
        {
            outcome = WpdFail(HRESULT_FROM_WIN32(ERROR_CANCELLED));
            break;
        }

        std::wstring childPath = sourceDirectory;
        WpdAppendComponent(childPath, data.cFileName);
        std::string childPanel;
        if (!WpdWideToUtf8(data.cFileName, childPanel))
        {
            outcome = WpdFail(E_FAIL);
            break;
        }
        ULONGLONG childSize = (static_cast<ULONGLONG>(data.nFileSizeHigh) << 32) | data.nFileSizeLow;
        bool childIsDir = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        outcome = WpdUploadDisk(context, parentObjectId, childPath, data.cFileName, childPanel.c_str(), childPanel.c_str(),
                                childSize, !childIsDir, &data.ftLastWriteTime, true);
        if (outcome.Failed())
        {
            break;
        }
        if (outcome.skipped)
        {
            anySkip = true;
        }
    } while (::FindNextFileW(find, &data));
    ::FindClose(find);
    if (outcome.Failed())
    {
        return outcome;
    }
    return anySkip ? WpdSkipped() : WpdOk();
}

static WpdOutcome WpdUploadDisk(WpdCopyContext& context, PCWSTR parentObjectId, const std::wstring& sourcePath, const wchar_t* targetWide,
                                const char* panelName, const char* sourceDisplay, ULONGLONG sourceSize, bool sourceHasSize,
                                const FILETIME* sourceTime, bool sourceHasTime)
{
    if (context.stop)
    {
        return WpdFail(HRESULT_FROM_WIN32(ERROR_CANCELLED));
    }

    std::wstring native = WpdForWin32(sourcePath);
    DWORD attributes = ::GetFileAttributesW(native.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        return WpdFail(HRESULT_FROM_WIN32(::GetLastError()));
    }
    bool sourceIsDir = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    if (!sourceIsDir)
    {
        ATL::CComPtr<IStream> source;
        HRESULT fileHr = ::SHCreateStreamOnFileEx(native.c_str(), STGM_READ | STGM_SHARE_DENY_NONE, FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, &source);
        if (FAILED(fileHr))
        {
            return WpdFail(fileHr);
        }
        fileHr = WpdPutFileStream(context, context.targetDevice, parentObjectId, targetWide, panelName, sourceDisplay, source,
                                  sourceHasSize ? sourceSize : static_cast<ULONGLONG>(-1), sourceHasSize, sourceTime, sourceHasTime, nullptr, nullptr, nullptr);
        if (fileHr == S_FALSE)
        {
            return WpdSkipped();
        }
        return FAILED(fileHr) ? WpdFail(fileHr) : WpdOk();
    }

    WpdNameChoice choice = wpdNameFree;
    PWSTR existingId = nullptr;
    HRESULT hr = WpdResolveName(context, context.targetDevice, parentObjectId, panelName, sourceDisplay, sourceIsDir,
                                sourceSize, sourceHasSize && !sourceIsDir, sourceTime, sourceHasTime, nullptr, choice, &existingId);
    if (FAILED(hr))
    {
        ::CoTaskMemFree(existingId);
        return WpdFail(hr);
    }
    if (choice == wpdNameSkip || choice == wpdNameCancel)
    {
        ::CoTaskMemFree(existingId);
        return choice == wpdNameSkip ? WpdSkipped() : WpdFail(HRESULT_FROM_WIN32(ERROR_CANCELLED));
    }

    PWSTR folderId = nullptr;
    bool createdFolder = false;
    if (choice == wpdNameExistingFolder)
    {
        folderId = existingId;
        existingId = nullptr;
    }
    else
    {
        ::CoTaskMemFree(existingId);
        existingId = nullptr;
        hr = WpdCreateFolderChecked(context.targetDevice, parentObjectId, targetWide, panelName);
        if (hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
        {
            bool skip = false;
            HRESULT askHr = WpdReportNameRemains(context.parent, panelName, context.conflict, skip);
            return FAILED(askHr) ? WpdFail(askHr) : WpdSkipped();
        }
        if (FAILED(hr))
        {
            return WpdFail(hr);
        }
        createdFolder = true;
        hr = WpdFindChild(context.targetDevice, parentObjectId, panelName, nullptr, &folderId, nullptr);
        if (hr != S_OK)
        {
            return WpdFail(FAILED(hr) ? hr : HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
        }
    }
    ::CoTaskMemFree(existingId);

    if (context.progress != nullptr &&
        !context.progress->BeginItem(sourceDisplay != nullptr ? sourceDisplay : panelName, static_cast<ULONGLONG>(-1)))
    {
        ::CoTaskMemFree(folderId);
        if (createdFolder)
        {
            PWSTR createdId = nullptr;
            if (WpdFindChild(context.targetDevice, parentObjectId, panelName, nullptr, &createdId, nullptr) == S_OK)
            {
                WpdDeleteObject(context.targetDevice, createdId);
                ::CoTaskMemFree(createdId);
            }
        }
        return WpdFail(HRESULT_FROM_WIN32(ERROR_CANCELLED));
    }
    WpdOutcome children = WpdUploadChildren(context, folderId, sourcePath);
    ::CoTaskMemFree(folderId);
    if (children.Failed() && createdFolder)
    {
        PWSTR createdId = nullptr;
        if (WpdFindChild(context.targetDevice, parentObjectId, panelName, nullptr, &createdId, nullptr) == S_OK)
        {
            WpdDeleteObject(context.targetDevice, createdId);
            ::CoTaskMemFree(createdId);
        }
    }
    return children;
}

static HRESULT WpdOpenReadStream(CWpdDevice* device, PCWSTR objectId, ATL::CComPtr<IStream>& stream)
{
    bool hold = !device->IsOpen();
    HRESULT hr = S_OK;
    if (hold)
    {
        hr = device->Open(GENERIC_READ | GENERIC_WRITE);
        if (FAILED(hr))
        {
            return hr;
        }
    }
    ATL::CComPtr<IPortableDeviceResources> resources;
    hr = device->GetContentNoAddRef()->Transfer(&resources);
    DWORD optimalBufferSize = 0;
    if (SUCCEEDED(hr))
    {
        hr = resources->GetStream(objectId, WPD_RESOURCE_DEFAULT, STGM_READ, &optimalBufferSize, &stream);
    }
    if (hold)
    {
        device->Close();
    }
    return hr;
}

static WpdOutcome WpdCopyDeviceObject(WpdCopyContext& context, CWpdDevice* sourceDevice, PCWSTR sourceObjectId, PCWSTR targetParentId, const char* panelName);

static HRESULT WpdEnumChildIds(CWpdDevice* device, PCWSTR parentObjectId, std::vector<std::wstring>& children)
{
    children.clear();
    bool hold = !device->IsOpen();
    HRESULT hr = S_OK;
    if (hold)
    {
        hr = device->Open(GENERIC_READ | GENERIC_WRITE);
        if (FAILED(hr))
        {
            return hr;
        }
    }
    ATL::CComPtr<IEnumPortableDeviceObjectIDs> enumerator;
    hr = device->GetContentNoAddRef()->EnumObjects(0U, parentObjectId, nullptr, &enumerator);
    if (SUCCEEDED(hr))
    {
        for (;;)
        {
            PWSTR childId = nullptr;
            ULONG fetched = 0;
            hr = enumerator->Next(1, &childId, &fetched);
            if (hr != S_OK)
            {
                hr = hr == S_FALSE ? S_OK : hr;
                break;
            }
            if (childId != nullptr)
            {
                children.push_back(childId);
                ::CoTaskMemFree(childId);
            }
        }
    }
    if (hold)
    {
        device->Close();
    }
    return hr;
}

static WpdOutcome WpdCopyDeviceObject(WpdCopyContext& context, CWpdDevice* sourceDevice, PCWSTR sourceObjectId, PCWSTR targetParentId, const char* panelName)
{
    if (context.stop)
    {
        return WpdFail(HRESULT_FROM_WIN32(ERROR_CANCELLED));
    }

    std::wstring originalName;
    std::wstring objectName;
    bool hasOriginal = false;
    bool hasObject = false;
    DWORD attributes = 0;
    HRESULT hr = WpdReadStoredNames(sourceDevice, sourceObjectId, originalName, hasOriginal, objectName, hasObject, attributes);
    if (FAILED(hr))
    {
        return WpdFail(hr);
    }
    const std::wstring& displayName = hasOriginal ? originalName : objectName;
    std::string displayUtf8;
    if (!WpdWideToUtf8(displayName.c_str(), displayUtf8))
    {
        return WpdFail(E_FAIL);
    }
    const char* name = panelName != nullptr && panelName[0] != '\0' ? panelName : displayUtf8.c_str();
    bool sourceIsDir = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    ULONGLONG size = 0;
    bool hasSize = false;
    FILETIME time = {};
    bool hasTime = false;
    WpdReadSizeAndTime(sourceDevice, sourceObjectId, size, hasSize, time, hasTime);

    if (!sourceIsDir)
    {
        ATL::CComPtr<IStream> source;
        hr = WpdOpenReadStream(sourceDevice, sourceObjectId, source);
        if (FAILED(hr))
        {
            return WpdFail(hr);
        }
        hr = WpdPutFileStream(context, context.targetDevice, targetParentId, displayName.c_str(), name, name, source, size, hasSize, &time, hasTime, nullptr, nullptr, sourceObjectId);
        if (hr == S_FALSE)
        {
            return WpdSkipped();
        }
        return FAILED(hr) ? WpdFail(hr) : WpdOk();
    }

    WpdNameChoice choice = wpdNameFree;
    PWSTR existingId = nullptr;
    hr = WpdResolveName(context, context.targetDevice, targetParentId, name, name, true, size, false, &time, hasTime, sourceObjectId, choice, &existingId);
    if (FAILED(hr))
    {
        ::CoTaskMemFree(existingId);
        return WpdFail(hr);
    }
    if (choice == wpdNameSkip || choice == wpdNameCancel)
    {
        ::CoTaskMemFree(existingId);
        return choice == wpdNameSkip ? WpdSkipped() : WpdFail(HRESULT_FROM_WIN32(ERROR_CANCELLED));
    }

    PWSTR folderId = nullptr;
    if (choice == wpdNameExistingFolder)
    {
        folderId = existingId;
    }
    else
    {
        ::CoTaskMemFree(existingId);
        hr = WpdCreateFolderChecked(context.targetDevice, targetParentId, displayName.c_str(), name);
        if (hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
        {
            bool skip = false;
            HRESULT askHr = WpdReportNameRemains(context.parent, name, context.conflict, skip);
            return FAILED(askHr) ? WpdFail(askHr) : WpdSkipped();
        }
        if (FAILED(hr))
        {
            return WpdFail(hr);
        }
        hr = WpdFindChild(context.targetDevice, targetParentId, name, nullptr, &folderId, nullptr);
        if (hr != S_OK)
        {
            return WpdFail(FAILED(hr) ? hr : HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
        }
    }

    if (context.progress != nullptr && !context.progress->BeginItem(name, static_cast<ULONGLONG>(-1)))
    {
        ::CoTaskMemFree(folderId);
        return WpdFail(HRESULT_FROM_WIN32(ERROR_CANCELLED));
    }
    std::vector<std::wstring> children;
    hr = WpdEnumChildIds(sourceDevice, sourceObjectId, children);
    if (FAILED(hr))
    {
        ::CoTaskMemFree(folderId);
        return WpdFail(hr);
    }
    bool anySkip = false;
    for (size_t i = 0; i < children.size(); ++i)
    {
        WpdOutcome child = WpdCopyDeviceObject(context, sourceDevice, children[i].c_str(), folderId, nullptr);
        if (child.Failed())
        {
            ::CoTaskMemFree(folderId);
            return child;
        }
        if (child.skipped)
        {
            anySkip = true;
        }
    }
    ::CoTaskMemFree(folderId);
    return anySkip ? WpdSkipped() : WpdOk();
}

static HRESULT WpdDownloadObjectWide(WpdCopyContext& context, CWpdDevice* device, PCWSTR objectId, const std::wstring& targetPath)
{
    std::wstring originalName;
    std::wstring objectName;
    bool hasOriginal = false;
    bool hasObject = false;
    DWORD attributes = 0;
    HRESULT hr = WpdReadStoredNames(device, objectId, originalName, hasOriginal, objectName, hasObject, attributes);
    if (FAILED(hr))
    {
        return hr;
    }

    std::wstring native = WpdForWin32(targetPath);
    if ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
    {
        if (!::CreateDirectoryW(native.c_str(), nullptr) && ::GetLastError() != ERROR_ALREADY_EXISTS)
        {
            return HRESULT_FROM_WIN32(::GetLastError());
        }
        std::vector<std::wstring> children;
        hr = WpdEnumChildIds(device, objectId, children);
        if (FAILED(hr))
        {
            return hr;
        }
        for (size_t i = 0; i < children.size(); ++i)
        {
            std::wstring childOriginal;
            std::wstring childObject;
            bool childHasOriginal = false;
            bool childHasObject = false;
            DWORD childAttributes = 0;
            hr = WpdReadStoredNames(device, children[i].c_str(), childOriginal, childHasOriginal, childObject, childHasObject, childAttributes);
            if (FAILED(hr))
            {
                return hr;
            }
            const std::wstring& childName = childHasOriginal ? childOriginal : childObject;
            std::wstring childTarget = targetPath;
            WpdAppendComponent(childTarget, WpdSafeComponent(childName).c_str());
            hr = WpdDownloadObjectWide(context, device, children[i].c_str(), childTarget);
            if (FAILED(hr))
            {
                return hr;
            }
        }
        return S_OK;
    }

    if (context.progress != nullptr)
    {
        std::string utf8;
        WpdWideToUtf8((hasOriginal ? originalName : objectName).c_str(), utf8);
        ULONGLONG size = 0;
        bool hasSize = false;
        FILETIME time = {};
        bool hasTime = false;
        WpdReadSizeAndTime(device, objectId, size, hasSize, time, hasTime);
        if (!context.progress->BeginItem(utf8.c_str(), hasSize ? size : static_cast<ULONGLONG>(-1)))
        {
            return HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
    }

    ATL::CComPtr<IStream> source;
    hr = WpdOpenReadStream(device, objectId, source);
    if (FAILED(hr))
    {
        return hr;
    }
    ATL::CComPtr<IStream> target;
    hr = ::SHCreateStreamOnFileEx(native.c_str(), STGM_CREATE | STGM_WRITE | STGM_SHARE_EXCLUSIVE, FILE_ATTRIBUTE_NORMAL, TRUE, nullptr, &target);
    if (FAILED(hr))
    {
        return hr;
    }
    ULONGLONG size = 0;
    bool hasSize = false;
    FILETIME time = {};
    bool hasTime = false;
    WpdReadSizeAndTime(device, objectId, size, hasSize, time, hasTime);
    hr = WpdCopyStream(source, target, hasSize ? size : static_cast<ULONGLONG>(-1));
    if (SUCCEEDED(hr))
    {
        hr = target->Commit(STGC_DEFAULT);
    }
    if (SUCCEEDED(hr) && context.progress != nullptr)
    {
        context.progress->EndItem(hasSize ? size : 0);
    }
    return hr;
}

static HRESULT WpdSetStringProperty(CWpdDevice* device, PCWSTR objectId, REFPROPERTYKEY key, const wchar_t* value, HRESULT& propertyHr)
{
    propertyHr = S_OK;
    ATL::CComPtr<IPortableDeviceValues> values;
    HRESULT hr = values.CoCreateInstance(CLSID_PortableDeviceValues);
    if (FAILED(hr))
    {
        propertyHr = hr;
        return hr;
    }
    hr = values->SetStringValue(key, value);
    if (FAILED(hr))
    {
        propertyHr = hr;
        return hr;
    }
    ATL::CComPtr<IPortableDeviceValues> results;
    hr = device->GetPropertiesNoAddRef()->SetValues(objectId, values, &results);
    propertyHr = hr;
    if (results != nullptr)
    {
        HRESULT perProperty = hr;
        if (SUCCEEDED(results->GetErrorValue(key, &perProperty)))
        {
            propertyHr = perProperty;
        }
    }
    return FAILED(propertyHr) ? propertyHr : hr;
}

static HRESULT WpdReadParentId(CWpdDevice* device, PCWSTR objectId, std::wstring& parentId)
{
    static const PROPERTYKEY* const keys[] = {&WPD_OBJECT_PARENT_ID};
    ATL::CComPtr<IPortableDeviceKeyCollection> keyCollection;
    keyCollection.Attach(WpdInitKeys(keys, _countof(keys)));
    ATL::CComPtr<IPortableDeviceValues> values;
    HRESULT hr = device->GetPropertiesNoAddRef()->GetValues(objectId, keyCollection, &values);
    if (FAILED(hr))
    {
        return hr;
    }
    PWSTR text = nullptr;
    hr = values->GetStringValue(WPD_OBJECT_PARENT_ID, &text);
    if (FAILED(hr) || text == nullptr)
    {
        ::CoTaskMemFree(text);
        return FAILED(hr) ? hr : E_FAIL;
    }
    parentId = text;
    ::CoTaskMemFree(text);
    return S_OK;
}

static HRESULT WpdMoveChildren(CWpdDevice* device, PCWSTR sourceFolderId, PCWSTR destinationFolderId, bool& unsupported)
{
    unsupported = false;
    std::vector<std::wstring> children;
    HRESULT hr = WpdEnumChildIds(device, sourceFolderId, children);
    if (FAILED(hr))
    {
        return hr;
    }
    if (children.empty())
    {
        return S_OK;
    }

    ATL::CComPtr<IPortableDevicePropVariantCollection> objects;
    hr = objects.CoCreateInstance(CLSID_PortableDevicePropVariantCollection);
    if (FAILED(hr))
    {
        return hr;
    }
    for (size_t i = 0; i < children.size(); ++i)
    {
        hr = WpdAddObjectId(objects, children[i].c_str());
        if (FAILED(hr))
        {
            return hr;
        }
    }
    ATL::CComPtr<IPortableDevicePropVariantCollection> results;
    hr = device->GetContentNoAddRef()->Move(objects, destinationFolderId, &results);
    if (WpdIsMoveUnsupported(hr))
    {
        unsupported = true;
        return hr;
    }
    if (FAILED(hr))
    {
        return hr;
    }
    return WpdReadResultCollection(results);
}

static HRESULT WpdRenameFolderFallback(WpdCopyContext& context, CWpdDevice* device, PCWSTR objectId, const wchar_t* wideName, const char* newName)
{
    std::wstring parentId;
    HRESULT hr = WpdReadParentId(device, objectId, parentId);
    if (FAILED(hr))
    {
        return hr;
    }

    WpdNameChoice choice = wpdNameFree;
    PWSTR existingId = nullptr;
    ULONGLONG size = 0;
    bool hasSize = false;
    FILETIME time = {};
    bool hasTime = false;
    WpdReadSizeAndTime(device, objectId, size, hasSize, time, hasTime);
    hr = WpdResolveName(context, device, parentId.c_str(), newName, newName, true, size, false, &time, hasTime, objectId, choice, &existingId);
    if (FAILED(hr))
    {
        ::CoTaskMemFree(existingId);
        return hr;
    }
    if (choice == wpdNameSkip || choice == wpdNameCancel)
    {
        ::CoTaskMemFree(existingId);
        return choice == wpdNameSkip ? S_FALSE : HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }
    if (choice == wpdNameExistingFile)
    {
        ::CoTaskMemFree(existingId);
        bool skip = false;
        HRESULT askHr = WpdAskTypeMismatch(context.parent, newName, true, context.conflict, skip);
        if (SUCCEEDED(askHr) && skip)
        {
            return S_FALSE;
        }
        return FAILED(askHr) ? askHr : HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }

    PWSTR destinationId = nullptr;
    bool createdDestination = false;
    if (choice == wpdNameExistingFolder)
    {
        destinationId = existingId;
    }
    else
    {
        ::CoTaskMemFree(existingId);
        if (choice == wpdNameSameObject)
        {
            // The case-insensitive match is this folder. Create a temporary
            // sibling, move the children, remove the old folder, then create
            // the requested name once that name is free.
            std::wstring temporary = std::wstring(wideName) + L"~wpd";
            std::string temporaryPanel;
            if (!WpdWideToUtf8(temporary.c_str(), temporaryPanel))
            {
                return E_FAIL;
            }
            hr = WpdCreateFolderChecked(device, parentId.c_str(), temporary.c_str(), temporaryPanel.c_str());
            if (FAILED(hr))
            {
                return hr;
            }
            hr = WpdFindChild(device, parentId.c_str(), temporaryPanel.c_str(), nullptr, &destinationId, nullptr);
            if (hr != S_OK)
            {
                return FAILED(hr) ? hr : HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            }
            createdDestination = true;
        }
        else
        {
            hr = WpdCreateFolderChecked(device, parentId.c_str(), wideName, newName);
            if (hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
            {
                bool skip = false;
                HRESULT askHr = WpdReportNameRemains(context.parent, newName, context.conflict, skip);
                if (SUCCEEDED(askHr) && skip)
                {
                    return S_FALSE;
                }
                return FAILED(askHr) ? askHr : HRESULT_FROM_WIN32(ERROR_CANCELLED);
            }
            if (FAILED(hr))
            {
                return hr;
            }
            hr = WpdFindChild(device, parentId.c_str(), newName, nullptr, &destinationId, nullptr);
            if (hr != S_OK)
            {
                return FAILED(hr) ? hr : HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            }
            createdDestination = true;
        }
    }

    bool unsupported = false;
    hr = WpdMoveChildren(device, objectId, destinationId, unsupported);
    if (unsupported)
    {
        std::wstring tempDirectory;
        HRESULT downloadHr = WpdMakeTempDirectory(tempDirectory);
        if (SUCCEEDED(downloadHr))
        {
            downloadHr = WpdDownloadObjectWide(context, device, objectId, tempDirectory);
        }
        if (SUCCEEDED(downloadHr))
        {
            WpdOutcome uploaded = WpdUploadChildren(context, destinationId, tempDirectory);
            if (uploaded.Failed())
            {
                downloadHr = uploaded.hr;
            }
            else if (uploaded.skipped)
            {
                downloadHr = S_FALSE;
            }
            else
            {
                downloadHr = S_OK;
            }
        }
        WpdDeleteDiskTree(tempDirectory);
        hr = downloadHr;
    }
    if (hr == S_FALSE)
    {
        ::CoTaskMemFree(destinationId);
        return S_FALSE;
    }
    if (FAILED(hr))
    {
        if (createdDestination && destinationId != nullptr)
        {
            WpdDeleteObject(device, destinationId);
        }
        ::CoTaskMemFree(destinationId);
        return hr;
    }

    hr = WpdDeleteObject(device, objectId);
    if (choice == wpdNameSameObject && SUCCEEDED(hr))
    {
        HRESULT createHr = WpdCreateFolderChecked(device, parentId.c_str(), wideName, newName);
        if (createHr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
        {
            ::CoTaskMemFree(destinationId);
            bool skip = false;
            HRESULT askHr = WpdReportNameRemains(context.parent, newName, context.conflict, skip);
            if (SUCCEEDED(askHr) && skip)
            {
                return S_FALSE;
            }
            return FAILED(askHr) ? askHr : HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        }
        if (FAILED(createHr))
        {
            ::CoTaskMemFree(destinationId);
            return createHr;
        }
        PWSTR finalId = nullptr;
        createHr = WpdFindChild(device, parentId.c_str(), newName, nullptr, &finalId, nullptr);
        if (createHr == S_OK)
        {
            bool secondUnsupported = false;
            HRESULT moveHr = WpdMoveChildren(device, destinationId, finalId, secondUnsupported);
            if (secondUnsupported)
            {
                std::wstring tempDirectory;
                moveHr = WpdMakeTempDirectory(tempDirectory);
                if (SUCCEEDED(moveHr))
                {
                    moveHr = WpdDownloadObjectWide(context, device, destinationId, tempDirectory);
                }
                if (SUCCEEDED(moveHr))
                {
                    WpdOutcome uploaded = WpdUploadChildren(context, finalId, tempDirectory);
                    moveHr = uploaded.Failed() ? uploaded.hr : S_OK;
                    if (uploaded.skipped)
                    {
                        moveHr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                    }
                }
                WpdDeleteDiskTree(tempDirectory);
            }
            if (SUCCEEDED(moveHr))
            {
                WpdDeleteObject(device, destinationId);
            }
            ::CoTaskMemFree(finalId);
            ::CoTaskMemFree(destinationId);
            return moveHr;
        }
        ::CoTaskMemFree(destinationId);
        return createHr;
    }
    ::CoTaskMemFree(destinationId);
    return hr;
}
static void WpdReleaseReconnect(CWpdDevice* first, CWpdDevice* second)
{
    if (first != nullptr)
    {
        first->ForceClose();
    }
    if (second != nullptr && second != first)
    {
        second->ForceClose();
    }
}

HRESULT WINAPI CWpdFS::FinishWpdBatchError(WpdCopyContext& context, HRESULT hr, const char* name)
{
    if (WpdRequiresDeviceReconnect(hr))
    {
        context.stop = true;
        WpdReleaseReconnect(context.sourceDevice, context.targetDevice);
        if (!WpdReportedError)
        {
            WpdReportedError = true;
            std::string message = WpdText(IDS_RECONNECT_DEVICE_REQUIRED);
            std::string title = WpdText(IDS_OPERATIONPROGRESS_TITLE);
            SalamanderGeneral->DialogError(WpdDialogParent(context.parent), BUTTONS_OK, "", message.c_str(), title.c_str());
        }
        ChangeDirectory("\\");
        SalamanderGeneral->PostRefreshPanelFS(this);
        return hr;
    }
    if (!WpdIsOperationCancelled(hr) && !WpdReportedError)
    {
        WpdShowOperationError(context.parent, context.operationId, name, hr);
    }
    return hr;
}
static HRESULT WpdRenameFileFallback(WpdCopyContext& context, CWpdDevice* device, CWpdBaseContentItem* item,
                                     const wchar_t* wideName, const char* newName)
{
    std::wstring parentId;
    HRESULT hr = WpdReadParentId(device, item->GetObjectId(), parentId);
    if (FAILED(hr))
    {
        return hr;
    }

    std::wstring originalName;
    std::wstring objectName;
    bool hasOriginal = false;
    bool hasObject = false;
    DWORD storedAttributes = 0;
    hr = WpdReadStoredNames(device, item->GetObjectId(), originalName, hasOriginal, objectName, hasObject, storedAttributes);
    if (FAILED(hr))
    {
        return hr;
    }
    const std::wstring& oldWide = hasOriginal ? originalName : objectName;
    std::string oldPanel;
    if (!WpdWideToUtf8(oldWide.c_str(), oldPanel))
    {
        return E_FAIL;
    }

    ULONGLONG size = 0;
    bool hasSize = false;
    FILETIME time = {};
    bool hasTime = false;
    WpdReadSizeAndTime(device, item->GetObjectId(), size, hasSize, time, hasTime);

    PWSTR existingId = nullptr;
    DWORD existingAttributes = 0;
    HRESULT findHr = WpdFindChild(device, parentId.c_str(), newName, nullptr, &existingId, &existingAttributes);
    bool sameObject = findHr == S_OK && existingId != nullptr && lstrcmpW(existingId, item->GetObjectId()) == 0;
    ::CoTaskMemFree(existingId);
    if (FAILED(findHr))
    {
        return findHr;
    }

    if (sameObject)
    {
        std::wstring tempDirectory;
        hr = WpdMakeTempDirectory(tempDirectory);
        if (FAILED(hr))
        {
            return hr;
        }
        std::wstring tempFile = tempDirectory;
        WpdAppendComponent(tempFile, L"item");
        hr = WpdDownloadObjectWide(context, device, item->GetObjectId(), tempFile);
        if (FAILED(hr))
        {
            WpdDeleteDiskTree(tempDirectory);
            return hr;
        }

        bool gone = false;
        hr = WpdDeleteAndConfirmGone(context, device, parentId.c_str(), item->GetObjectId(), newName, gone);
        if (FAILED(hr) || !gone)
        {
            WpdDeleteDiskTree(tempDirectory);
            if (!WpdRequiresDeviceReconnect(hr))
            {
                bool skip = false;
                HRESULT askHr = WpdReportNameRemains(context.parent, newName, context.conflict, skip);
                if (SUCCEEDED(askHr) && skip)
                {
                    return S_FALSE;
                }
                return FAILED(askHr) ? askHr : HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
            }
            return hr;
        }

        WpdOutcome uploaded = WpdUploadDisk(context, parentId.c_str(), tempFile, wideName, newName, newName, size, hasSize, hasTime ? &time : nullptr, hasTime);
        if (!uploaded.Completed())
        {
            WpdOutcome restored = WpdUploadDisk(context, parentId.c_str(), tempFile, oldWide.c_str(), oldPanel.c_str(), oldPanel.c_str(), size, hasSize, hasTime ? &time : nullptr, hasTime);
            if (!restored.Completed())
            {
                std::string tempUtf8;
                if (WpdWideToUtf8(tempFile.c_str(), tempUtf8) && !WpdReportedError)
                {
                    WpdShowOperationError(context.parent, IDS_OPERATION_RENAME, tempUtf8.c_str(), uploaded.Failed() ? uploaded.hr : restored.hr);
                }
                return uploaded.Failed() ? uploaded.hr : HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            }
            WpdDeleteDiskTree(tempDirectory);
            return uploaded.skipped ? S_FALSE : uploaded.hr;
        }
        WpdDeleteDiskTree(tempDirectory);
        return S_OK;
    }

    ATL::CComPtr<IStream> source;
    hr = WpdOpenReadStream(device, item->GetObjectId(), source);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = WpdPutFileStream(context, device, parentId.c_str(), wideName, newName, newName, source, size, hasSize, hasTime ? &time : nullptr, hasTime, nullptr, nullptr, item->GetObjectId());
    if (hr == S_FALSE)
    {
        return S_FALSE;
    }
    if (FAILED(hr))
    {
        return hr;
    }
    return WpdDeleteObject(device, item->GetObjectId());
}

static ULONGLONG WpdPanelSelectionBytes(int panel, BOOL focused)
{
    ULONGLONG total = 0;
    int index = 0;
    for (;;)
    {
        BOOL isDir = FALSE;
        const CFileData* file = focused ? SalamanderGeneral->GetPanelFocusedItem(panel, &isDir)
                                        : SalamanderGeneral->GetPanelSelectedItem(panel, &index, &isDir);
        if (file == nullptr)
        {
            break;
        }
        if ((file->Attr & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            total += file->Size.Value;
        }
        if (focused)
        {
            break;
        }
    }
    return total;
}

static std::string WpdOperationTitle(UINT operationId, CWpdDevice* device)
{
    std::string title = WpdText(operationId);
    if (device != nullptr)
    {
        CFxString name;
        device->GetName(name);
        if (!name.IsEmpty())
        {
            title += " - ";
            title += name.GetString();
        }
    }
    return title;
}

static HRESULT WpdConfirmDiskTarget(WpdCopyContext& context, const std::wstring& targetPath, const char* sourceName,
                                   bool sourceIsDir, ULONGLONG sourceSize, bool sourceHasSize, const FILETIME* sourceTime, bool sourceHasTime)
{
    if (context.conflict.skipAll)
    {
        return S_FALSE;
    }

    std::wstring native = WpdForWin32(targetPath);
    DWORD attributes = ::GetFileAttributesW(native.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        return S_OK;
    }

    bool destinationIsDir = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    if (destinationIsDir != sourceIsDir)
    {
        bool skip = false;
        HRESULT askHr = WpdAskTypeMismatch(context.parent, sourceName, sourceIsDir, context.conflict, skip);
        if (SUCCEEDED(askHr) && skip)
        {
            return S_FALSE;
        }
        return FAILED(askHr) ? askHr : HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }

    if (context.conflict.overwriteAll)
    {
        return S_OK;
    }

    ULONGLONG destinationSize = 0;
    bool destinationHasSize = false;
    FILETIME destinationTime = {};
    bool destinationHasTime = false;
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (::GetFileAttributesExW(native.c_str(), GetFileExInfoStandard, &info))
    {
        destinationHasSize = !destinationIsDir;
        destinationSize = (static_cast<ULONGLONG>(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
        destinationTime = info.ftLastWriteTime;
        destinationHasTime = true;
    }

    std::string targetUtf8;
    if (!WpdWideToUtf8(targetPath.c_str(), targetUtf8))
    {
        targetUtf8.clear();
    }
    std::string targetInfo = WpdFormatDetails(destinationSize, destinationHasSize, destinationHasTime ? &destinationTime : nullptr, destinationHasTime);
    std::string sourceInfo = WpdFormatDetails(sourceSize, sourceHasSize && !sourceIsDir, sourceTime, sourceHasTime);
    int answer = SalamanderGeneral->DialogOverwrite(WpdDialogParent(context.parent), BUTTONS_YESALLSKIPCANCEL,
                                                    targetUtf8.c_str(), targetInfo.c_str(),
                                                    sourceName != nullptr ? sourceName : "", sourceInfo.c_str());
    if (answer == DIALOG_ALL)
    {
        context.conflict.overwriteAll = true;
        return S_OK;
    }
    if (answer == DIALOG_YES)
    {
        return S_OK;
    }
    if (answer == DIALOG_SKIPALL)
    {
        context.conflict.skipAll = true;
        return S_FALSE;
    }
    if (answer == DIALOG_SKIP)
    {
        return S_FALSE;
    }
    return HRESULT_FROM_WIN32(ERROR_CANCELLED);
}

HRESULT WINAPI CWpdFS::CreateWpdFolder(CWpdDevice* device, PCWSTR parentObjectId, PCSTR name)
{
    std::wstring wideName;
    HRESULT hr = WpdPanelTextToWide(name, wideName);
    if (FAILED(hr))
    {
        return hr;
    }
    return WpdCreateFolderChecked(device, parentObjectId, wideName.c_str(), name);
}

HRESULT WINAPI CWpdFS::FindWpdChildObject(CWpdDevice* device, PCWSTR parentObjectId, PCSTR childName, _Out_ PWSTR* childObjectId, _Out_opt_ DWORD* attributes)
{
    return WpdFindChild(device, parentObjectId, childName, nullptr, childObjectId, attributes);
}

HRESULT WINAPI CWpdFS::AddWpdObjectId(IPortableDevicePropVariantCollection* objects, PCWSTR objectId)
{
    return WpdAddObjectId(objects, objectId);
}

HRESULT WINAPI CWpdFS::DeleteWpdObjects(CWpdDevice* device, IPortableDevicePropVariantCollection* objects)
{
    bool hold = !device->IsOpen();
    HRESULT hr = S_OK;
    if (hold)
    {
        hr = device->Open(GENERIC_READ | GENERIC_WRITE);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    ATL::CComPtr<IPortableDevicePropVariantCollection> results;
    hr = device->GetContentNoAddRef()->Delete(PORTABLE_DEVICE_DELETE_WITH_RECURSION, objects, &results);
    HRESULT itemsHr = SUCCEEDED(hr) ? WpdReadResultCollection(results) : S_OK;
    if (hold)
    {
        device->Close();
    }
    if (FAILED(hr))
    {
        return hr;
    }
    return itemsHr;
}

HRESULT WINAPI CWpdFS::CopyOrMoveWpdObjects(CWpdDevice* device, IPortableDevicePropVariantCollection* objects, PCWSTR destinationObjectId, bool copy)
{
    bool hold = !device->IsOpen();
    HRESULT hr = S_OK;
    if (hold)
    {
        hr = device->Open(GENERIC_READ | GENERIC_WRITE);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    ATL::CComPtr<IPortableDevicePropVariantCollection> results;
    if (copy)
    {
        hr = device->GetContentNoAddRef()->Copy(objects, destinationObjectId, &results);
    }
    else
    {
        hr = device->GetContentNoAddRef()->Move(objects, destinationObjectId, &results);
    }
    HRESULT itemsHr = SUCCEEDED(hr) ? WpdReadResultCollection(results) : S_OK;
    if (hold)
    {
        device->Close();
    }
    if (FAILED(hr))
    {
        return hr;
    }
    return itemsHr;
}

HRESULT WINAPI CWpdFS::DownloadWpdObject(CWpdDevice* device, PCWSTR objectId, PCSTR targetName)
{
    std::wstring wideTarget;
    HRESULT hr = WpdPanelTextToWide(targetName, wideTarget);
    if (FAILED(hr))
    {
        return hr;
    }

    bool hold = !device->IsOpen();
    if (hold)
    {
        hr = device->Open(GENERIC_READ | GENERIC_WRITE);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    WpdCopyContext context;
    context.sourceDevice = device;
    context.targetDevice = device;
    context.progress = WpdActiveProgress;
    context.parent = WpdDialogOwner;
    hr = WpdDownloadObjectWide(context, device, objectId, wideTarget);
    if (hold)
    {
        device->Close();
    }
    return hr;
}

HRESULT WINAPI CWpdFS::DownloadWpdFile(CWpdBaseContentItem* item, PCSTR targetName)
{
    return DownloadWpdObject(item->GetDeviceNoAddRef(), item->GetObjectId(), targetName);
}

HRESULT WINAPI CWpdFS::UploadDiskObject(CWpdDevice* device, PCWSTR parentObjectId, PCSTR sourceName, PCSTR targetName)
{
    std::wstring sourceWide;
    std::wstring targetWide;
    HRESULT hr = WpdPanelTextToWide(sourceName, sourceWide);
    if (SUCCEEDED(hr))
    {
        hr = WpdPanelTextToWide(targetName, targetWide);
    }
    if (FAILED(hr))
    {
        return hr;
    }

    bool hold = !device->IsOpen();
    if (hold)
    {
        hr = device->Open(GENERIC_READ | GENERIC_WRITE);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    ULONGLONG size = 0;
    bool hasSize = false;
    FILETIME time = {};
    bool hasTime = false;
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (::GetFileAttributesExW(WpdForWin32(sourceWide).c_str(), GetFileExInfoStandard, &info))
    {
        hasSize = (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
        size = (static_cast<ULONGLONG>(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
        time = info.ftLastWriteTime;
        hasTime = true;
    }

    WpdCopyContext context;
    context.parent = WpdDialogOwner;
    context.targetDevice = device;
    context.sourceDevice = device;
    context.progress = WpdActiveProgress;
    context.operationId = IDS_OPERATIONPROGRESS_COPY;
    WpdOutcome outcome = WpdUploadDisk(context, parentObjectId, sourceWide, targetWide.c_str(), targetName, sourceName, size, hasSize, hasTime ? &time : nullptr, hasTime);
    if (hold)
    {
        device->Close();
    }
    if (outcome.skipped)
    {
        return S_FALSE;
    }
    return outcome.hr;
}

HRESULT WINAPI CWpdFS::UploadDiskFile(CWpdDevice* device, PCWSTR parentObjectId, PCSTR sourceName, PCSTR targetName)
{
    return UploadDiskObject(device, parentObjectId, sourceName, targetName);
}

HRESULT WINAPI CWpdFS::RenameWpdObject(HWND parent, CWpdBaseContentItem* item, PCSTR newName)
{
    if (item == nullptr || newName == nullptr || newName[0] == '\0')
    {
        return E_INVALIDARG;
    }

    CWpdDevice* device = item->GetDeviceNoAddRef();
    bool opened = false;
    HRESULT hr = S_OK;
    if (!device->IsOpen())
    {
        hr = device->Open(GENERIC_READ | GENERIC_WRITE);
        if (FAILED(hr))
        {
            return hr;
        }
        opened = true;
    }

    std::wstring wideName;
    if (SUCCEEDED(hr))
    {
        hr = WpdPanelTextToWide(newName, wideName);
    }

    HRESULT nameHr = S_OK;
    HRESULT originalHr = S_OK;
    bool isDirectory = (item->GetAttributes() & FILE_ATTRIBUTE_DIRECTORY) != 0;
    if (SUCCEEDED(hr))
    {
        std::wstring originalName;
        std::wstring objectName;
        bool hasOriginal = false;
        bool hasObject = false;
        DWORD attributes = 0;
        if (SUCCEEDED(WpdReadStoredNames(device, item->GetObjectId(), originalName, hasOriginal, objectName, hasObject, attributes)) &&
            (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
            isDirectory = true;
        }

        if (WpdActiveProgress != nullptr)
        {
            WpdActiveProgress->BeginItem(newName, static_cast<ULONGLONG>(-1));
        }

        if (WpdRenamePlanForObject(isDirectory) == WpdRenameFolderNameOnly)
        {
            // Folders keep ORIGINAL_FILE_NAME unchanged. Matching only that
            // property made every successful rename look like a failure and
            // triggered the download/upload fallback.
            nameHr = WpdSetStringProperty(device, item->GetObjectId(), WPD_OBJECT_NAME, wideName.c_str(), nameHr);
            hr = nameHr;
        }
        else
        {
            nameHr = WpdSetStringProperty(device, item->GetObjectId(), WPD_OBJECT_NAME, wideName.c_str(), nameHr);
            if (!WpdRequiresDeviceReconnect(nameHr))
            {
                originalHr = WpdSetStringProperty(device, item->GetObjectId(), WPD_OBJECT_ORIGINAL_FILE_NAME, wideName.c_str(), originalHr);
            }
            if (WpdRequiresDeviceReconnect(nameHr))
            {
                hr = nameHr;
            }
            else if (WpdRequiresDeviceReconnect(originalHr))
            {
                hr = originalHr;
            }
            else if (SUCCEEDED(nameHr) || SUCCEEDED(originalHr))
            {
                hr = S_OK;
            }
            else
            {
                hr = FAILED(nameHr) ? nameHr : originalHr;
            }
        }
    }

    bool matched = false;
    if (!WpdRequiresDeviceReconnect(hr))
    {
        std::wstring originalName;
        std::wstring objectName;
        bool hasOriginal = false;
        bool hasObject = false;
        DWORD attributes = 0;
        if (SUCCEEDED(WpdReadStoredNames(device, item->GetObjectId(), originalName, hasOriginal, objectName, hasObject, attributes)))
        {
            // Either stored name may carry the new display name. Folders only
            // update WPD_OBJECT_NAME, so ORIGINAL_FILE_NAME alone is not enough.
            matched = WpdStoredNamesMatch(originalName, hasOriginal, objectName, hasObject, newName);
        }
        else if (SUCCEEDED(hr))
        {
            // Some devices accept SetValues but refuse a follow-up GetValues.
            // Treat a successful write as done instead of copying the tree.
            matched = true;
        }
    }

    if (matched)
    {
        if (opened)
        {
            device->Close();
        }
        return S_OK;
    }

    if (WpdRequiresDeviceReconnect(hr))
    {
        if (opened)
        {
            device->Close();
        }
        return hr;
    }

    hr = RenameWpdObjectByCopy(parent, item, newName);
    if (opened)
    {
        device->Close();
    }
    return hr;
}

HRESULT WINAPI CWpdFS::RenameWpdObjectByCopy(HWND parent, CWpdBaseContentItem* item, PCSTR newName)
{
    CWpdDevice* device = item->GetDeviceNoAddRef();
    WpdCopyContext context;
    context.parent = parent;
    context.operationId = IDS_OPERATION_RENAME;
    context.sourceDevice = device;
    context.targetDevice = device;
    context.progress = WpdActiveProgress;

    std::wstring wideName;
    HRESULT hr = WpdPanelTextToWide(newName, wideName);
    if (SUCCEEDED(hr))
    {
        bool isDirectory = (item->GetAttributes() & FILE_ATTRIBUTE_DIRECTORY) != 0;
        std::wstring originalName;
        std::wstring objectName;
        bool hasOriginal = false;
        bool hasObject = false;
        DWORD attributes = 0;
        if (SUCCEEDED(WpdReadStoredNames(device, item->GetObjectId(), originalName, hasOriginal, objectName, hasObject, attributes)))
        {
            isDirectory = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        }

        if (WpdRenamePlanForObject(isDirectory) == WpdRenameFolderNameOnly)
        {
            hr = WpdRenameFolderFallback(context, device, item->GetObjectId(), wideName.c_str(), newName);
        }
        else
        {
            hr = WpdRenameFileFallback(context, device, item, wideName.c_str(), newName);
        }
    }

    if (WpdRequiresDeviceReconnect(hr))
    {
        WpdReleaseReconnect(device, nullptr);
    }
    return hr;
}

BOOL WINAPI CWpdFS::QuickRename(const char*, int mode, HWND parent, CFileData& file, BOOL, char* newName, BOOL& cancel)
{
    cancel = FALSE;
    if (mode == 1)
    {
        return FALSE;
    }

    HWND previousOwner = WpdDialogOwner;
    WpdDialogOwner = parent;
    WpdReportedError = false;
    auto item = static_cast<CWpdBaseContentItem*>(reinterpret_cast<CFxItem*>(file.PluginData));
    CWpdStandardProgress progress;
    progress.Open(parent, WpdOperationTitle(IDS_OPERATION_RENAME, item->GetDeviceNoAddRef()));
    HRESULT hr = RenameWpdObject(parent, item, newName);
    progress.Close();
    WpdDialogOwner = previousOwner;

    if (hr == S_FALSE)
    {
        return FALSE;
    }
    if (FAILED(hr))
    {
        if (WpdIsOperationCancelled(hr))
        {
            cancel = TRUE;
            return FALSE;
        }
        if (HandleDeviceReconnectRequired(parent, hr, item->GetDeviceNoAddRef()))
        {
            return FALSE;
        }
        if (!WpdReportedError)
        {
            WpdShowOperationError(parent, IDS_OPERATION_RENAME, file.Name, hr);
        }
        return FALSE;
    }

    SalamanderGeneral->PostRefreshPanelFS(this);
    return TRUE;
}

BOOL WINAPI CWpdFS::GetPathForMainWindowTitle(const char* fsName, int mode, char* buf, int bufSize)
{
    if (buf == nullptr || bufSize <= 0)
    {
        return FALSE;
    }

    buf[0] = '\0';

    char currentPath[MAX_PATH];
    GetCurrentPath(currentPath);

    const char* lastComponent = currentPath;
    int len = lstrlen(currentPath);
    while (len > 1 && currentPath[len - 1] == '\\')
    {
        currentPath[--len] = '\0';
    }

    char* lastSlash = strrchr(currentPath, '\\');
    if (lastSlash != nullptr && lastSlash[1] != '\0')
    {
        lastComponent = lastSlash + 1;
    }

    if (mode == 1)
    {
        if (lastComponent[0] == '\\' && lastComponent[1] == '\0')
        {
            StringCchPrintf(buf, bufSize, "%s:%s", fsName, currentPath);
        }
        else
        {
            StringCchCopy(buf, bufSize, lastComponent);
        }
        return TRUE;
    }

    if (mode == 2)
    {
        const char* firstComponent = currentPath;
        if (firstComponent[0] == '\\')
        {
            ++firstComponent;
        }

        const char* firstSlash = strchr(firstComponent, '\\');
        if (firstSlash != nullptr && firstSlash[1] != '\0')
        {
            char rootComponent[MAX_PATH];
            size_t rootLen = firstSlash - firstComponent;
            if (rootLen >= _countof(rootComponent))
            {
                rootLen = _countof(rootComponent) - 1;
            }
            memcpy(rootComponent, firstComponent, rootLen);
            rootComponent[rootLen] = '\0';
            StringCchPrintf(buf, bufSize, "%s:\\%s\\...\\%s", fsName, rootComponent, lastComponent);
        }
        else
        {
            StringCchPrintf(buf, bufSize, "%s:%s", fsName, currentPath);
        }
        return TRUE;
    }

    return FALSE;
}

BOOL WINAPI CWpdFS::GetNextDirectoryLineHotPath(const char* text, int pathLen, int& offset)
{
    const char* end = text + pathLen;
    const char* root = text;
    while (root < end && *root != ':')
    {
        ++root;
    }
    if (root < end && *root == ':')
    {
        ++root;
        if (root < end && *root == '\\')
        {
            ++root;
        }
    }

    const char* s = text + offset;
    if (s >= end)
    {
        return FALSE;
    }
    if (s < root)
    {
        offset = static_cast<int>(root - text);
    }
    else
    {
        if (*s == '\\')
        {
            ++s;
        }
        while (s < end && *s != '\\')
        {
            ++s;
        }
        offset = static_cast<int>(s - text);
    }
    return s < end;
}

void WINAPI CWpdFS::CompleteDirectoryLineHotPath(char* path, int pathBufSize)
{
    if (path == nullptr || pathBufSize <= 0)
    {
        return;
    }

    int len = lstrlen(path);
    while (len > 1 && path[len - 1] == '\\')
    {
        path[--len] = '\0';
    }
}

BOOL WINAPI CWpdFS::HandleDeviceReconnectRequired(HWND parent, HRESULT hr, CWpdDevice* device)
{
    if (!WpdRequiresDeviceReconnect(hr))
    {
        return FALSE;
    }

    if (device != nullptr)
    {
        device->ForceClose();
    }
    if (!WpdReportedError)
    {
        WpdReportedError = true;
        std::string message = WpdText(IDS_RECONNECT_DEVICE_REQUIRED);
        std::string title = WpdText(IDS_OPERATIONPROGRESS_TITLE);
        SalamanderGeneral->DialogError(WpdDialogParent(parent), BUTTONS_OK, "", message.c_str(), title.c_str());
    }
    ChangeDirectory("\\");
    SalamanderGeneral->PostRefreshPanelFS(this);
    return TRUE;
}

BOOL WINAPI CWpdFS::CreateDir(const char*, int mode, HWND parent, char* newName, BOOL& cancel)
{
    cancel = FALSE;
    if (mode == 1)
    {
        return FALSE;
    }

    HWND previousOwner = WpdDialogOwner;
    WpdDialogOwner = parent;
    WpdReportedError = false;

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

    WpdDialogOwner = previousOwner;
    if (hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))
    {
        std::string message = WpdText(IDS_OPERATION_ALREADY_EXISTS);
        std::string title = WpdText(IDS_OPERATIONPROGRESS_TITLE);
        SalamanderGeneral->DialogError(WpdDialogParent(parent), BUTTONS_OK, newName != nullptr ? newName : "", message.c_str(), title.c_str());
        return FALSE;
    }
    if (FAILED(hr))
    {
        if (HandleDeviceReconnectRequired(parent, hr, nullptr))
        {
            return FALSE;
        }
        if (!WpdReportedError)
        {
            WpdShowOperationError(parent, IDS_OPERATION_CREATEFOLDER, newName, hr);
        }
        return FALSE;
    }

    SalamanderGeneral->PostRefreshPanelFS(this);
    return TRUE;
}

void WINAPI CWpdFS::ViewFile(const char* fsName, HWND parent, CSalamanderForViewFileOnFSAbstract* salamander, CFileData& file)
{
    char uniqueFileName[2 * MAX_PATH];
    lstrcpyn(uniqueFileName, fsName, _countof(uniqueFileName));
    StringCchCat(uniqueFileName, _countof(uniqueFileName), ":");
    char currentPath[MAX_PATH];
    GetCurrentPath(currentPath);
    StringCchCat(uniqueFileName, _countof(uniqueFileName), currentPath);
    if (!SalamanderGeneral->SalPathAppend(uniqueFileName, file.Name, _countof(uniqueFileName)))
    {
        WpdShowOperationError(parent, IDS_OPERATION_VIEW, file.Name, HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE));
        return;
    }

    char nameInCache[MAX_PATH];
    lstrcpyn(nameInCache, file.Name, _countof(nameInCache));
    SalamanderGeneral->SalMakeValidFileNameComponent(nameInCache);

    BOOL fileExists = FALSE;
    const char* tmpFileName = salamander->AllocFileNameInCache(parent, uniqueFileName, nameInCache, nullptr, fileExists);
    if (tmpFileName == nullptr)
    {
        return;
    }

    BOOL newFileOK = FALSE;
    CQuadWord newFileSize(0, 0);
    if (!fileExists)
    {
        auto item = static_cast<CWpdBaseContentItem*>(reinterpret_cast<CFxItem*>(file.PluginData));
        HRESULT hr = DownloadWpdFile(item, tmpFileName);
        if (SUCCEEDED(hr))
        {
            newFileOK = TRUE;
            ULONGLONG size = 0;
            if (SUCCEEDED(item->GetSize(size)))
            {
                newFileSize.SetUI64(size);
            }
        }
        else
        {
            if (!HandleDeviceReconnectRequired(parent, hr, item->GetDeviceNoAddRef())) WpdShowOperationError(parent, IDS_OPERATION_VIEW, file.Name, hr);
        }
    }

    HANDLE fileLock = nullptr;
    BOOL fileLockOwner = FALSE;
    if ((fileExists || newFileOK) && !salamander->OpenViewer(parent, tmpFileName, &fileLock, &fileLockOwner))
    {
        fileLock = nullptr;
        fileLockOwner = FALSE;
    }

    salamander->FreeFileNameInCache(uniqueFileName, fileExists, newFileOK, newFileSize, fileLock, fileLockOwner, FALSE);
}

BOOL WINAPI CWpdFS::TryShellContextMenu(const char* fsName, HWND parent, int menuX, int menuY, int panel)
{
    UNREFERENCED_PARAMETER(fsName);
    BOOL focusIsDir = FALSE;
    const CFileData* focused = SalamanderGeneral->GetPanelFocusedItem(panel, &focusIsDir);
    UNREFERENCED_PARAMETER(focusIsDir);
    if (focused == nullptr)
    {
        return FALSE;
    }

    char currentPath[MAX_PATH];
    GetCurrentPath(currentPath);

    char fullPortablePath[2 * MAX_PATH];
    lstrcpyn(fullPortablePath, currentPath, _countof(fullPortablePath));
    if (!SalamanderGeneral->SalPathAppend(fullPortablePath, focused->Name, _countof(fullPortablePath)))
    {
        return FALSE;
    }

    char components[16][MAX_PATH];
    int componentCount = 0;
    char* context = nullptr;
    char* token = strtok_s(fullPortablePath, "\\", &context);
    while (token != nullptr && componentCount < _countof(components))
    {
        if (token[0] != '\0')
        {
            lstrcpyn(components[componentCount++], token, _countof(components[0]));
        }
        token = strtok_s(nullptr, "\\", &context);
    }
    if (componentCount == 0)
    {
        return FALSE;
    }

    auto findChildByDisplayName = [](IShellFolder* folder, HWND enumParent, PCSTR name, _Out_ LPITEMIDLIST* childPidl) -> HRESULT
    {
        *childPidl = nullptr;
        ATL::CComPtr<IEnumIDList> childEnum;
        HRESULT hr = folder->EnumObjects(enumParent, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_INCLUDEHIDDEN, &childEnum);
        if (FAILED(hr))
        {
            return hr;
        }

        for (;;)
        {
            LPITEMIDLIST enumPidl = nullptr;
            ULONG fetched = 0;
            hr = childEnum->Next(1, &enumPidl, &fetched);
            if (hr != S_OK)
            {
                return S_FALSE;
            }

            char displayName[MAX_PATH];
            STRRET strret;
            hr = folder->GetDisplayNameOf(enumPidl, SHGDN_NORMAL, &strret);
            if (SUCCEEDED(hr))
            {
                hr = ::StrRetToBuf(&strret, enumPidl, displayName, _countof(displayName));
            }
            if (SUCCEEDED(hr) && SalamanderGeneral->StrICmp(displayName, name) == 0)
            {
                *childPidl = enumPidl;
                return S_OK;
            }
            ::CoTaskMemFree(enumPidl);
        }
    };

    static PCWSTR const shellRoots[] =
        {
            L"::{35786D3C-B075-49b9-88DD-029876E11C01}", // Portable Devices
            L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}", // This PC / My Computer
        };

    ATL::CComPtr<IShellFolder> parentFolder;
    LPITEMIDLIST childPidl = nullptr;
    HRESULT hr = E_FAIL;
    for (int rootIndex = 0; rootIndex < _countof(shellRoots) && parentFolder == nullptr; ++rootIndex)
    {
        LPITEMIDLIST rootPidl = nullptr;
        hr = ::SHParseDisplayName(shellRoots[rootIndex], nullptr, &rootPidl, 0, nullptr);
        if (FAILED(hr) || rootPidl == nullptr)
        {
            continue;
        }

        ATL::CComPtr<IShellFolder> desktop;
        ATL::CComPtr<IShellFolder> currentFolder;
        hr = ::SHGetDesktopFolder(&desktop);
        if (SUCCEEDED(hr))
        {
            hr = desktop->BindToObject(rootPidl, nullptr, IID_IShellFolder, reinterpret_cast<void**>(&currentFolder.p));
        }
        ::CoTaskMemFree(rootPidl);
        if (FAILED(hr) || currentFolder == nullptr)
        {
            continue;
        }

        for (int component = 0; component < componentCount; ++component)
        {
            LPITEMIDLIST nextPidl = nullptr;
            hr = findChildByDisplayName(currentFolder, parent, components[component], &nextPidl);
            if (FAILED(hr) || nextPidl == nullptr)
            {
                break;
            }
            if (component == componentCount - 1)
            {
                parentFolder = currentFolder;
                childPidl = nextPidl;
                hr = S_OK;
                break;
            }

            ATL::CComPtr<IShellFolder> nextFolder;
            hr = currentFolder->BindToObject(nextPidl, nullptr, IID_IShellFolder, reinterpret_cast<void**>(&nextFolder.p));
            ::CoTaskMemFree(nextPidl);
            if (FAILED(hr) || nextFolder == nullptr)
            {
                break;
            }
            currentFolder = nextFolder;
        }
    }
    if (parentFolder == nullptr || childPidl == nullptr)
    {
        return FALSE;
    }

    LPCITEMIDLIST childPidlConst = childPidl;
    ATL::CComPtr<IContextMenu> contextMenu;
    hr = parentFolder->GetUIObjectOf(parent, 1, &childPidlConst, IID_IContextMenu, nullptr, reinterpret_cast<void**>(&contextMenu.p));
    if (FAILED(hr) || contextMenu == nullptr)
    {
        ::CoTaskMemFree(childPidl);
        return FALSE;
    }

    HMENU menu = ::CreatePopupMenu();
    if (menu == nullptr)
    {
        ::CoTaskMemFree(childPidl);
        return FALSE;
    }

    m_shellContextMenu2.Release();
    m_shellContextMenu3.Release();
    contextMenu->QueryInterface(IID_IContextMenu2, reinterpret_cast<void**>(&m_shellContextMenu2.p));
    contextMenu->QueryInterface(IID_IContextMenu3, reinterpret_cast<void**>(&m_shellContextMenu3.p));

    bool menuShown = SUCCEEDED(contextMenu->QueryContextMenu(menu, 0, 1, 0x7FFF, CMF_NORMAL | CMF_EXPLORE));
    if (menuShown)
    {
        UINT cmd = ::TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, menuX, menuY, parent, nullptr);
        if (cmd != 0)
        {
            CMINVOKECOMMANDINFO invoke = {};
            invoke.cbSize = sizeof(invoke);
            invoke.hwnd = parent;
            invoke.lpVerb = MAKEINTRESOURCEA(cmd - 1);
            invoke.nShow = SW_SHOWNORMAL;
            contextMenu->InvokeCommand(&invoke);
        }
    }

    m_shellContextMenu3.Release();
    m_shellContextMenu2.Release();
    ::DestroyMenu(menu);
    ::CoTaskMemFree(childPidl);
    return menuShown;
}

void WINAPI CWpdFS::ContextMenu(const char* fsName, HWND parent, int menuX, int menuY, int type, int panel, int selectedFiles, int selectedDirs)
{
    if (type == fscmItemsInPanel && selectedFiles + selectedDirs <= 1 && TryShellContextMenu(fsName, parent, menuX, menuY, panel))
    {
        return;
    }

    CGUIMenuPopupAbstract* menu = SalamanderGUI->CreateMenuPopup();
    if (menu == nullptr)
    {
        return;
    }

    auto insertSalamanderCommand = [menu](int& index, int command, DWORD state = 0)
    {
        char name[256];
        if (!SalamanderGeneral->GetSalamanderCommand(command, name, _countof(name), nullptr, nullptr))
        {
            return;
        }

        MENU_ITEM_INFO mi;
        mi.Mask = MENU_MASK_TYPE | MENU_MASK_STATE | MENU_MASK_ID | MENU_MASK_STRING;
        mi.Type = MENU_TYPE_STRING;
        mi.State = state;
        mi.ID = command + 1;
        mi.String = name;
        menu->InsertItem(index++, TRUE, &mi);
    };

    auto insertSeparator = [menu](int& index)
    {
        MENU_ITEM_INFO mi;
        mi.Mask = MENU_MASK_TYPE;
        mi.Type = MENU_TYPE_SEPARATOR;
        menu->InsertItem(index++, TRUE, &mi);
    };

    int index = 0;
    if (type == fscmItemsInPanel)
    {
        BOOL focusIsDir = FALSE;
        const CFileData* focused = SalamanderGeneral->GetPanelFocusedItem(panel, &focusIsDir);
        const bool hasFocusedItem = focused != nullptr;
        const bool multipleSelection = selectedFiles + selectedDirs > 1;
        const DWORD fileOnlyState = (!hasFocusedItem || focusIsDir || multipleSelection) ? MENU_STATE_GRAYED : 0;

        insertSalamanderCommand(index, SALCMD_OPEN, MENU_STATE_DEFAULT);
        insertSalamanderCommand(index, SALCMD_VIEW, fileOnlyState);
        insertSalamanderCommand(index, SALCMD_VIEWWITH, fileOnlyState);
        insertSalamanderCommand(index, SALCMD_EDIT, fileOnlyState);
        insertSalamanderCommand(index, SALCMD_EDITWITH, fileOnlyState);
        insertSeparator(index);
        insertSalamanderCommand(index, SALCMD_COPY);
        insertSalamanderCommand(index, SALCMD_MOVE);
        insertSalamanderCommand(index, SALCMD_DELETE);
        insertSalamanderCommand(index, SALCMD_QUICKRENAME, multipleSelection ? MENU_STATE_GRAYED : 0);
    }
    else if (type == fscmPathInPanel || type == fscmPanel)
    {
        insertSalamanderCommand(index, SALCMD_CREATEDIRECTORY);
        insertSeparator(index);
        insertSalamanderCommand(index, SALCMD_REFRESH);
    }

    DWORD cmd = menu->Track(MENU_TRACK_RETURNCMD | MENU_TRACK_RIGHTBUTTON | MENU_TRACK_NONOTIFY, menuX, menuY, parent, nullptr);
    if (cmd > 0)
    {
        SalamanderGeneral->PostSalamanderCommand(cmd - 1);
    }

    SalamanderGUI->DestroyMenuPopup(menu);
}

BOOL WINAPI CWpdFS::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plResult)
{
    if (m_shellContextMenu3 != nullptr)
    {
        return SUCCEEDED(m_shellContextMenu3->HandleMenuMsg2(uMsg, wParam, lParam, plResult));
    }
    if (m_shellContextMenu2 != nullptr)
    {
        return SUCCEEDED(m_shellContextMenu2->HandleMenuMsg(uMsg, wParam, lParam));
    }
    return FALSE;
}

BOOL WINAPI CWpdFS::Delete(const char*, int mode, HWND parent, int panel, int selectedFiles, int selectedDirs, BOOL& cancelOrError)
{
    cancelOrError = FALSE;
    if (mode == 1)
    {
        return FALSE;
    }

    HWND previousOwner = WpdDialogOwner;
    WpdDialogOwner = parent;
    WpdReportedError = false;

    ATL::CComPtr<IPortableDevicePropVariantCollection> objects;
    HRESULT hr = objects.CoCreateInstance(CLSID_PortableDevicePropVariantCollection);
    if (FAILED(hr))
    {
        WpdDialogOwner = previousOwner;
        if (!HandleDeviceReconnectRequired(parent, hr, nullptr))
        {
            WpdShowOperationError(parent, IDS_OPERATIONPROGRESS_DELETE, "", hr);
        }
        cancelOrError = TRUE;
        return TRUE;
    }

    BOOL focused = (selectedFiles == 0 && selectedDirs == 0);
    CWpdDevice* device = nullptr;
    bool opened = false;
    CWpdStandardProgress progress;
    progress.AddTotal(focused ? 1 : static_cast<ULONGLONG>(selectedFiles + selectedDirs));
    bool ok = true;
    int index = 0;
    for (;;)
    {
        BOOL isDir = FALSE;
        const CFileData* file = focused ? SalamanderGeneral->GetPanelFocusedItem(panel, &isDir)
                                        : SalamanderGeneral->GetPanelSelectedItem(panel, &index, &isDir);
        if (file == nullptr)
        {
            break;
        }

        auto item = static_cast<CWpdBaseContentItem*>(reinterpret_cast<CFxItem*>(file->PluginData));
        if (device == nullptr)
        {
            device = item->GetDeviceNoAddRef();
            progress.Open(parent, WpdOperationTitle(IDS_OPERATIONPROGRESS_DELETE, device));
            hr = device->Open(GENERIC_READ | GENERIC_WRITE);
            if (FAILED(hr))
            {
                WpdCopyContext context;
                context.parent = parent;
                context.operationId = IDS_OPERATIONPROGRESS_DELETE;
                context.sourceDevice = device;
                context.targetDevice = device;
                FinishWpdBatchError(context, hr, file->Name);
                ok = false;
                break;
            }
            opened = true;
        }

        if (!progress.BeginItem(file->Name, 1))
        {
            ok = false;
            break;
        }
        hr = AddWpdObjectId(objects, item->GetObjectId());
        if (FAILED(hr))
        {
            WpdCopyContext context;
            context.parent = parent;
            context.operationId = IDS_OPERATIONPROGRESS_DELETE;
            context.sourceDevice = device;
            context.targetDevice = device;
            FinishWpdBatchError(context, hr, file->Name);
            ok = false;
            break;
        }
        progress.EndItem(1);
        if (focused)
        {
            break;
        }
    }

    if (ok && device != nullptr && opened)
    {
        std::string deleting = WpdText(IDS_OPERATIONPROGRESS_DELETINGSELECTED);
        if (!progress.BeginItem(deleting.c_str(), 1))
        {
            ok = false;
        }
        else
        {
            hr = DeleteWpdObjects(device, objects);
            if (FAILED(hr))
            {
                WpdCopyContext context;
                context.parent = parent;
                context.operationId = IDS_OPERATIONPROGRESS_DELETE;
                context.sourceDevice = device;
                context.targetDevice = device;
                FinishWpdBatchError(context, hr, "");
                ok = false;
            }
            else
            {
                progress.EndItem(1);
            }
        }
    }

    progress.Close();
    if (opened && device != nullptr)
    {
        device->Close();
    }
    WpdDialogOwner = previousOwner;
    cancelOrError = !ok;
    if (ok && device != nullptr)
    {
        SalamanderGeneral->PostRefreshPanelFS(this);
    }
    return TRUE;
}

static std::string WpdTargetUserPart(const char* fsName, const char* targetPath)
{
    const char* raw = WpdGetUserPartFromFSPath(fsName, targetPath);
    std::string user = raw != nullptr ? raw : "";
    std::vector<char> mutableUser(user.begin(), user.end());
    mutableUser.push_back('\0');
    WpdStripOperationMask(mutableUser.data(), static_cast<int>(mutableUser.size()));
    return std::string(mutableUser.data());
}

static bool WpdHoldSourceDevice(std::vector<CWpdDevice*>& opened, CWpdDevice* source, CWpdDevice* target, HRESULT& hr)
{
    if (source == nullptr || source == target)
    {
        return true;
    }
    for (size_t i = 0; i < opened.size(); ++i)
    {
        if (opened[i] == source)
        {
            return true;
        }
    }
    hr = source->Open(GENERIC_READ | GENERIC_WRITE);
    if (FAILED(hr))
    {
        return false;
    }
    opened.push_back(source);
    return true;
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
    HWND dropTarget)
{
    UNREFERENCED_PARAMETER(dropTarget);
    operationMask = FALSE;
    cancelOrHandlePath = FALSE;

    if (mode == 1)
    {
        if (targetPath[0] == '\0')
        {
            std::vector<char> path(32768, '\0');
            int targetPanel = (panel == PANEL_LEFT ? PANEL_RIGHT : PANEL_LEFT);
            int type = 0;
            char* fs = nullptr;
            if (SalamanderGeneral->GetPanelPath(targetPanel, path.data(), static_cast<int>(path.size()), &type, &fs))
            {
                // The caller buffer has no length argument. Salamander passes the historical 2 * MAX_PATH array.
                lstrcpyn(targetPath, path.data(), 2 * MAX_PATH);
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
    bool isOurFsTarget = SalamanderGeneral->StrNICmp(targetPath, fsName, fsNameLen) == 0 && targetPath[fsNameLen] == ':';
    UINT operationId = copy ? IDS_OPERATIONPROGRESS_COPY : IDS_OPERATIONPROGRESS_MOVE;

    if (isOurFsTarget)
    {
        HWND previousOwner = WpdDialogOwner;
        WpdDialogOwner = parent;
        WpdReportedError = false;

        std::string userPart = WpdTargetUserPart(fsName, targetPath);
        CWpdDevice* targetDevice = nullptr;
        CFxString targetObjectId;
        HRESULT hr = GetContentLocationForPath(userPart.c_str(), targetDevice, targetObjectId);
        if (FAILED(hr) || targetDevice == nullptr)
        {
            if (targetDevice != nullptr)
            {
                targetDevice->Release();
            }
            WpdDialogOwner = previousOwner;
            HRESULT reportHr = FAILED(hr) ? hr : E_FAIL;
            if (!HandleDeviceReconnectRequired(parent, reportHr, nullptr))
            {
                WpdShowOperationError(parent, operationId, targetPath, reportHr);
            }
            cancelOrHandlePath = TRUE;
            return TRUE;
        }

        ATL::CA2W wideTarget(targetObjectId);
        BOOL focused = (selectedFiles == 0 && selectedDirs == 0);
        CWpdStandardProgress progress;
        progress.AddTotal(WpdPanelSelectionBytes(panel, focused));
        progress.Open(parent, WpdOperationTitle(operationId, targetDevice));
        progress.CommitTotals();

        WpdCopyContext context;
        context.parent = parent;
        context.operationId = operationId;
        context.progress = &progress;
        context.targetDevice = targetDevice;

        bool ok = true;
        bool targetOpened = false;
        std::vector<CWpdDevice*> openedSources;
        hr = targetDevice->Open(GENERIC_READ | GENERIC_WRITE);
        if (FAILED(hr))
        {
            context.sourceDevice = targetDevice;
            FinishWpdBatchError(context, hr, targetPath);
            ok = false;
        }
        else
        {
            targetOpened = true;
            int index = 0;
            for (;;)
            {
                BOOL isDir = FALSE;
                const CFileData* file = focused ? SalamanderGeneral->GetPanelFocusedItem(panel, &isDir)
                                               : SalamanderGeneral->GetPanelSelectedItem(panel, &index, &isDir);
                if (file == nullptr)
                {
                    break;
                }

                auto item = static_cast<CWpdBaseContentItem*>(reinterpret_cast<CFxItem*>(file->PluginData));
                CWpdDevice* source = item->GetDeviceNoAddRef();
                context.sourceDevice = source;
                if (!WpdHoldSourceDevice(openedSources, source, targetDevice, hr))
                {
                    FinishWpdBatchError(context, hr, file->Name);
                    ok = false;
                    break;
                }

                WpdOutcome outcome = WpdCopyDeviceObject(context, source, item->GetObjectId(), wideTarget, file->Name);
                if (outcome.Failed())
                {
                    FinishWpdBatchError(context, outcome.hr, file->Name);
                    ok = false;
                    break;
                }
                if (!copy && outcome.Completed())
                {
                    hr = WpdDeleteObject(source, item->GetObjectId());
                    if (FAILED(hr))
                    {
                        FinishWpdBatchError(context, hr, file->Name);
                        ok = false;
                        break;
                    }
                }
                if (context.stop || focused)
                {
                    if (context.stop)
                    {
                        ok = false;
                    }
                    break;
                }
            }
        }

        progress.Close();
        for (size_t i = 0; i < openedSources.size(); ++i)
        {
            openedSources[i]->Close();
        }
        if (targetOpened)
        {
            targetDevice->Close();
        }
        targetDevice->Release();
        WpdDialogOwner = previousOwner;
        cancelOrHandlePath = !ok;
        if (ok)
        {
            targetPath[0] = '\0';
            SalamanderGeneral->PostRefreshPanelFS(this);
        }
        return TRUE;
    }

    if (mode == 2 || mode == 5)
    {
        cancelOrHandlePath = TRUE;
        return FALSE;
    }

    if (mode == 3)
    {
        HWND previousOwner = WpdDialogOwner;
        WpdDialogOwner = parent;
        WpdReportedError = false;

        BOOL focused = (selectedFiles == 0 && selectedDirs == 0);
        CWpdStandardProgress progress;
        progress.AddTotal(WpdPanelSelectionBytes(panel, focused));
        WpdCopyContext context;
        context.parent = parent;
        context.operationId = operationId;
        context.progress = &progress;

        bool ok = true;
        bool opened = false;
        CWpdDevice* device = nullptr;
        int index = 0;
        for (;;)
        {
            BOOL isDir = FALSE;
            const CFileData* file = focused ? SalamanderGeneral->GetPanelFocusedItem(panel, &isDir)
                                           : SalamanderGeneral->GetPanelSelectedItem(panel, &index, &isDir);
            if (file == nullptr)
            {
                break;
            }

            auto item = static_cast<CWpdBaseContentItem*>(reinterpret_cast<CFxItem*>(file->PluginData));
            if (device == nullptr)
            {
                device = item->GetDeviceNoAddRef();
                context.sourceDevice = device;
                context.targetDevice = device;
                progress.Open(parent, WpdOperationTitle(operationId, device));
                progress.CommitTotals();
                HRESULT openHr = device->Open(GENERIC_READ | GENERIC_WRITE);
                if (FAILED(openHr))
                {
                    FinishWpdBatchError(context, openHr, file->Name);
                    ok = false;
                    break;
                }
                opened = true;
            }

            std::string targetName;
            HRESULT hr = WpdJoinSalPath(targetPath, file->Name, targetName);
            if (FAILED(hr))
            {
                WpdShowOperationError(parent, operationId, file->Name, hr);
                ok = false;
                break;
            }
            std::wstring targetWide;
            hr = WpdPanelTextToWide(targetName.c_str(), targetWide);
            if (FAILED(hr))
            {
                WpdShowOperationError(parent, operationId, file->Name, hr);
                ok = false;
                break;
            }

            bool sourceIsDir = (file->Attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
            HRESULT confirmHr = WpdConfirmDiskTarget(context, targetWide, file->Name, sourceIsDir, file->Size.Value, !sourceIsDir, &file->LastWrite, TRUE);
            if (confirmHr == S_FALSE)
            {
                if (focused)
                {
                    break;
                }
                continue;
            }
            if (FAILED(confirmHr))
            {
                if (!WpdIsOperationCancelled(confirmHr) && !WpdReportedError)
                {
                    WpdShowOperationError(parent, operationId, file->Name, confirmHr);
                }
                ok = false;
                break;
            }

            hr = WpdDownloadObjectWide(context, device, item->GetObjectId(), targetWide);
            if (FAILED(hr))
            {
                FinishWpdBatchError(context, hr, file->Name);
                ok = false;
                break;
            }
            if (!copy)
            {
                hr = WpdDeleteObject(device, item->GetObjectId());
                if (FAILED(hr))
                {
                    FinishWpdBatchError(context, hr, file->Name);
                    ok = false;
                    break;
                }
            }
            if (context.stop || focused)
            {
                if (context.stop)
                {
                    ok = false;
                }
                break;
            }
        }

        progress.Close();
        if (opened && device != nullptr)
        {
            device->Close();
        }
        WpdDialogOwner = previousOwner;
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
    int sourceFiles,
    int sourceDirs,
    char* targetPath,
    BOOL* invalidPathOrCancel)
{
    UNREFERENCED_PARAMETER(sourceFiles);
    UNREFERENCED_PARAMETER(sourceDirs);
    if (invalidPathOrCancel != nullptr)
    {
        *invalidPathOrCancel = FALSE;
    }

    if (mode == 1)
    {
        SalamanderGeneral->SalPathAppend(targetPath, "*.*", 2 * MAX_PATH);
        return TRUE;
    }

    if (mode != 2 && mode != 3)
    {
        return FALSE;
    }

    HWND previousOwner = WpdDialogOwner;
    WpdDialogOwner = parent;
    WpdReportedError = false;
    UINT operationId = copy ? IDS_OPERATIONPROGRESS_COPY : IDS_OPERATIONPROGRESS_MOVE;

    // Show progress before the slow path walk and sibling lookup so the
    // overwrite dialog is not preceded by a silent freeze.
    CWpdStandardProgress progress;
    progress.Open(parent, WpdText(operationId));

    std::string userPart = WpdTargetUserPart(fsName, targetPath);
    CWpdDevice* targetDevice = nullptr;
    CFxString targetObjectId;
    HRESULT hr = GetContentLocationForPath(userPart.c_str(), targetDevice, targetObjectId);
    if (FAILED(hr) || targetDevice == nullptr)
    {
        progress.Close();
        if (targetDevice != nullptr)
        {
            targetDevice->Release();
        }
        WpdDialogOwner = previousOwner;
        HRESULT reportHr = FAILED(hr) ? hr : E_FAIL;
        if (!HandleDeviceReconnectRequired(parent, reportHr, nullptr))
        {
            WpdShowOperationError(parent, operationId, targetPath, reportHr);
        }
        if (invalidPathOrCancel != nullptr)
        {
            *invalidPathOrCancel = TRUE;
        }
        return TRUE;
    }

    progress.BeginItem(WpdOperationTitle(operationId, targetDevice).c_str(), static_cast<ULONGLONG>(-1));
    ATL::CA2W wideTarget(targetObjectId);
    WpdCopyContext context;
    context.parent = parent;
    context.operationId = operationId;
    context.progress = &progress;
    context.targetDevice = targetDevice;

    struct WpdDiskUploadItem
    {
        std::string name;
        BOOL isDir;
        ULONGLONG size;
        FILETIME lastWrite;
    };

    bool ok = true;
    bool opened = false;
    hr = targetDevice->Open(GENERIC_READ | GENERIC_WRITE);
    if (FAILED(hr))
    {
        FinishWpdBatchError(context, hr, targetPath);
        ok = false;
    }

    const char* name = nullptr;
    const char* dosName = nullptr;
    BOOL isDir = FALSE;
    CQuadWord size;
    DWORD attr = 0;
    FILETIME lastWrite;
    int errorOccured = SALENUM_SUCCESS;
    std::vector<WpdDiskUploadItem> items;
    if (ok)
    {
        opened = true;
        // Collect the whole selection and its byte total first. Adding sizes
        // while uploading made Total % run far ahead of File %.
        while ((name = next(parent, 0, &dosName, &isDir, &size, &attr, &lastWrite, nextParam, &errorOccured)) != nullptr)
        {
            WpdDiskUploadItem item;
            item.name = name;
            item.isDir = isDir;
            item.size = size.Value;
            item.lastWrite = lastWrite;
            items.push_back(item);

            std::string sourceJoined;
            hr = WpdJoinSalPath(sourcePath, name, sourceJoined);
            if (FAILED(hr))
            {
                WpdShowOperationError(parent, operationId, name, hr);
                ok = false;
                break;
            }
            std::wstring sourceWide;
            hr = WpdPanelTextToWide(sourceJoined.c_str(), sourceWide);
            if (FAILED(hr))
            {
                WpdShowOperationError(parent, operationId, name, hr);
                ok = false;
                break;
            }
            if (isDir)
            {
                progress.AddTotal(WpdDiskTreeBytes(sourceWide));
            }
            else
            {
                progress.AddTotal(size.Value);
            }
        }
    }

    if (ok)
    {
        progress.CommitTotals();
        for (size_t i = 0; i < items.size(); ++i)
        {
            const WpdDiskUploadItem& item = items[i];
            std::string sourceJoined;
            hr = WpdJoinSalPath(sourcePath, item.name.c_str(), sourceJoined);
            if (FAILED(hr))
            {
                WpdShowOperationError(parent, operationId, item.name.c_str(), hr);
                ok = false;
                break;
            }

            std::wstring sourceWide;
            hr = WpdPanelTextToWide(sourceJoined.c_str(), sourceWide);
            const char* leaf = strrchr(item.name.c_str(), '\\');
            leaf = leaf != nullptr ? leaf + 1 : item.name.c_str();
            std::wstring leafWide;
            if (SUCCEEDED(hr))
            {
                hr = WpdPanelTextToWide(leaf, leafWide);
            }
            if (FAILED(hr))
            {
                WpdShowOperationError(parent, operationId, item.name.c_str(), hr);
                ok = false;
                break;
            }

            WpdOutcome outcome = WpdUploadDisk(context, wideTarget, sourceWide, leafWide.c_str(), leaf, item.name.c_str(),
                                               item.size, !item.isDir, &item.lastWrite, TRUE);
            if (outcome.Failed())
            {
                FinishWpdBatchError(context, outcome.hr, item.name.c_str());
                ok = false;
                break;
            }
            if (!copy && outcome.Completed())
            {
                WpdDeleteDiskTree(sourceWide);
                if (::GetFileAttributesW(WpdForWin32(sourceWide).c_str()) != INVALID_FILE_ATTRIBUTES)
                {
                    hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
                    FinishWpdBatchError(context, hr, item.name.c_str());
                    ok = false;
                    break;
                }
            }
            if (context.stop)
            {
                ok = false;
                break;
            }
        }
    }

    if (errorOccured == SALENUM_CANCEL)
    {
        ok = FALSE;
    }

    progress.Close();
    if (opened)
    {
        targetDevice->Close();
    }
    targetDevice->Release();
    WpdDialogOwner = previousOwner;
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
