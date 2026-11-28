// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-28 | Restore the original icon column, responsive layout, hyperlinks, and dynamic row action. | Keep the native updates dialog visually and functionally aligned with its C# predecessor. | | openrouter/openai/gpt-6-luna-pro |
// | 2026-11-28 | Keep updater error dialogs fully localized. | Native Win32 error details can contain English API text. | | openrouter/openai/gpt-6-luna-pro |
// | 2026-11-28 | Implement localized native settings, catalog, and notification dialogs. | Replace the managed WinForms updater UI. | | openrouter/openai/gpt-6-luna-pro |

#include "precomp.h"
#include "native_updater.h"
#include "native_installer.h"
#include "native_icons.h"
#include "native_utils.h"
#include "../../darkmode.h"

#include <algorithm>
#include <map>
#include <memory>
#include <new>
#include <set>
#include <shellapi.h>
#include <utility>
#include <vector>
#include <windowsx.h>

namespace SamandarinNative
{
namespace
{
constexpr UINT WM_SAMANDARIN_CHECK_COMPLETE = WM_APP + 0x450;
constexpr UINT WM_SAMANDARIN_CATALOG_COMPLETE = WM_APP + 0x451;
constexpr UINT WM_SAMANDARIN_INSTALL_COMPLETE = WM_APP + 0x452;
constexpr UINT WM_SAMANDARIN_CONFIG_REFRESH = WM_APP + 0x453;
constexpr UINT WM_SAMANDARIN_CATALOG_ICONS_COMPLETE = WM_APP + 0x454;
const wchar_t* const kMessageWindowClass = L"OpenSamandarin.NativeUpdater.MessageWindow";

constexpr int kColorItemForeground = 6;
constexpr int kColorItemBackground = 11;

struct MessageDialogParameters
{
    HWND Owner = nullptr;
    std::wstring Caption;
    std::wstring Text;
    std::wstring Affirmative;
    std::wstring Negative;
    HICON Icon = nullptr;
    bool Confirm = false;
};

struct ConfigurationParameters
{
    NativeUpdateSettings Settings = {};
    std::wstring CurrentVersion;
    bool Saved = false;
};

struct CaseInsensitiveStringLess
{
    bool operator()(const std::wstring& left, const std::wstring& right) const
    {
        return CompareStringOrdinal(left.c_str(), static_cast<int>(left.size()),
                                    right.c_str(), static_cast<int>(right.size()), TRUE) == CSTR_LESS_THAN;
    }
};

struct UpdatesDialogState
{
    HWND Window = nullptr;
    HWND Parent = nullptr;
    std::vector<PluginUpdateRow> Rows;
    std::vector<CatalogIconImage> CatalogImages;
    std::vector<size_t> VisibleRows;
    std::map<std::wstring, int, CaseInsensitiveStringLess> ImageIndexes;
    HIMAGELIST ImageList = nullptr;
    HFONT LinkFont = nullptr;
    int DefaultImageIndex = -1;
    int SortColumn = 2;
    bool SortDescending = false;
    bool RefreshInProgress = false;
    bool InstallInProgress = false;
    bool ShowOnlyUpdates = false;
    int ContextRow = -1;
    int ContextColumn = -1;
    std::wstring PreferredSelectionId;
    std::wstring PreferredSelectionName;
};

struct SourcesDialogState
{
    std::vector<CatalogSource> Sources;
    bool Saved = false;
};

HWND gMessageWindow = nullptr;
HWND gConfigurationWindow = nullptr;
HWND gUpdatesWindow = nullptr;
HBRUSH gDialogBrush = nullptr;
COLORREF gConfiguredBackground = CLR_INVALID;
COLORREF gConfiguredForeground = CLR_INVALID;
bool gConfiguredDark = false;

template <typename T, typename... Args>
T* NewNoThrow(Args&&... args)
{
#if defined(_DEBUG) && defined(_MSC_VER)
#undef new
#define SAMANDARIN_RESTORE_DEBUG_NEW
#endif
    T* result = new (std::nothrow) T(std::forward<Args>(args)...);
#ifdef SAMANDARIN_RESTORE_DEBUG_NEW
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef SAMANDARIN_RESTORE_DEBUG_NEW
#endif
    return result;
}

std::wstring String(int id)
{
    return GetLocalizedString(id);
}

bool EqualsIgnoreCase(const std::wstring& left, const std::wstring& right)
{
    return CompareStringOrdinal(left.c_str(), static_cast<int>(left.size()),
                                right.c_str(), static_cast<int>(right.size()), TRUE) == CSTR_EQUAL;
}

std::wstring Lower(std::wstring value)
{
    for (wchar_t& c : value)
        if (c >= L'A' && c <= L'Z') c = static_cast<wchar_t>(c - L'A' + L'a');
    return value;
}

std::wstring Trim(std::wstring value)
{
    const size_t first = value.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) return std::wstring();
    return value.substr(first, value.find_last_not_of(L" \t\r\n") - first + 1);
}

void SetText(HWND dialog, int control, const std::wstring& text)
{
    SetDlgItemTextW(dialog, control, text.c_str());
}

bool IsDarkColor(COLORREF color)
{
    const unsigned brightness = GetRValue(color) + GetGValue(color) + GetBValue(color);
    return brightness < 3 * 128;
}

void ConfigureDarkMode()
{
    COLORREF background = GetSysColor(COLOR_BTNFACE);
    COLORREF foreground = GetSysColor(COLOR_BTNTEXT);
    if (SalamanderGeneral != nullptr)
    {
        background = SalamanderGeneral->GetCurrentColor(kColorItemBackground);
        foreground = SalamanderGeneral->GetCurrentColor(kColorItemForeground);
    }
    const bool dark = IsDarkColor(background);
    const COLORREF fallbackText = GetSysColor(COLOR_BTNTEXT);
    const COLORREF fallbackBackground = GetSysColor(COLOR_BTNFACE);
    const COLORREF readableText = DarkModeEnsureReadableForeground(foreground, background);
    if (gDialogBrush != nullptr && gConfiguredBackground == background &&
        gConfiguredForeground == foreground && gConfiguredDark == dark)
    {
        DarkModeSetEnabled(dark);
        return;
    }
    HBRUSH brush = CreateSolidBrush(background);
    DarkModeSetConfiguredColors(foreground, background, fallbackText, fallbackBackground);
    DarkModeConfigureDialogColors(readableText, background,
                                  brush != nullptr ? brush : GetSysColorBrush(COLOR_BTNFACE));
    DarkModeSetEnabled(dark);
    if (gDialogBrush != nullptr) DeleteObject(gDialogBrush);
    gDialogBrush = brush;
    gConfiguredBackground = background;
    gConfiguredForeground = foreground;
    gConfiguredDark = dark;
}

void ApplyDarkMode(HWND window)
{
    if (window == nullptr) return;
    ConfigureDarkMode();
    DarkModeApplyWindow(window);
    DarkModeRefreshTitleBar(window);
    DarkModeAllowDarkScrollbars(window);
    DarkModeApplyTree(window);
}

bool HandleDarkColor(HWND window, UINT message, WPARAM wParam, LPARAM lParam, INT_PTR* result)
{
    if (!DarkModeShouldUseDarkColors()) return false;
    ConfigureDarkMode();
    LRESULT brush = 0;
    if (!DarkModeHandleCtlColor(message, wParam, lParam, brush)) return false;
    *result = static_cast<INT_PTR>(brush);
    (void)window;
    return true;
}

void CenterDialog(HWND window, HWND owner)
{
    RECT dialog = {};
    RECT parent = {};
    if (!GetWindowRect(window, &dialog)) return;
    HMONITOR monitor = MonitorFromWindow(owner != nullptr ? owner : window, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info = {sizeof(info)};
    if (owner != nullptr && GetWindowRect(owner, &parent) && GetMonitorInfoW(monitor, &info))
    {
        int x = parent.left + ((parent.right - parent.left) - (dialog.right - dialog.left)) / 2;
        int y = parent.top + ((parent.bottom - parent.top) - (dialog.bottom - dialog.top)) / 2;
        x = (std::max)(static_cast<int>(info.rcWork.left),
                       (std::min)(x, static_cast<int>(info.rcWork.right - (dialog.right - dialog.left))));
        y = (std::max)(static_cast<int>(info.rcWork.top),
                       (std::min)(y, static_cast<int>(info.rcWork.bottom - (dialog.bottom - dialog.top))));
        SetWindowPos(window, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
    else if (GetMonitorInfoW(monitor, &info))
    {
        const int width = dialog.right - dialog.left;
        const int height = dialog.bottom - dialog.top;
        SetWindowPos(window, nullptr, info.rcWork.left + ((info.rcWork.right - info.rcWork.left) - width) / 2,
                     info.rcWork.top + ((info.rcWork.bottom - info.rcWork.top) - height) / 2,
                     0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

INT_PTR CALLBACK MessageDialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto* parameters = reinterpret_cast<MessageDialogParameters*>(GetWindowLongPtrW(dialog, GWLP_USERDATA));
    if (message == WM_INITDIALOG)
    {
        parameters = reinterpret_cast<MessageDialogParameters*>(lParam);
        SetWindowLongPtrW(dialog, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(parameters));
        SetWindowTextW(dialog, parameters->Caption.c_str());
        SetText(dialog, IDC_MESSAGE_TEXT, parameters->Text);
        SetText(dialog, IDOK, parameters->Affirmative);
        SetText(dialog, IDCANCEL, parameters->Negative);
        ShowWindow(GetDlgItem(dialog, IDCANCEL), parameters->Confirm ? SW_SHOW : SW_HIDE);
        if (parameters->Icon != nullptr)
        {
            SendDlgItemMessageW(dialog, IDC_MESSAGE_ICON, STM_SETICON, reinterpret_cast<WPARAM>(parameters->Icon), 0);
            SetWindowPos(GetDlgItem(dialog, IDC_MESSAGE_TEXT), nullptr, 40, 10, 250, 58,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
        else
        {
            ShowWindow(GetDlgItem(dialog, IDC_MESSAGE_ICON), SW_HIDE);
            SetWindowPos(GetDlgItem(dialog, IDC_MESSAGE_TEXT), nullptr, 10, 10, 280, 58,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
        if (!parameters->Confirm)
        {
            RECT client = {};
            GetClientRect(dialog, &client);
            SetWindowPos(GetDlgItem(dialog, IDOK), nullptr, (client.right - 50) / 2, 78, 50, 14,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
        CenterDialog(dialog, parameters->Owner);
        ApplyDarkMode(dialog);
        return TRUE;
    }
    if (message == WM_COMMAND)
    {
        if (LOWORD(wParam) == IDOK)
        {
            EndDialog(dialog, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(dialog, IDCANCEL);
            return TRUE;
        }
    }
    if (message == WM_CLOSE)
    {
        EndDialog(dialog, IDCANCEL);
        return TRUE;
    }
    if (message == WM_THEMECHANGED || message == WM_SETTINGCHANGE)
    {
        ApplyDarkMode(dialog);
        InvalidateRect(dialog, nullptr, TRUE);
    }
    INT_PTR colorResult = 0;
    if (message >= WM_CTLCOLORMSGBOX && message <= WM_CTLCOLORSTATIC &&
        HandleDarkColor(dialog, message, wParam, lParam, &colorResult))
        return colorResult;
    return FALSE;
}

int ShowNativeMessage(HWND owner, const std::wstring& caption, const std::wstring& text,
                      UINT icon, bool confirm, int affirmativeId = IDS_OK,
                      int negativeId = IDS_CANCEL)
{
    MessageDialogParameters parameters;
    parameters.Owner = owner;
    parameters.Caption = caption;
    parameters.Text = text;
    parameters.Affirmative = String(affirmativeId);
    parameters.Negative = String(negativeId);
    parameters.Confirm = confirm;
    if (icon == MB_ICONERROR) parameters.Icon = LoadIconW(nullptr, MAKEINTRESOURCEW(32513));
    else if (icon == MB_ICONWARNING || icon == MB_ICONEXCLAMATION) parameters.Icon = LoadIconW(nullptr, MAKEINTRESOURCEW(32515));
    else if (icon == MB_ICONINFORMATION) parameters.Icon = LoadIconW(nullptr, MAKEINTRESOURCEW(32516));
    const INT_PTR result = DialogBoxParamW(DLLInstance, MAKEINTRESOURCEW(IDD_SAMANDARIN_MESSAGE),
                                           owner, MessageDialogProc, reinterpret_cast<LPARAM>(&parameters));
    return result == -1 ? IDCANCEL : static_cast<int>(result);
}

HWND ResolveOwner(HWND requested)
{
    HWND foreground = GetForegroundWindow();
    if (foreground != nullptr)
    {
        HWND root = GetAncestor(foreground, GA_ROOT);
        if (root != nullptr) foreground = root;
        DWORD processId = 0;
        GetWindowThreadProcessId(foreground, &processId);
        if (processId == GetCurrentProcessId() && IsWindowVisible(foreground) && !IsIconic(foreground))
            return foreground;
    }
    return requested;
}

void UpdateConfigurationLabels(HWND dialog, const ConfigurationParameters& parameters)
{
    SetText(dialog, IDC_CONFIG_CURRENT_VERSION,
            FormatLocalizedString(IDS_CURRENT_VERSION, {parameters.CurrentVersion.empty() ? String(IDS_UNKNOWN) : parameters.CurrentVersion}));
    if (parameters.Settings.HasLastCheckUtc)
    {
        constexpr LONGLONG kDotNetToFileTimeTicks = 504911232000000000LL;
        const LONGLONG fileTicks = parameters.Settings.LastCheckUtcTicks - kDotNetToFileTimeTicks;
        if (fileTicks >= 0)
        {
            ULARGE_INTEGER timestamp = {};
            timestamp.QuadPart = static_cast<ULONGLONG>(fileTicks);
            FILETIME utc = {timestamp.LowPart, timestamp.HighPart};
            FILETIME local = {};
            SYSTEMTIME systemTime = {};
            if (FileTimeToLocalFileTime(&utc, &local) && FileTimeToSystemTime(&local, &systemTime))
            {
                wchar_t date[80] = {};
                wchar_t time[80] = {};
                GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systemTime, nullptr, date, _countof(date));
                GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &systemTime, nullptr, time, _countof(time));
                SetText(dialog, IDC_CONFIG_LAST_CHECK,
                        FormatLocalizedString(IDS_LAST_AUTO_CHECK, {std::wstring(date) + L" " + time}));
            }
            else
            {
                SetText(dialog, IDC_CONFIG_LAST_CHECK, String(IDS_LAST_AUTO_CHECK_NEVER));
            }
        }
        else
        {
            SetText(dialog, IDC_CONFIG_LAST_CHECK, String(IDS_LAST_AUTO_CHECK_NEVER));
        }
    }
    else
    {
        SetText(dialog, IDC_CONFIG_LAST_CHECK, String(IDS_LAST_AUTO_CHECK_NEVER));
    }
    SetText(dialog, IDC_CONFIG_LATEST_VERSION,
            parameters.Settings.LastKnownRemoteVersion[0] == '\0'
                ? String(IDS_LAST_KNOWN_RELEASE_UNKNOWN)
                : FormatLocalizedString(IDS_LAST_KNOWN_RELEASE,
                                        {AnsiOrUtf8ToWide(parameters.Settings.LastKnownRemoteVersion)}));
}

INT_PTR CALLBACK ConfigurationDialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto* parameters = reinterpret_cast<ConfigurationParameters*>(GetWindowLongPtrW(dialog, GWLP_USERDATA));
    if (message == WM_INITDIALOG)
    {
        parameters = reinterpret_cast<ConfigurationParameters*>(lParam);
        SetWindowLongPtrW(dialog, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(parameters));
        gConfigurationWindow = dialog;
        SetWindowTextW(dialog, String(IDS_CONFIG_TITLE).c_str());
        SetText(dialog, IDC_CONFIG_DESCRIPTION, String(IDS_CONFIG_DESCRIPTION));
        SetText(dialog, IDC_CONFIG_CHECK_STARTUP, String(IDS_CHECK_ON_STARTUP));
        SetText(dialog, IDC_CONFIG_FREQUENCY_LABEL, String(IDS_PERIODIC_CHECK));
        SetText(dialog, IDOK, String(IDS_OK));
        SetText(dialog, IDCANCEL, String(IDS_CANCEL));
        SetText(dialog, IDC_CONFIG_CHECK_NOW, String(IDS_CHECK_NOW));
        HWND frequency = GetDlgItem(dialog, IDC_CONFIG_FREQUENCY);
        for (int id : {IDS_FREQ_DISABLED, IDS_FREQ_DAILY, IDS_FREQ_WEEKLY, IDS_FREQ_MONTHLY})
            SendMessageW(frequency, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(String(id).c_str()));
        SendMessageW(frequency, CB_SETCURSEL,
                     static_cast<WPARAM>((parameters->Settings.Frequency >= 0 && parameters->Settings.Frequency <= 3)
                                             ? parameters->Settings.Frequency
                                             : static_cast<int>(NativeUpdateFrequency::Weekly)), 0);
        CheckDlgButton(dialog, IDC_CONFIG_CHECK_STARTUP,
                       parameters->Settings.CheckOnStartup ? BST_CHECKED : BST_UNCHECKED);
        UpdateConfigurationLabels(dialog, *parameters);
        ApplyDarkMode(dialog);
        CenterDialog(dialog, GetParent(dialog));
        return TRUE;
    }
    if (message == WM_COMMAND && parameters != nullptr)
    {
        switch (LOWORD(wParam))
        {
        case IDC_CONFIG_CHECK_NOW:
            SetText(dialog, IDC_CONFIG_STATUS, String(IDS_STATUS_CHECKING));
            EnableWindow(GetDlgItem(dialog, IDC_CONFIG_CHECK_NOW), FALSE);
            if (!NativeUpdater_CheckNow(dialog))
            {
                EnableWindow(GetDlgItem(dialog, IDC_CONFIG_CHECK_NOW), TRUE);
                SetText(dialog, IDC_CONFIG_STATUS, String(IDS_PERFORM_CHECK_ERROR));
            }
            return TRUE;
        case IDOK:
            parameters->Settings.CheckOnStartup = IsDlgButtonChecked(dialog, IDC_CONFIG_CHECK_STARTUP) == BST_CHECKED;
            parameters->Settings.Frequency = static_cast<int>(SendDlgItemMessageW(dialog, IDC_CONFIG_FREQUENCY, CB_GETCURSEL, 0, 0));
            if (!Samandarin_SaveSettings(&parameters->Settings))
            {
                ShowNativeMessage(dialog, String(IDS_PLUGINNAME), String(IDS_PERFORM_CHECK_ERROR), MB_ICONERROR, false);
                return TRUE;
            }
            parameters->Saved = true;
            EndDialog(dialog, IDOK);
            return TRUE;
        case IDCANCEL:
            EndDialog(dialog, IDCANCEL);
            return TRUE;
        }
    }
    if (message == WM_SAMANDARIN_CONFIG_REFRESH && parameters != nullptr)
    {
        Samandarin_LoadSettings(&parameters->Settings);
        UpdateConfigurationLabels(dialog, *parameters);
        SetText(dialog, IDC_CONFIG_STATUS, String(IDS_STATUS_COMPLETED));
        EnableWindow(GetDlgItem(dialog, IDC_CONFIG_CHECK_NOW), TRUE);
        return TRUE;
    }
    if (message == WM_CLOSE)
    {
        EndDialog(dialog, IDCANCEL);
        return TRUE;
    }
    if (message == WM_THEMECHANGED || message == WM_SETTINGCHANGE) ApplyDarkMode(dialog);
    INT_PTR colorResult = 0;
    if (message >= WM_CTLCOLORMSGBOX && message <= WM_CTLCOLORSTATIC &&
        HandleDarkColor(dialog, message, wParam, lParam, &colorResult))
        return colorResult;
    if (message == WM_NCDESTROY && gConfigurationWindow == dialog) gConfigurationWindow = nullptr;
    return FALSE;
}

std::wstring FormatCapability(const std::wstring& value, int label)
{
    std::wstring localized;
    if (value.empty()) localized = String(IDS_PLUGIN_SECURITY_UNKNOWN);
    else if (EqualsIgnoreCase(value, L"yes")) localized = String(IDS_PLUGIN_SECURITY_YES);
    else if (EqualsIgnoreCase(value, L"no")) localized = String(IDS_PLUGIN_SECURITY_NO);
    else if (EqualsIgnoreCase(value, L"possible")) localized = String(IDS_PLUGIN_SECURITY_POSSIBLE);
    else if (label == IDS_PLUGIN_SECURITY_ELEVATION && EqualsIgnoreCase(value, L"install"))
        localized = String(IDS_PLUGIN_SECURITY_ELEVATION_INSTALL);
    else if (label == IDS_PLUGIN_SECURITY_ELEVATION && EqualsIgnoreCase(value, L"never"))
        localized = String(IDS_PLUGIN_SECURITY_ELEVATION_NEVER);
    else
        localized = value;
    return String(label) + L": " + localized;
}

std::wstring BuildSecurityDetails(const PluginUpdateRow& row)
{
    std::vector<std::wstring> lines;
    lines.push_back(FormatCapability(row.Security.NetworkAccess, IDS_PLUGIN_SECURITY_NETWORK));
    lines.push_back(FormatCapability(row.Security.ExternalProcesses, IDS_PLUGIN_SECURITY_PROCESSES));
    lines.push_back(FormatCapability(row.Security.ScriptExecution, IDS_PLUGIN_SECURITY_SCRIPT));
    lines.push_back(FormatCapability(row.Security.ActiveWebContent, IDS_PLUGIN_SECURITY_WEB));
    lines.push_back(FormatCapability(row.Security.Elevation, IDS_PLUGIN_SECURITY_ELEVATION));
    lines.push_back(String(IDS_PLUGIN_SECURITY_SOURCE) + L": " +
                    (row.WebUrl.empty() ? String(IDS_PLUGIN_SECURITY_UNKNOWN) : row.WebUrl));
    InstallReceipt receipt;
    const PackageKind kind = row.CatalogKind != PackageKind::Unknown ? row.CatalogKind : row.InstalledKind;
    const bool hasReceipt = GetInstallReceipt(row.Id, kind, &receipt);
    lines.push_back(String(IDS_PLUGIN_SECURITY_SIGNER) + L": " +
                    (hasReceipt && !receipt.Signer.empty() ? receipt.Signer : String(IDS_PLUGIN_SECURITY_UNKNOWN)));
    const std::wstring hash = !row.PackageSha256.empty() ? row.PackageSha256 :
                              hasReceipt ? receipt.PackageSha256 : std::wstring();
    lines.push_back(String(IDS_PLUGIN_SECURITY_HASH) + L": " +
                    (hash.empty() ? String(IDS_PLUGIN_SECURITY_UNKNOWN) : hash));
    lines.push_back(CanInstallPluginRow(row) ? String(IDS_PLUGIN_SECURITY_VERIFIED)
                                             : String(IDS_PLUGIN_SECURITY_UNVERIFIED));
    std::wstring text;
    for (const std::wstring& line : lines)
    {
        if (!text.empty()) text.append(L"\r\n");
        text += line;
    }
    return text;
}

std::wstring ValueForColumn(const PluginUpdateRow& row, int column)
{
    switch (column)
    {
    case 0: return std::wstring();
    case 1: return row.Source;
    case 2: return row.Name;
    case 3: return row.InstalledVersion;
    case 4: return row.LatestVersion;
    case 5: return row.StatusText;
    case 6: return row.Author;
    default: return row.Name;
    }
}

std::wstring SortValueForColumn(const PluginUpdateRow& row, int column)
{
    return column == 0 ? row.Name : ValueForColumn(row, column);
}

int ScaleLogical(HWND window, int value)
{
    HDC dc = GetDC(window);
    const int dpi = dc != nullptr ? GetDeviceCaps(dc, LOGPIXELSX) : 96;
    if (dc != nullptr) ReleaseDC(window, dc);
    return MulDiv(value, dpi > 0 ? dpi : 96, 96);
}

void SetDialogItemRect(HWND dialog, int control, int x, int y, int width, int height)
{
    RECT rect = {x, y, x + width, y + height};
    if (!MapDialogRect(dialog, &rect)) return;
    SetWindowPos(GetDlgItem(dialog, control), nullptr, rect.left, rect.top,
                 rect.right - rect.left, rect.bottom - rect.top,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

void LayoutUpdatesDialog(HWND dialog)
{
    RECT client = {};
    RECT base = {0, 0, 650, 415};
    if (!GetClientRect(dialog, &client) || !MapDialogRect(dialog, &base) ||
        base.right <= 0 || base.bottom <= 0)
        return;
    const int width = MulDiv(client.right, 650, base.right);
    const int height = MulDiv(client.bottom, 415, base.bottom);
    if (width < 40 || height < 120) return;

    constexpr int listTop = 30;
    constexpr int checkboxHeight = 12;
    constexpr int listCheckboxGap = 11;
    constexpr int checkboxGroupGap = 10;
    const int groupBottom = (std::max)(listTop + 100, height - 40);
    const int available = (std::max)(100, groupBottom - listTop - listCheckboxGap -
                                                checkboxHeight - checkboxGroupGap);
    const int groupHeight = (available * 42 + 50) / 100;
    const int listHeight = available - groupHeight;
    const int checkboxY = listTop + listHeight + listCheckboxGap;
    const int groupY = checkboxY + checkboxHeight + checkboxGroupGap;
    const int contentWidth = (std::max)(1, width - 20);

    SetDialogItemRect(dialog, IDC_UPDATES_DESCRIPTION, 10, 8, contentWidth, 20);
    SetDialogItemRect(dialog, IDC_UPDATES_LIST, 10, listTop, contentWidth, listHeight);
    SetDialogItemRect(dialog, IDC_UPDATES_ONLY, 10, checkboxY, 250, checkboxHeight);
    SetDialogItemRect(dialog, IDC_UPDATES_DETAIL_GROUP, 10, groupY, contentWidth, groupHeight);

    const int leftValueX = 82;
    const int rightLabelX = width / 2 - 15;
    const int rightValueX = width / 2 + 49;
    const int leftValueWidth = (std::max)(1, rightLabelX - 18 - leftValueX);
    const int rightValueWidth = (std::max)(1, width - 26 - rightValueX);
    const int rowY[] = {groupY + 14, groupY + 30, groupY + 46, groupY + 62};
    const int leftLabels[] = {IDC_UPDATES_DETAIL_NAME_LABEL, IDC_UPDATES_DETAIL_AUTHOR_LABEL,
                              IDC_UPDATES_DETAIL_INSTALLED_LABEL, IDC_UPDATES_DETAIL_HOMEPAGE_LABEL};
    const int leftValues[] = {IDC_UPDATES_DETAIL_NAME, IDC_UPDATES_DETAIL_AUTHOR,
                              IDC_UPDATES_DETAIL_INSTALLED, IDC_UPDATES_DETAIL_HOMEPAGE};
    const int rightLabels[] = {IDC_UPDATES_DETAIL_SOURCE_LABEL, IDC_UPDATES_DETAIL_STATUS_LABEL,
                               IDC_UPDATES_DETAIL_LATEST_LABEL, IDC_UPDATES_DETAIL_DOWNLOAD_LABEL};
    const int rightValues[] = {IDC_UPDATES_DETAIL_SOURCE, IDC_UPDATES_DETAIL_STATUS,
                               IDC_UPDATES_DETAIL_LATEST, IDC_UPDATES_DETAIL_DOWNLOAD};
    for (int index = 0; index < 4; ++index)
    {
        SetDialogItemRect(dialog, leftLabels[index], 18, rowY[index], 62, 12);
        SetDialogItemRect(dialog, leftValues[index], leftValueX, rowY[index], leftValueWidth, 12);
        SetDialogItemRect(dialog, rightLabels[index], rightLabelX, rowY[index], 62, 12);
        SetDialogItemRect(dialog, rightValues[index], rightValueX, rowY[index], rightValueWidth, 12);
    }

    const int descriptionTop = groupY + 76;
    const int textBottom = groupHeight - 10;
    const int textGap = 3;
    const int textHeight = (std::max)(16, (textBottom - 76 - textGap) / 2);
    const int securityTop = descriptionTop + textHeight + textGap;
    const int securityHeight = (std::max)(16, textBottom - (securityTop - groupY));
    const int detailTextWidth = (std::max)(1, width - 108);
    SetDialogItemRect(dialog, IDC_UPDATES_DESCRIPTION_LABEL, 18, descriptionTop + 3, 62, 12);
    SetDialogItemRect(dialog, IDC_UPDATES_DESCRIPTION_TEXT, leftValueX, descriptionTop,
                      detailTextWidth, textHeight);
    SetDialogItemRect(dialog, IDC_UPDATES_SECURITY_LABEL, 18, securityTop + 3, 62, 12);
    SetDialogItemRect(dialog, IDC_UPDATES_SECURITY_TEXT, leftValueX, securityTop,
                      detailTextWidth, securityHeight);

    SetDialogItemRect(dialog, IDC_UPDATES_STATUS, 10, height - 36,
                      (std::max)(100, width / 3), 22);
    const int buttonY = height - 33;
    constexpr int buttonGap = 3;
    constexpr int installWidth = 77;
    constexpr int sourcesWidth = 77;
    constexpr int refreshWidth = 50;
    constexpr int closeWidth = 50;
    const int closeX = width - 10 - closeWidth;
    const int refreshX = closeX - buttonGap - refreshWidth;
    const int sourcesX = refreshX - buttonGap - sourcesWidth;
    const int installX = sourcesX - buttonGap - installWidth;
    SetDialogItemRect(dialog, IDC_UPDATES_INSTALL, installX, buttonY, installWidth, 18);
    SetDialogItemRect(dialog, IDC_UPDATES_SOURCES, sourcesX, buttonY, sourcesWidth, 18);
    SetDialogItemRect(dialog, IDC_UPDATES_REFRESH, refreshX, buttonY, refreshWidth, 18);
    SetDialogItemRect(dialog, IDCANCEL, closeX, buttonY, closeWidth, 18);
}

void SetListViewColumns(HWND list)
{
    const int ids[] = {0, IDS_PLUGIN_COL_SOURCE, IDS_PLUGIN_COL_NAME, IDS_PLUGIN_COL_INSTALLED,
                       IDS_PLUGIN_COL_LATEST, IDS_PLUGIN_COL_STATUS, IDS_PLUGIN_COL_AUTHOR};
    const int widths[] = {46, 120, 180, 90, 90, 120, 110};
    for (int index = 0; index < _countof(ids); ++index)
    {
        LVCOLUMNW column = {};
        column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        std::wstring title = ids[index] == 0 ? std::wstring() : String(ids[index]);
        column.pszText = title.data();
        column.cx = ScaleLogical(list, widths[index]);
        column.iSubItem = index;
        SendMessageW(list, LVM_INSERTCOLUMNW, static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(&column));
    }
}

void AdjustListViewColumns(HWND list)
{
    constexpr int columnCount = 7;
    constexpr int logicalMinimums[columnCount] = {46, 120, 180, 90, 90, 120, 110};
    constexpr double extraWidthWeights[columnCount] = {0.0, 0.15, 0.30, 0.12, 0.12, 0.18, 0.13};
    RECT client = {};
    if (list == nullptr || ListView_GetHeader(list) == nullptr ||
        ListView_GetItemCount(list) < 0 || !GetClientRect(list, &client) || client.right <= 0)
        return;
    const int available = (std::max)(0, static_cast<int>(client.right) - ScaleLogical(list, 4));
    int minimums[columnCount] = {};
    int totalMinimum = 0;
    for (int index = 0; index < columnCount; ++index)
    {
        minimums[index] = ScaleLogical(list, logicalMinimums[index]);
        totalMinimum += minimums[index];
    }
    int widths[columnCount] = {};
    int assigned = 0;
    if (available <= totalMinimum)
    {
        for (int index = 0; index < columnCount; ++index)
        {
            widths[index] = index == columnCount - 1
                                ? available - assigned
                                : (totalMinimum == 0 ? 0 : MulDiv(minimums[index], available, totalMinimum));
            assigned += widths[index];
        }
    }
    else
    {
        const int extra = available - totalMinimum;
        for (int index = 0; index < columnCount; ++index)
        {
            widths[index] = index == columnCount - 1
                                ? available - assigned
                                : minimums[index] + static_cast<int>(extra * extraWidthWeights[index]);
            assigned += widths[index];
        }
    }
    for (int index = 0; index < columnCount; ++index)
        ListView_SetColumnWidth(list, index, (std::max)(0, widths[index]));
}

void SetSortArrow(HWND list, int sortColumn, bool descending)
{
    HWND header = ListView_GetHeader(list);
    if (header == nullptr) return;
    for (int index = 0; index < 7; ++index)
    {
        HDITEMW item = {};
        item.mask = HDI_FORMAT;
        if (!SendMessageW(header, HDM_GETITEMW, static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(&item))) continue;
        item.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
        if (index == sortColumn) item.fmt |= descending ? HDF_SORTDOWN : HDF_SORTUP;
        SendMessageW(header, HDM_SETITEMW, static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(&item));
    }
}

void UpdateInstallButton(HWND dialog, const UpdatesDialogState& state)
{
    const int selected = ListView_GetNextItem(GetDlgItem(dialog, IDC_UPDATES_LIST), -1, LVNI_SELECTED);
    if (selected < 0 || static_cast<size_t>(selected) >= state.VisibleRows.size())
    {
        SetText(dialog, IDC_UPDATES_INSTALL, String(IDS_PLUGIN_UPDATES_OPEN_PAGE));
        EnableWindow(GetDlgItem(dialog, IDC_UPDATES_INSTALL), FALSE);
        return;
    }
    const PluginUpdateRow& row = state.Rows[state.VisibleRows[static_cast<size_t>(selected)]];
    const bool canInstall = CanInstallPluginRow(row);
    const bool canOpenPage = !row.WebUrl.empty() || !row.Homepage.empty();
    SetText(dialog, IDC_UPDATES_INSTALL,
            canInstall ? (row.InstallDirectory.empty() ? String(IDS_PLUGIN_INSTALL) : String(IDS_PLUGIN_UPDATE))
                       : String(IDS_PLUGIN_UPDATES_OPEN_PAGE));
    EnableWindow(GetDlgItem(dialog, IDC_UPDATES_INSTALL),
                 !state.RefreshInProgress && !state.InstallInProgress && (canInstall || canOpenPage));
}

void UpdateSelectedDetails(HWND dialog, UpdatesDialogState* state)
{
    if (state == nullptr) return;
    const int selected = ListView_GetNextItem(GetDlgItem(dialog, IDC_UPDATES_LIST), -1, LVNI_SELECTED);
    if (selected < 0 || static_cast<size_t>(selected) >= state->VisibleRows.size())
    {
        for (int control : {IDC_UPDATES_DETAIL_NAME, IDC_UPDATES_DETAIL_SOURCE, IDC_UPDATES_DETAIL_AUTHOR,
                            IDC_UPDATES_DETAIL_STATUS, IDC_UPDATES_DETAIL_INSTALLED, IDC_UPDATES_DETAIL_LATEST,
                            IDC_UPDATES_DETAIL_HOMEPAGE, IDC_UPDATES_DETAIL_DOWNLOAD,
                            IDC_UPDATES_DESCRIPTION_TEXT, IDC_UPDATES_SECURITY_TEXT})
            SetText(dialog, control, L"");
        UpdateInstallButton(dialog, *state);
        return;
    }
    const PluginUpdateRow& row = state->Rows[state->VisibleRows[static_cast<size_t>(selected)]];
    SetText(dialog, IDC_UPDATES_DETAIL_NAME, row.Name);
    SetText(dialog, IDC_UPDATES_DETAIL_SOURCE, row.Source);
    SetText(dialog, IDC_UPDATES_DETAIL_AUTHOR, row.Author);
    SetText(dialog, IDC_UPDATES_DETAIL_STATUS, row.StatusText);
    SetText(dialog, IDC_UPDATES_DETAIL_INSTALLED, row.InstalledVersion);
    SetText(dialog, IDC_UPDATES_DETAIL_LATEST, row.LatestVersion);
    SetText(dialog, IDC_UPDATES_DETAIL_HOMEPAGE, row.Homepage);
    SetText(dialog, IDC_UPDATES_DETAIL_DOWNLOAD, row.WebUrl);
    SetText(dialog, IDC_UPDATES_DESCRIPTION_TEXT, row.Description);
    SetText(dialog, IDC_UPDATES_SECURITY_TEXT, BuildSecurityDetails(row));
    UpdateInstallButton(dialog, *state);
}

int FindCachedImageIndex(const UpdatesDialogState& state, const std::wstring& key)
{
    if (key.empty()) return -1;
    const auto image = state.ImageIndexes.find(key);
    return image == state.ImageIndexes.end() ? -1 : image->second;
}

int PluginImageIndex(UpdatesDialogState* state, const PluginUpdateRow& row)
{
    if (state == nullptr) return -1;
    const std::wstring catalogReference = ResolveCatalogIconReference(row);
    int imageIndex = FindCachedImageIndex(*state, catalogReference);
    if (imageIndex >= 0) return imageIndex;

    if (!row.IconPath.empty())
    {
        const auto cached = state->ImageIndexes.find(row.IconPath);
        if (cached != state->ImageIndexes.end())
        {
            imageIndex = cached->second;
        }
        else
        {
            imageIndex = NativeIcons_AddImageFromPath(state->ImageList, row.IconPath);
            state->ImageIndexes.emplace(row.IconPath, imageIndex);
        }
        if (imageIndex >= 0) return imageIndex;
    }
    return state->DefaultImageIndex;
}

void RefreshCatalogImages(HWND dialog, UpdatesDialogState* state)
{
    if (state == nullptr) return;
    HWND list = GetDlgItem(dialog, IDC_UPDATES_LIST);
    for (size_t displayIndex = 0; displayIndex < state->VisibleRows.size(); ++displayIndex)
    {
        const size_t rowIndex = state->VisibleRows[displayIndex];
        if (rowIndex >= state->Rows.size()) continue;
        LVITEMW item = {};
        item.mask = LVIF_IMAGE;
        item.iItem = static_cast<int>(displayIndex);
        item.iImage = PluginImageIndex(state, state->Rows[rowIndex]);
        SendMessageW(list, LVM_SETITEMW, 0, reinterpret_cast<LPARAM>(&item));
    }
    InvalidateRect(list, nullptr, TRUE);
}

void BindRows(HWND dialog, UpdatesDialogState* state)
{
    if (state == nullptr) return;
    HWND list = GetDlgItem(dialog, IDC_UPDATES_LIST);
    const int previous = ListView_GetNextItem(list, -1, LVNI_SELECTED);
    size_t selectedRow = static_cast<size_t>(-1);
    if (!state->PreferredSelectionId.empty() || !state->PreferredSelectionName.empty())
    {
        if (!state->PreferredSelectionName.empty())
        {
            for (size_t index = 0; index < state->Rows.size(); ++index)
            {
                if (EqualsIgnoreCase(state->Rows[index].Name, state->PreferredSelectionName))
                {
                    selectedRow = index;
                    break;
                }
            }
        }
        if (selectedRow == static_cast<size_t>(-1) && !state->PreferredSelectionId.empty())
        {
            for (size_t index = 0; index < state->Rows.size(); ++index)
            {
                if (EqualsIgnoreCase(state->Rows[index].Id, state->PreferredSelectionId))
                {
                    selectedRow = index;
                    break;
                }
            }
        }
        state->PreferredSelectionId.clear();
        state->PreferredSelectionName.clear();
    }
    else if (previous >= 0 && static_cast<size_t>(previous) < state->VisibleRows.size())
        selectedRow = state->VisibleRows[static_cast<size_t>(previous)];

    state->VisibleRows.clear();
    for (size_t index = 0; index < state->Rows.size(); ++index)
    {
        if (!state->ShowOnlyUpdates || state->Rows[index].Status == PluginUpdateStatus::UpdateAvailable)
            state->VisibleRows.push_back(index);
    }
    std::stable_sort(state->VisibleRows.begin(), state->VisibleRows.end(), [&](size_t left, size_t right)
    {
        const std::wstring a = SortValueForColumn(state->Rows[left], state->SortColumn);
        const std::wstring b = SortValueForColumn(state->Rows[right], state->SortColumn);
        const int compare = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                                           a.c_str(), static_cast<int>(a.size()),
                                           b.c_str(), static_cast<int>(b.size()));
        return state->SortDescending ? compare == CSTR_GREATER_THAN : compare == CSTR_LESS_THAN;
    });

    SendMessageW(list, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(list);
    for (size_t displayIndex = 0; displayIndex < state->VisibleRows.size(); ++displayIndex)
    {
        const size_t rowIndex = state->VisibleRows[displayIndex];
        const PluginUpdateRow& row = state->Rows[rowIndex];
        const std::wstring first;
        LVITEMW item = {};
        item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
        item.iItem = static_cast<int>(displayIndex);
        item.pszText = const_cast<wchar_t*>(first.c_str());
        item.lParam = static_cast<LPARAM>(rowIndex);
        item.iImage = PluginImageIndex(state, row);
        const int inserted = static_cast<int>(SendMessageW(list, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item)));
        if (inserted < 0) continue;
        for (int column = 1; column < 7; ++column)
        {
            const std::wstring value = ValueForColumn(row, column);
            LVITEMW subItem = {};
            subItem.iSubItem = column;
            subItem.pszText = const_cast<wchar_t*>(value.c_str());
            SendMessageW(list, LVM_SETITEMTEXTW, static_cast<WPARAM>(inserted), reinterpret_cast<LPARAM>(&subItem));
        }
        if (rowIndex == selectedRow)
        {
            LVITEMW itemState = {};
            itemState.state = LVIS_SELECTED | LVIS_FOCUSED;
            itemState.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
            SendMessageW(list, LVM_SETITEMSTATE, static_cast<WPARAM>(inserted), reinterpret_cast<LPARAM>(&itemState));
        }
    }
    SetSortArrow(list, state->SortColumn, state->SortDescending);
    AdjustListViewColumns(list);
    SendMessageW(list, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(list, nullptr, TRUE);
    DarkModeUpdateListViewColors(list,
                                 SalamanderGeneral != nullptr ? SalamanderGeneral->GetCurrentColor(kColorItemForeground) : GetSysColor(COLOR_WINDOWTEXT),
                                 SalamanderGeneral != nullptr ? SalamanderGeneral->GetCurrentColor(kColorItemBackground) : GetSysColor(COLOR_WINDOW),
                                 true);
    UpdateSelectedDetails(dialog, state);
}

std::vector<PluginUpdateRow> ResolveInstallDependencies(const PluginUpdateRow& root,
                                                        const std::vector<PluginUpdateRow>& rows,
                                                        std::vector<std::wstring>* unresolved)
{
    std::set<std::wstring> installed;
    for (const PluginUpdateRow& row : rows)
    {
        if (!row.Id.empty() && !row.InstalledVersion.empty()) installed.insert(Lower(row.Id));
    }
    std::vector<PluginUpdateRow> missing;
    std::set<std::wstring> visited;
    std::set<std::wstring> visiting;
    auto visit = [&](auto&& self, const std::wstring& dependencyId) -> void
    {
        const std::wstring normalized = Lower(Trim(dependencyId));
        if (normalized.empty() || visited.find(normalized) != visited.end()) return;
        if (visiting.find(normalized) != visiting.end())
        {
            unresolved->push_back(dependencyId);
            return;
        }
        visiting.insert(normalized);
        const PluginUpdateRow* candidate = nullptr;
        for (const PluginUpdateRow& row : rows)
        {
            if (!EqualsIgnoreCase(row.Id, dependencyId)) continue;
            if (candidate == nullptr ||
                (CanInstallPluginRow(row) && !CanInstallPluginRow(*candidate)) ||
                (!row.WebUrl.empty() && candidate->WebUrl.empty())) candidate = &row;
        }
        if (candidate == nullptr)
        {
            unresolved->push_back(dependencyId);
            visiting.erase(normalized);
            visited.insert(normalized);
            return;
        }
        for (const std::wstring& child : candidate->Dependencies) self(self, child);
        if (installed.find(normalized) == installed.end())
        {
            if (!CanInstallPluginRow(*candidate) && candidate->WebUrl.empty())
                unresolved->push_back(dependencyId);
            else
                missing.push_back(*candidate);
        }
        visiting.erase(normalized);
        visited.insert(normalized);
    };
    for (const std::wstring& dependency : root.Dependencies) visit(visit, dependency);
    std::set<std::wstring> seen;
    std::vector<PluginUpdateRow> unique;
    for (PluginUpdateRow& item : missing)
    {
        if (seen.insert(Lower(item.Id)).second) unique.push_back(std::move(item));
    }
    if (unresolved != nullptr)
    {
        std::set<std::wstring> uniqueUnresolved;
        std::vector<std::wstring> result;
        for (const std::wstring& item : *unresolved)
        {
            if (uniqueUnresolved.insert(Lower(item)).second) result.push_back(item);
        }
        *unresolved = std::move(result);
    }
    return unique;
}

std::wstring FormatCapabilitySummary(const CatalogSecurity& security)
{
    std::vector<std::wstring> parts;
    const auto append = [&](const std::wstring& value, int id)
    {
        if (EqualsIgnoreCase(value, L"yes") || EqualsIgnoreCase(value, L"possible"))
            parts.push_back(FormatCapability(value, id));
    };
    append(security.NetworkAccess, IDS_PLUGIN_SECURITY_NETWORK);
    append(security.ExternalProcesses, IDS_PLUGIN_SECURITY_PROCESSES);
    append(security.ScriptExecution, IDS_PLUGIN_SECURITY_SCRIPT);
    append(security.ActiveWebContent, IDS_PLUGIN_SECURITY_WEB);
    std::wstring result;
    for (const std::wstring& part : parts)
    {
        if (!result.empty()) result.append(L", ");
        result += part;
    }
    return result.empty() ? String(IDS_PLUGIN_SECURITY_UNKNOWN) : result;
}

INT_PTR CALLBACK SourcesDialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam);

void CopyTextToClipboard(HWND owner, const std::wstring& text)
{
    if (!OpenClipboard(owner)) return;
    EmptyClipboard();
    const SIZE_T bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL data = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (data != nullptr)
    {
        void* destination = GlobalLock(data);
        if (destination != nullptr)
        {
            memcpy(destination, text.c_str(), bytes);
            GlobalUnlock(data);
            if (SetClipboardData(CF_UNICODETEXT, data) == nullptr) GlobalFree(data);
        }
        else
        {
            GlobalFree(data);
        }
    }
    CloseClipboard();
}

void OpenUrl(HWND owner, const std::wstring& url)
{
    if (url.empty()) return;
    const HINSTANCE result = ShellExecuteW(owner, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32)
        ShowNativeMessage(owner, String(IDS_PLUGINNAME),
                          String(IDS_OPEN_BROWSER_ERROR) + L"\r\n" + std::to_wstring(reinterpret_cast<INT_PTR>(result)),
                          MB_ICONERROR, false);
}

void ShowCopyMenu(HWND dialog, UpdatesDialogState* state, LPARAM position)
{
    if (state == nullptr) return;
    HWND list = GetDlgItem(dialog, IDC_UPDATES_LIST);
    POINT point = {GET_X_LPARAM(position), GET_Y_LPARAM(position)};
    if (point.x == -1 && point.y == -1) GetCursorPos(&point);
    state->ContextRow = -1;
    state->ContextColumn = -1;
    POINT client = point;
    ScreenToClient(list, &client);
    LVHITTESTINFO hit = {};
    hit.pt = client;
    const int index = ListView_SubItemHitTest(list, &hit);
    if (index >= 0)
    {
        state->ContextRow = index;
        state->ContextColumn = hit.iSubItem;
        LVITEMW itemState = {};
        itemState.state = LVIS_SELECTED | LVIS_FOCUSED;
        itemState.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
        SendMessageW(list, LVM_SETITEMSTATE, static_cast<WPARAM>(index), reinterpret_cast<LPARAM>(&itemState));
        UpdateSelectedDetails(dialog, state);
    }
    if (state->ContextRow < 0 || static_cast<size_t>(state->ContextRow) >= state->VisibleRows.size()) return;
    HMENU menu = CreatePopupMenu();
    if (menu == nullptr) return;
    AppendMenuW(menu, MF_STRING, IDM_UPDATES_COPY_VALUE, String(IDS_PLUGIN_COPY_VALUE).c_str());
    AppendMenuW(menu, MF_STRING, IDM_UPDATES_COPY_ROW, String(IDS_PLUGIN_COPY_ROW_HEADERS).c_str());
    const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                                        point.x, point.y, 0, dialog, nullptr);
    DestroyMenu(menu);
    const PluginUpdateRow& row = state->Rows[state->VisibleRows[static_cast<size_t>(state->ContextRow)]];
    if (command == IDM_UPDATES_COPY_VALUE)
    {
        CopyTextToClipboard(dialog, ValueForColumn(row, state->ContextColumn));
    }
    else if (command == IDM_UPDATES_COPY_ROW)
    {
        const int ids[] = {0, IDS_PLUGIN_COL_SOURCE, IDS_PLUGIN_COL_NAME, IDS_PLUGIN_COL_INSTALLED,
                           IDS_PLUGIN_COL_LATEST, IDS_PLUGIN_COL_STATUS, IDS_PLUGIN_COL_AUTHOR};
        std::wstring text;
        for (int index = 0; index < _countof(ids); ++index)
        {
            if (index != 0) text.append(L"\r\n");
            if (ids[index] != 0) text += String(ids[index]);
            text.append(L": ");
            text += ValueForColumn(row, index);
        }
        if (!row.Description.empty())
            text += L"\r\n" + String(IDS_PLUGIN_DESCRIPTION_LABEL) + L": " + row.Description;
        CopyTextToClipboard(dialog, text);
    }
}

bool InstallSelected(HWND dialog, UpdatesDialogState* state)
{
    if (state == nullptr || state->InstallInProgress || state->RefreshInProgress) return false;
    const int selected = ListView_GetNextItem(GetDlgItem(dialog, IDC_UPDATES_LIST), -1, LVNI_SELECTED);
    if (selected < 0 || static_cast<size_t>(selected) >= state->VisibleRows.size()) return false;
    const PluginUpdateRow& root = state->Rows[state->VisibleRows[static_cast<size_t>(selected)]];
    if (!CanInstallPluginRow(root))
    {
        const std::wstring url = root.WebUrl.empty() ? root.Homepage : root.WebUrl;
        if (url.empty())
            ShowNativeMessage(dialog, String(IDS_PLUGINNAME), String(IDS_PLUGIN_UPDATES_NO_URL), MB_ICONINFORMATION, false);
        else
            OpenUrl(dialog, url);
        return true;
    }

    std::vector<std::wstring> unresolved;
    const std::vector<PluginUpdateRow> dependencies = ResolveInstallDependencies(root, state->Rows, &unresolved);
    if (!unresolved.empty())
    {
        std::wstring lines;
        for (const std::wstring& id : unresolved)
        {
            if (!lines.empty()) lines.append(L"\r\n");
            lines += L"- " + id;
        }
        ShowNativeMessage(dialog, String(IDS_PLUGINNAME),
                          FormatLocalizedString(IDS_PLUGIN_DEPENDENCY_UNAVAILABLE, {lines}), MB_ICONWARNING, false);
        return false;
    }
    if (!dependencies.empty())
    {
        std::wstring lines;
        for (const PluginUpdateRow& dependency : dependencies)
        {
            if (!lines.empty()) lines.append(L"\r\n");
            lines += L"- " + dependency.Name + L" - " +
                     String(CanInstallPluginRow(dependency) ? IDS_PLUGIN_INSTALL : IDS_PLUGIN_UPDATES_OPEN_PAGE);
        }
        if (ShowNativeMessage(dialog, String(IDS_PLUGINNAME),
                              FormatLocalizedString(IDS_PLUGIN_DEPENDENCIES_REQUIRED, {lines}),
                              MB_ICONWARNING, true, IDS_PLUGIN_INSTALL, IDS_CANCEL) != IDOK)
            return false;
    }

    std::vector<PluginUpdateRow> risky;
    if (root.Security.IsHigherRisk()) risky.push_back(root);
    for (const PluginUpdateRow& dependency : dependencies)
    {
        if (dependency.Security.IsHigherRisk()) risky.push_back(dependency);
    }
    if (!risky.empty())
    {
        std::wstring lines;
        for (const PluginUpdateRow& package : risky)
        {
            if (!lines.empty()) lines.append(L"\r\n");
            lines += L"- " + package.Name + L": " + FormatCapabilitySummary(package.Security);
        }
        if (ShowNativeMessage(dialog, String(IDS_PLUGINNAME),
                              FormatLocalizedString(IDS_PLUGIN_SECURITY_CAPABILITIES_CONFIRM, {lines}),
                              MB_ICONWARNING, true, IDS_PLUGIN_INSTALL, IDS_CANCEL) != IDOK)
            return false;
    }

    std::vector<PluginUpdateRow> automatic;
    std::vector<std::wstring> webUrls;
    for (const PluginUpdateRow& dependency : dependencies)
    {
        if (CanInstallPluginRow(dependency)) automatic.push_back(dependency);
        else if (!dependency.WebUrl.empty()) webUrls.push_back(dependency.WebUrl);
    }
    automatic.push_back(root);
    state->InstallInProgress = true;
    SetText(dialog, IDC_UPDATES_STATUS, String(IDS_PLUGIN_INSTALL_PREPARING));
    EnableWindow(GetDlgItem(dialog, IDC_UPDATES_INSTALL), FALSE);
    EnableWindow(GetDlgItem(dialog, IDC_UPDATES_REFRESH), FALSE);
    EnableWindow(GetDlgItem(dialog, IDC_UPDATES_SOURCES), FALSE);
    NativeUpdater_QueueInstallPlan(dialog, std::move(automatic), std::move(webUrls));
    return true;
}

void SetUpdatesDialogIcon(HWND dialog)
{
    HICON icon = LoadIconW(DLLInstance, MAKEINTRESOURCEW(IDI_PLUGINICON));
    if (icon != nullptr)
    {
        SendMessageW(dialog, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon));
        SendMessageW(dialog, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon));
    }
}

void SetUpdatesLinkFont(HWND dialog, UpdatesDialogState* state)
{
    if (state == nullptr) return;
    HFONT baseFont = reinterpret_cast<HFONT>(SendMessageW(dialog, WM_GETFONT, 0, 0));
    if (baseFont == nullptr) baseFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    if (state->LinkFont != nullptr)
    {
        for (int control : {IDC_UPDATES_DETAIL_HOMEPAGE, IDC_UPDATES_DETAIL_DOWNLOAD})
            SendDlgItemMessageW(dialog, control, WM_SETFONT, reinterpret_cast<WPARAM>(baseFont), TRUE);
        DeleteObject(state->LinkFont);
        state->LinkFont = nullptr;
    }
    LOGFONTW description = {};
    if (baseFont == nullptr || GetObjectW(baseFont, sizeof(description), &description) == 0) return;
    description.lfUnderline = TRUE;
    state->LinkFont = CreateFontIndirectW(&description);
    if (state->LinkFont == nullptr) return;
    for (int control : {IDC_UPDATES_DETAIL_HOMEPAGE, IDC_UPDATES_DETAIL_DOWNLOAD})
        SendDlgItemMessageW(dialog, control, WM_SETFONT, reinterpret_cast<WPARAM>(state->LinkFont), TRUE);
}

void OpenSelectedDetailLink(HWND dialog, const UpdatesDialogState& state, int control)
{
    const int selected = ListView_GetNextItem(GetDlgItem(dialog, IDC_UPDATES_LIST), -1, LVNI_SELECTED);
    if (selected < 0 || static_cast<size_t>(selected) >= state.VisibleRows.size()) return;
    const PluginUpdateRow& row = state.Rows[state.VisibleRows[static_cast<size_t>(selected)]];
    OpenUrl(dialog, control == IDC_UPDATES_DETAIL_HOMEPAGE ? row.Homepage : row.WebUrl);
}

void RebuildUpdatesImageList(HWND dialog, UpdatesDialogState* state)
{
    if (state == nullptr) return;
    int defaultImageIndex = -1;
    HIMAGELIST imageList = NativeIcons_CreateImageList(ScaleLogical(dialog, 16), &defaultImageIndex);
    if (imageList == nullptr) return;
    HIMAGELIST previous = state->ImageList;
    state->ImageList = imageList;
    state->DefaultImageIndex = defaultImageIndex;
    state->ImageIndexes.clear();
    ListView_SetImageList(GetDlgItem(dialog, IDC_UPDATES_LIST), imageList, LVSIL_SMALL);
    if (previous != nullptr) ImageList_Destroy(previous);
    for (const CatalogIconImage& image : state->CatalogImages)
    {
        const int imageIndex = NativeIcons_AddImage(imageList, image.Bytes);
        if (imageIndex >= 0) state->ImageIndexes[image.Reference] = imageIndex;
    }
    BindRows(dialog, state);
}

INT_PTR CALLBACK UpdatesDialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto* state = reinterpret_cast<UpdatesDialogState*>(GetWindowLongPtrW(dialog, GWLP_USERDATA));
    if (message == WM_GETMINMAXINFO)
    {
        auto* limits = reinterpret_cast<MINMAXINFO*>(lParam);
        RECT minimum = {0, 0, ScaleLogical(dialog, 940), ScaleLogical(dialog, 650)};
        if (limits != nullptr && AdjustWindowRectEx(&minimum,
                static_cast<DWORD>(GetWindowLongPtrW(dialog, GWL_STYLE)), FALSE,
                static_cast<DWORD>(GetWindowLongPtrW(dialog, GWL_EXSTYLE))))
        {
            limits->ptMinTrackSize.x = minimum.right - minimum.left;
            limits->ptMinTrackSize.y = minimum.bottom - minimum.top;
        }
        return TRUE;
    }
    if (message == WM_INITDIALOG)
    {
        state = reinterpret_cast<UpdatesDialogState*>(lParam);
        state->Window = dialog;
        SetWindowLongPtrW(dialog, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        gUpdatesWindow = dialog;
        SetWindowTextW(dialog, String(IDS_PLUGIN_UPDATES_TITLE).c_str());
        SetUpdatesDialogIcon(dialog);
        SetUpdatesLinkFont(dialog, state);
        SetText(dialog, IDC_UPDATES_DESCRIPTION, String(IDS_PLUGIN_UPDATES_DESCRIPTION));
        SetText(dialog, IDC_UPDATES_ONLY, String(IDS_PLUGIN_UPDATES_SHOW_ONLY));
        SetText(dialog, IDC_UPDATES_DETAIL_NAME_LABEL, String(IDS_PLUGIN_COL_NAME) + L":");
        SetText(dialog, IDC_UPDATES_DETAIL_SOURCE_LABEL, String(IDS_PLUGIN_COL_SOURCE) + L":");
        SetText(dialog, IDC_UPDATES_DETAIL_AUTHOR_LABEL, String(IDS_PLUGIN_COL_AUTHOR) + L":");
        SetText(dialog, IDC_UPDATES_DETAIL_STATUS_LABEL, String(IDS_PLUGIN_COL_STATUS) + L":");
        SetText(dialog, IDC_UPDATES_DETAIL_INSTALLED_LABEL, String(IDS_PLUGIN_COL_INSTALLED) + L":");
        SetText(dialog, IDC_UPDATES_DETAIL_LATEST_LABEL, String(IDS_PLUGIN_COL_LATEST) + L":");
        SetText(dialog, IDC_UPDATES_DETAIL_HOMEPAGE_LABEL, String(IDS_PLUGIN_COL_HOMEPAGE) + L":");
        SetText(dialog, IDC_UPDATES_DETAIL_DOWNLOAD_LABEL, String(IDS_PLUGIN_DOWNLOAD_PAGE) + L":");
        SetText(dialog, IDC_UPDATES_DESCRIPTION_LABEL, String(IDS_PLUGIN_DESCRIPTION_LABEL) + L":");
        SetText(dialog, IDC_UPDATES_SECURITY_LABEL, String(IDS_PLUGIN_SECURITY_LABEL) + L":");
        SetText(dialog, IDC_UPDATES_REFRESH, String(IDS_PLUGIN_UPDATES_REFRESH));
        SetText(dialog, IDC_UPDATES_SOURCES, String(IDS_PLUGIN_UPDATES_CONFIGURE_SOURCES));
        SetText(dialog, IDC_UPDATES_INSTALL, String(IDS_PLUGIN_INSTALL));
        SetText(dialog, IDCANCEL, String(IDS_PLUGIN_UPDATES_CLOSE));
        HWND list = GetDlgItem(dialog, IDC_UPDATES_LIST);
        ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
        state->ImageList = NativeIcons_CreateImageList(ScaleLogical(dialog, 16), &state->DefaultImageIndex);
        if (state->ImageList != nullptr) ListView_SetImageList(list, state->ImageList, LVSIL_SMALL);
        SetListViewColumns(list);
        LayoutUpdatesDialog(dialog);
        AdjustListViewColumns(list);
        ApplyDarkMode(dialog);
        DarkModeUpdateListViewColors(list,
                                     SalamanderGeneral != nullptr ? SalamanderGeneral->GetCurrentColor(kColorItemForeground) : GetSysColor(COLOR_WINDOWTEXT),
                                     SalamanderGeneral != nullptr ? SalamanderGeneral->GetCurrentColor(kColorItemBackground) : GetSysColor(COLOR_WINDOW),
                                     true);
        CenterDialog(dialog, state->Parent);
        SetText(dialog, IDC_UPDATES_STATUS, String(IDS_PLUGIN_UPDATES_LOADING));
        state->RefreshInProgress = true;
        EnableWindow(GetDlgItem(dialog, IDC_UPDATES_ONLY), FALSE);
        EnableWindow(GetDlgItem(dialog, IDC_UPDATES_REFRESH), FALSE);
        EnableWindow(GetDlgItem(dialog, IDC_UPDATES_SOURCES), FALSE);
        NativeUpdater_RequestCatalogRefresh(dialog);
        return TRUE;
    }
    if (message == WM_SIZE && state != nullptr && wParam != SIZE_MINIMIZED)
    {
        LayoutUpdatesDialog(dialog);
        AdjustListViewColumns(GetDlgItem(dialog, IDC_UPDATES_LIST));
        return TRUE;
    }
    if (message == WM_DPICHANGED && state != nullptr)
    {
        const auto* suggested = reinterpret_cast<const RECT*>(lParam);
        if (suggested != nullptr)
            SetWindowPos(dialog, nullptr, suggested->left, suggested->top,
                         suggested->right - suggested->left, suggested->bottom - suggested->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        RebuildUpdatesImageList(dialog, state);
        SetUpdatesLinkFont(dialog, state);
        LayoutUpdatesDialog(dialog);
        AdjustListViewColumns(GetDlgItem(dialog, IDC_UPDATES_LIST));
        return TRUE;
    }
    if (message == WM_NOTIFY && state != nullptr)
    {
        const NMHDR* header = reinterpret_cast<const NMHDR*>(lParam);
        if (header->idFrom == IDC_UPDATES_LIST)
        {
            if (header->code == LVN_ITEMCHANGED)
            {
                UpdateSelectedDetails(dialog, state);
                return TRUE;
            }
            if (header->code == LVN_COLUMNCLICK)
            {
                const auto* click = reinterpret_cast<const NMLISTVIEW*>(lParam);
                if (state->SortColumn == click->iSubItem) state->SortDescending = !state->SortDescending;
                else { state->SortColumn = click->iSubItem; state->SortDescending = false; }
                BindRows(dialog, state);
                return TRUE;
            }
            if (header->code == NM_DBLCLK)
            {
                InstallSelected(dialog, state);
                return TRUE;
            }
        }
    }
    if (message == WM_CONTEXTMENU && state != nullptr && reinterpret_cast<HWND>(wParam) == GetDlgItem(dialog, IDC_UPDATES_LIST))
    {
        ShowCopyMenu(dialog, state, lParam);
        return TRUE;
    }
    if (message == WM_COMMAND && state != nullptr)
    {
        if (HIWORD(wParam) == STN_CLICKED &&
            (LOWORD(wParam) == IDC_UPDATES_DETAIL_HOMEPAGE ||
             LOWORD(wParam) == IDC_UPDATES_DETAIL_DOWNLOAD))
        {
            OpenSelectedDetailLink(dialog, *state, LOWORD(wParam));
            return TRUE;
        }
        switch (LOWORD(wParam))
        {
        case IDC_UPDATES_ONLY:
            state->ShowOnlyUpdates = IsDlgButtonChecked(dialog, IDC_UPDATES_ONLY) == BST_CHECKED;
            BindRows(dialog, state);
            return TRUE;
        case IDC_UPDATES_REFRESH:
            if (!state->RefreshInProgress && !state->InstallInProgress)
            {
                state->RefreshInProgress = true;
                SetText(dialog, IDC_UPDATES_STATUS, String(IDS_PLUGIN_UPDATES_LOADING));
                EnableWindow(GetDlgItem(dialog, IDC_UPDATES_ONLY), FALSE);
                EnableWindow(GetDlgItem(dialog, IDC_UPDATES_REFRESH), FALSE);
                EnableWindow(GetDlgItem(dialog, IDC_UPDATES_SOURCES), FALSE);
                NativeUpdater_RequestCatalogRefresh(dialog);
            }
            return TRUE;
        case IDC_UPDATES_SOURCES:
        {
            if (state->RefreshInProgress || state->InstallInProgress) return TRUE;
            const INT_PTR result = DialogBoxParamW(DLLInstance, MAKEINTRESOURCEW(IDD_SAMANDARIN_SOURCES),
                                                   dialog, SourcesDialogProc, 0);
            if (result == IDOK)
            {
                state->RefreshInProgress = true;
                SetText(dialog, IDC_UPDATES_STATUS, String(IDS_PLUGIN_UPDATES_LOADING));
                EnableWindow(GetDlgItem(dialog, IDC_UPDATES_ONLY), FALSE);
                EnableWindow(GetDlgItem(dialog, IDC_UPDATES_REFRESH), FALSE);
                EnableWindow(GetDlgItem(dialog, IDC_UPDATES_SOURCES), FALSE);
                NativeUpdater_RequestCatalogRefresh(dialog);
            }
            return TRUE;
        }
        case IDC_UPDATES_INSTALL:
            InstallSelected(dialog, state);
            return TRUE;
        case IDCANCEL:
            if (state->InstallInProgress)
            {
                MessageBeep(MB_ICONINFORMATION);
                return TRUE;
            }
            EndDialog(dialog, IDCANCEL);
            return TRUE;
        }
    }
    if (message == WM_SAMANDARIN_CATALOG_COMPLETE && state != nullptr)
    {
        std::unique_ptr<CatalogCompletion> completion(reinterpret_cast<CatalogCompletion*>(lParam));
        if (completion != nullptr)
        {
            const int selected = ListView_GetNextItem(GetDlgItem(dialog, IDC_UPDATES_LIST), -1, LVNI_SELECTED);
            if (selected >= 0 && static_cast<size_t>(selected) < state->VisibleRows.size())
            {
                const size_t rowIndex = state->VisibleRows[static_cast<size_t>(selected)];
                if (rowIndex < state->Rows.size())
                {
                    state->PreferredSelectionId = state->Rows[rowIndex].Id;
                    state->PreferredSelectionName = state->Rows[rowIndex].Name;
                }
            }
            state->Rows = std::move(completion->Rows);
            state->RefreshInProgress = false;
            EnableWindow(GetDlgItem(dialog, IDC_UPDATES_ONLY), TRUE);
            SetText(dialog, IDC_UPDATES_STATUS, String(IDS_PLUGIN_UPDATES_READY));
            if (!completion->Errors.empty())
            {
                SetText(dialog, IDC_UPDATES_STATUS,
                        String(IDS_PLUGIN_UPDATES_READY) + L" " + String(IDS_PLUGIN_STATUS_CATALOG_ERROR));
            }
            EnableWindow(GetDlgItem(dialog, IDC_UPDATES_REFRESH), TRUE);
            EnableWindow(GetDlgItem(dialog, IDC_UPDATES_SOURCES), TRUE);
            BindRows(dialog, state);
            NativeUpdater_RequestCatalogIcons(dialog, state->Rows);
        }
        return TRUE;
    }
    if (message == WM_SAMANDARIN_CATALOG_ICONS_COMPLETE && state != nullptr)
    {
        std::unique_ptr<CatalogIconsCompletion> completion(reinterpret_cast<CatalogIconsCompletion*>(lParam));
        if (completion != nullptr && completion->Target == dialog && state->ImageList != nullptr)
        {
            for (const CatalogIconImage& image : completion->Images)
            {
                auto cached = std::find_if(state->CatalogImages.begin(), state->CatalogImages.end(),
                                           [&](const CatalogIconImage& existing)
                                           { return EqualsIgnoreCase(existing.Reference, image.Reference); });
                if (cached == state->CatalogImages.end())
                    state->CatalogImages.push_back(image);
                else
                    cached->Bytes = image.Bytes;
                const int imageIndex = NativeIcons_AddImage(state->ImageList, image.Bytes);
                if (imageIndex >= 0) state->ImageIndexes[image.Reference] = imageIndex;
            }
            RefreshCatalogImages(dialog, state);
        }
        return TRUE;
    }
    if (message == WM_SAMANDARIN_INSTALL_COMPLETE && state != nullptr)
    {
        std::unique_ptr<InstallCompletion> completion(reinterpret_cast<InstallCompletion*>(lParam));
        state->InstallInProgress = false;
        EnableWindow(GetDlgItem(dialog, IDC_UPDATES_REFRESH), TRUE);
        EnableWindow(GetDlgItem(dialog, IDC_UPDATES_SOURCES), TRUE);
        if (completion != nullptr)
        {
            if (completion->Success)
            {
                SetText(dialog, IDC_UPDATES_STATUS, completion->Status);
                ShowNativeMessage(dialog, String(IDS_PLUGINNAME), completion->Status, MB_ICONINFORMATION, false);
                for (const std::wstring& url : completion->WebUrls) OpenUrl(dialog, url);
            }
            else
            {
                SetText(dialog, IDC_UPDATES_STATUS, String(IDS_PLUGIN_INSTALL_FAILED));
                ShowNativeMessage(dialog, String(IDS_PLUGINNAME), String(IDS_PLUGIN_INSTALL_FAILED), MB_ICONERROR, false);
            }
        }
        UpdateSelectedDetails(dialog, state);
        return TRUE;
    }
    if (message == WM_THEMECHANGED || message == WM_SETTINGCHANGE)
    {
        ApplyDarkMode(dialog);
        DarkModeUpdateListViewColors(GetDlgItem(dialog, IDC_UPDATES_LIST));
    }
    if (message == WM_SETCURSOR &&
        (reinterpret_cast<HWND>(wParam) == GetDlgItem(dialog, IDC_UPDATES_DETAIL_HOMEPAGE) ||
         reinterpret_cast<HWND>(wParam) == GetDlgItem(dialog, IDC_UPDATES_DETAIL_DOWNLOAD)))
    {
        SetCursor(LoadCursorW(nullptr, MAKEINTRESOURCEW(32649)));
        return TRUE;
    }
    if (message == WM_CLOSE)
    {
        if (state != nullptr && state->InstallInProgress) return TRUE;
        EndDialog(dialog, IDCANCEL);
        return TRUE;
    }
    if (message == WM_CTLCOLORSTATIC && state != nullptr &&
        (reinterpret_cast<HWND>(lParam) == GetDlgItem(dialog, IDC_UPDATES_DETAIL_HOMEPAGE) ||
         reinterpret_cast<HWND>(lParam) == GetDlgItem(dialog, IDC_UPDATES_DETAIL_DOWNLOAD)))
    {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, DarkModeShouldUseDarkColors() ? RGB(86, 156, 255) : GetSysColor(COLOR_HOTLIGHT));
        SetBkMode(dc, TRANSPARENT);
        return reinterpret_cast<INT_PTR>(gDialogBrush != nullptr ? gDialogBrush : GetSysColorBrush(COLOR_BTNFACE));
    }
    INT_PTR colorResult = 0;
    if (message >= WM_CTLCOLORMSGBOX && message <= WM_CTLCOLORSTATIC &&
        HandleDarkColor(dialog, message, wParam, lParam, &colorResult))
        return colorResult;
    if (message == WM_NCDESTROY)
    {
        NativeUpdater_CancelCatalogIcons(dialog);
        if (state != nullptr)
        {
            if (state->ImageList != nullptr)
            {
                ListView_SetImageList(GetDlgItem(dialog, IDC_UPDATES_LIST), nullptr, LVSIL_SMALL);
                ImageList_Destroy(state->ImageList);
                state->ImageList = nullptr;
            }
            if (state->LinkFont != nullptr)
            {
                DeleteObject(state->LinkFont);
                state->LinkFont = nullptr;
            }
        }
        if (gUpdatesWindow == dialog) gUpdatesWindow = nullptr;
    }
    return FALSE;
}

void PopulateSources(HWND dialog, SourcesDialogState* state)
{
    HWND list = GetDlgItem(dialog, IDC_SOURCES_LIST);
    ListView_DeleteAllItems(list);
    for (size_t index = 0; index < state->Sources.size(); ++index)
    {
        const CatalogSource& source = state->Sources[index];
        LVITEMW item = {};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = static_cast<int>(index);
        item.lParam = static_cast<LPARAM>(index);
        item.pszText = const_cast<wchar_t*>(source.Url.c_str());
        SendMessageW(list, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item));
        ListView_SetCheckState(list, static_cast<int>(index), source.Enabled);
    }
    LVITEMW blank = {};
    blank.mask = LVIF_TEXT | LVIF_PARAM;
    blank.iItem = static_cast<int>(state->Sources.size());
    blank.lParam = blank.iItem;
    blank.pszText = const_cast<wchar_t*>(L"");
    SendMessageW(list, LVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&blank));
    ListView_SetCheckState(list, blank.iItem, TRUE);
    RECT client = {};
    GetClientRect(list, &client);
    SendMessageW(list, LVM_SETCOLUMNWIDTH, 0,
                 MAKELPARAM((std::max)(100, static_cast<int>(client.right) - 4), 0));
}

INT_PTR CALLBACK SourcesDialogProc(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto* state = reinterpret_cast<SourcesDialogState*>(GetWindowLongPtrW(dialog, GWLP_USERDATA));
    if (message == WM_INITDIALOG)
    {
        state = NewNoThrow<SourcesDialogState>();
        if (state == nullptr)
        {
            EndDialog(dialog, IDCANCEL);
            return TRUE;
        }
        state->Sources = LoadCatalogSources();
        SetWindowLongPtrW(dialog, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
        SetWindowTextW(dialog, String(IDS_PLUGIN_UPDATES_SOURCES_TITLE).c_str());
        SetText(dialog, IDC_SOURCES_DESCRIPTION, String(IDS_PLUGIN_UPDATES_SOURCES));
        SetText(dialog, IDC_SOURCES_SAVE, String(IDS_PLUGIN_UPDATES_SAVE_SOURCES));
        SetText(dialog, IDC_SOURCES_CLOSE, String(IDS_PLUGIN_UPDATES_CLOSE));
        HWND list = GetDlgItem(dialog, IDC_SOURCES_LIST);
        ListView_SetExtendedListViewStyle(list, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        LVCOLUMNW column = {};
        column.mask = LVCF_TEXT | LVCF_WIDTH;
        column.pszText = const_cast<wchar_t*>(L"");
        column.cx = 500;
        SendMessageW(list, LVM_INSERTCOLUMNW, 0, reinterpret_cast<LPARAM>(&column));
        PopulateSources(dialog, state);
        ApplyDarkMode(dialog);
        CenterDialog(dialog, GetParent(dialog));
        return TRUE;
    }
    if (message == WM_COMMAND && state != nullptr)
    {
        if (LOWORD(wParam) == IDC_SOURCES_SAVE)
        {
            const int last = ListView_GetItemCount(GetDlgItem(dialog, IDC_SOURCES_LIST)) - 1;
            if (last >= 0 && static_cast<size_t>(last) < state->Sources.size() &&
                state->Sources[static_cast<size_t>(last)].Url.empty())
                state->Sources.pop_back();
            if (!SaveCatalogSources(state->Sources))
            {
                ShowNativeMessage(dialog, String(IDS_PLUGINNAME), String(IDS_PLUGIN_INSTALL_FAILED), MB_ICONERROR, false);
                return TRUE;
            }
            state->Saved = true;
            ShowNativeMessage(dialog, String(IDS_PLUGINNAME), String(IDS_PLUGIN_SOURCES_SAVED), MB_ICONINFORMATION, false);
            EndDialog(dialog, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDC_SOURCES_CLOSE || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(dialog, IDCANCEL);
            return TRUE;
        }
    }
    if (message == WM_NOTIFY && state != nullptr)
    {
        const NMHDR* header = reinterpret_cast<const NMHDR*>(lParam);
        if (header->idFrom == IDC_SOURCES_LIST)
        {
            HWND list = GetDlgItem(dialog, IDC_SOURCES_LIST);
            if (header->code == LVN_ITEMCHANGED)
            {
                const auto* changed = reinterpret_cast<const NMLISTVIEW*>(lParam);
                const int index = changed->iItem;
                if (index >= 0 && static_cast<size_t>(index) < state->Sources.size() &&
                    (changed->uChanged & LVIF_STATE) != 0)
                {
                    state->Sources[static_cast<size_t>(index)].Enabled = ListView_GetCheckState(list, index) != FALSE;
                }
                return TRUE;
            }
            if (header->code == NM_DBLCLK)
            {
                const int selected = ListView_GetNextItem(list, -1, LVNI_SELECTED);
                if (selected >= 0) SendMessageW(list, LVM_EDITLABELW, static_cast<WPARAM>(selected), 0);
                return TRUE;
            }
            if (header->code == LVN_ENDLABELEDITW)
            {
                const auto* info = reinterpret_cast<const NMLVDISPINFOW*>(lParam);
                const int index = info->item.iItem;
                if (info->item.pszText == nullptr || index < 0) return FALSE;
                const std::wstring url = Trim(info->item.pszText);
                if (url.empty()) return FALSE;
                if (static_cast<size_t>(index) == state->Sources.size())
                {
                    state->Sources.push_back({url, true});
                    PopulateSources(dialog, state);
                }
                else if (static_cast<size_t>(index) < state->Sources.size())
                {
                    state->Sources[static_cast<size_t>(index)].Url = url;
                    SetWindowTextW(GetDlgItem(dialog, IDC_SOURCES_DESCRIPTION), String(IDS_PLUGIN_UPDATES_SOURCES).c_str());
                    PopulateSources(dialog, state);
                }
                return TRUE;
            }
            if (header->code == LVN_KEYDOWN)
            {
                const auto* key = reinterpret_cast<const NMLVKEYDOWN*>(lParam);
                const int selected = ListView_GetNextItem(list, -1, LVNI_SELECTED);
                if (key->wVKey == VK_F2 && selected >= 0)
                {
                    SendMessageW(list, LVM_EDITLABELW, static_cast<WPARAM>(selected), 0);
                    return TRUE;
                }
                if (key->wVKey == VK_DELETE && selected >= 0 && static_cast<size_t>(selected) < state->Sources.size())
                {
                    state->Sources.erase(state->Sources.begin() + selected);
                    PopulateSources(dialog, state);
                    return TRUE;
                }
            }
        }
    }
    if (message == WM_THEMECHANGED || message == WM_SETTINGCHANGE) ApplyDarkMode(dialog);
    INT_PTR colorResult = 0;
    if (message >= WM_CTLCOLORMSGBOX && message <= WM_CTLCOLORSTATIC &&
        HandleDarkColor(dialog, message, wParam, lParam, &colorResult))
        return colorResult;
    if (message == WM_NCDESTROY)
    {
        delete state;
        SetWindowLongPtrW(dialog, GWLP_USERDATA, 0);
    }
    return FALSE;
}

void HandleCheckCompletion(CheckCompletion* completion)
{
    if (completion == nullptr) return;
    if (gConfigurationWindow != nullptr && IsWindow(gConfigurationWindow))
        PostMessageW(gConfigurationWindow, WM_SAMANDARIN_CONFIG_REFRESH, 0, 0);
    const HWND owner = ResolveOwner(completion->Owner);
    if (completion->NotifyRelease)
    {
        const std::wstring message = String(IDS_UPDATE_AVAILABLE) + L"\r\n\r\n" +
                                     FormatLocalizedString(IDS_CURRENT_VERSION, {completion->CurrentVersion}) + L"\r\n" +
                                     FormatLocalizedString(IDS_LATEST_VERSION, {completion->LatestVersion}) + L"\r\n\r\n" +
                                     String(IDS_OPEN_DOWNLOAD_PAGE);
        if (ShowNativeMessage(owner, String(IDS_PLUGINNAME), message, MB_ICONINFORMATION,
                              true, IDS_OK, IDS_CANCEL) == IDOK)
        {
            OpenUrl(owner, L"https://github.com/KRtkovo-eu-AI/salamander/releases/latest");
        }
    }
    else if (completion->UserInitiated && !completion->Error.empty())
    {
        const int errorString = completion->InstallError ? IDS_PLUGIN_INSTALL_FAILED : IDS_CHECK_ERROR;
        ShowNativeMessage(owner, String(IDS_PLUGINNAME), String(errorString), MB_ICONERROR, false);
    }
    else if (completion->UserInitiated && completion->ShowIfCurrent)
    {
        const std::wstring message = completion->LatestVersion.empty()
            ? FormatLocalizedString(IDS_UP_TO_DATE_NO_RELEASE, {completion->CurrentVersion})
            : FormatLocalizedString(IDS_UP_TO_DATE_RELEASE, {completion->CurrentVersion, completion->LatestVersion});
        ShowNativeMessage(owner, String(IDS_PLUGINNAME), message, MB_ICONINFORMATION, false);
    }

    std::vector<std::wstring> updates;
    for (const PluginUpdateRow& row : completion->PluginRows)
    {
        if (row.Status == PluginUpdateStatus::UpdateAvailable)
            updates.push_back(L"- " + row.Name + L": " + row.LatestVersion);
    }
    if (!updates.empty())
    {
        std::wstring list;
        for (const std::wstring& update : updates)
        {
            if (!list.empty()) list.append(L"\r\n");
            list += update;
        }
        const std::wstring message = FormatLocalizedString(IDS_PLUGIN_UPDATES_AVAILABLE, {list});
        if (ShowNativeMessage(owner, String(IDS_PLUGIN_UPDATES_TITLE), message, MB_ICONINFORMATION,
                              true, IDS_PLUGIN_UPDATES_OPEN, IDS_PLUGIN_UPDATES_CLOSE) == IDOK)
        {
            NativeUi_ShowPluginUpdates(owner);
        }
    }
    else if (completion->UserInitiated && completion->Error.empty() && !completion->CatalogError.empty())
    {
        ShowNativeMessage(owner, String(IDS_PLUGINNAME), String(IDS_CHECK_ERROR), MB_ICONERROR, false);
    }
}

LRESULT CALLBACK MessageWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_SAMANDARIN_CHECK_COMPLETE)
    {
        std::unique_ptr<CheckCompletion> completion(reinterpret_cast<CheckCompletion*>(lParam));
        HandleCheckCompletion(completion.get());
        return 0;
    }
    if (message == WM_SAMANDARIN_CATALOG_COMPLETE)
    {
        std::unique_ptr<CatalogCompletion> completion(reinterpret_cast<CatalogCompletion*>(lParam));
        if (completion != nullptr && completion->Target != nullptr && completion->Target == gUpdatesWindow && IsWindow(completion->Target))
        {
            CatalogCompletion* raw = completion.release();
            if (!PostMessageW(raw->Target, WM_SAMANDARIN_CATALOG_COMPLETE, 0, reinterpret_cast<LPARAM>(raw))) delete raw;
        }
        return 0;
    }
    if (message == WM_SAMANDARIN_CATALOG_ICONS_COMPLETE)
    {
        std::unique_ptr<CatalogIconsCompletion> completion(reinterpret_cast<CatalogIconsCompletion*>(lParam));
        if (completion != nullptr && completion->Target != nullptr &&
            completion->Target == gUpdatesWindow && IsWindow(completion->Target))
        {
            CatalogIconsCompletion* raw = completion.release();
            if (!PostMessageW(raw->Target, WM_SAMANDARIN_CATALOG_ICONS_COMPLETE, 0,
                              reinterpret_cast<LPARAM>(raw)))
                delete raw;
        }
        return 0;
    }
    if (message == WM_SAMANDARIN_INSTALL_COMPLETE)
    {
        std::unique_ptr<InstallCompletion> completion(reinterpret_cast<InstallCompletion*>(lParam));
        if (completion != nullptr && completion->Target != nullptr && completion->Target == gUpdatesWindow && IsWindow(completion->Target))
        {
            InstallCompletion* raw = completion.release();
            if (!PostMessageW(raw->Target, WM_SAMANDARIN_INSTALL_COMPLETE, 0, reinterpret_cast<LPARAM>(raw))) delete raw;
        }
        return 0;
    }
    if (message == WM_NCDESTROY)
    {
        if (gMessageWindow == window) gMessageWindow = nullptr;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}
} // namespace

bool NativeUi_Initialize(HWND parent)
{
    if (NativeUi_GetMessageWindow() != nullptr) return true;
    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = MessageWindowProc;
    windowClass.hInstance = DLLInstance;
    windowClass.lpszClassName = kMessageWindowClass;
    if (!RegisterClassExW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
    gMessageWindow = CreateWindowExW(0, kMessageWindowClass, L"", 0,
                                     0, 0, 0, 0, HWND_MESSAGE, nullptr, DLLInstance, nullptr);
    if (gMessageWindow == nullptr)
    {
        UnregisterClassW(kMessageWindowClass, DLLInstance);
        return false;
    }
    (void)parent;
    return true;
}

void NativeUi_Shutdown()
{
    if (gConfigurationWindow != nullptr && IsWindow(gConfigurationWindow))
        SendMessageW(gConfigurationWindow, WM_CLOSE, 0, 0);
    if (gUpdatesWindow != nullptr && IsWindow(gUpdatesWindow))
        SendMessageW(gUpdatesWindow, WM_CLOSE, 0, 0);
    HWND messageWindow = NativeUi_GetMessageWindow();
    if (messageWindow != nullptr)
    {
        DestroyWindow(messageWindow);
        InterlockedExchangePointer(reinterpret_cast<PVOID volatile*>(&gMessageWindow), nullptr);
    }
    UnregisterClassW(kMessageWindowClass, DLLInstance);
    if (gDialogBrush != nullptr)
    {
        DarkModeConfigureDialogColors(GetSysColor(COLOR_BTNTEXT), GetSysColor(COLOR_BTNFACE),
                                      GetSysColorBrush(COLOR_BTNFACE));
        DeleteObject(gDialogBrush);
        gDialogBrush = nullptr;
        gConfiguredBackground = CLR_INVALID;
        gConfiguredForeground = CLR_INVALID;
    }
}

HWND NativeUi_GetMessageWindow()
{
    return reinterpret_cast<HWND>(InterlockedCompareExchangePointer(
        reinterpret_cast<PVOID volatile*>(&gMessageWindow), nullptr, nullptr));
}

BOOL NativeUi_ShowConfiguration(HWND parent, const NativeUpdateSettings& settings,
                                const std::wstring& currentVersion)
{
    ConfigurationParameters parameters;
    parameters.Settings = settings;
    parameters.CurrentVersion = currentVersion;
    const INT_PTR result = DialogBoxParamW(DLLInstance, MAKEINTRESOURCEW(IDD_SAMANDARIN_CONFIG),
                                           ResolveOwner(parent), ConfigurationDialogProc,
                                           reinterpret_cast<LPARAM>(&parameters));
    return result != -1;
}

BOOL NativeUi_ShowPluginUpdates(HWND parent)
{
    UpdatesDialogState state;
    state.Parent = ResolveOwner(parent);
    const INT_PTR result = DialogBoxParamW(DLLInstance, MAKEINTRESOURCEW(IDD_SAMANDARIN_UPDATES),
                                           state.Parent, UpdatesDialogProc,
                                           reinterpret_cast<LPARAM>(&state));
    return result != -1;
}

void NativeUi_NotifyColorsChanged()
{
    ConfigureDarkMode();
    for (HWND window : {gMessageWindow, gConfigurationWindow, gUpdatesWindow})
    {
        if (window != nullptr && IsWindow(window))
        {
            ApplyDarkMode(window);
            InvalidateRect(window, nullptr, TRUE);
        }
    }
}

void NativeUi_PostCheckCompletion(CheckCompletion* completion)
{
    HWND window = NativeUi_GetMessageWindow();
    if (completion == nullptr) return;
    if (window == nullptr || !PostMessageW(window, WM_SAMANDARIN_CHECK_COMPLETE, 0, reinterpret_cast<LPARAM>(completion)))
        delete completion;
}

void NativeUi_PostCatalogCompletion(CatalogCompletion* completion)
{
    HWND window = NativeUi_GetMessageWindow();
    if (completion == nullptr) return;
    if (window == nullptr || !PostMessageW(window, WM_SAMANDARIN_CATALOG_COMPLETE, 0, reinterpret_cast<LPARAM>(completion)))
        delete completion;
}

void NativeUi_PostCatalogIcons(CatalogIconsCompletion* completion)
{
    HWND window = NativeUi_GetMessageWindow();
    if (completion == nullptr) return;
    if (window == nullptr ||
        !PostMessageW(window, WM_SAMANDARIN_CATALOG_ICONS_COMPLETE, 0,
                      reinterpret_cast<LPARAM>(completion)))
        delete completion;
}

void NativeUi_PostInstallCompletion(InstallCompletion* completion)
{
    HWND window = NativeUi_GetMessageWindow();
    if (completion == nullptr) return;
    if (window == nullptr || !PostMessageW(window, WM_SAMANDARIN_INSTALL_COMPLETE, 0, reinterpret_cast<LPARAM>(completion)))
        delete completion;
}
} // namespace SamandarinNative
