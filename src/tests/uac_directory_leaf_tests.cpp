/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-07 | Test anchored mkdir against real NTFS substitutions. | Verify that a replacement path and reparse points cannot redirect a creation. | | GPT-6.1 Sol (Codex) |
*/
#include "../uacbroker/directoryleaf.h"
#include <winioctl.h>
#include <cstdio>
#include <memory>

namespace
{
struct Close
{
    void operator()(void* handle) const { CloseHandle(handle); }
};
using Handle = std::unique_ptr<void, Close>;

bool DirectoryExists(const std::wstring& path)
{
    const DWORD attr = GetFileAttributesW(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

int failures = 0;
void Check(bool passed, const char* label, DWORD error = 0)
{
    std::printf("%s: %s (%lu)\n", passed ? "PASS" : "FAIL", label, error);
    if (!passed)
        ++failures;
}
}

int wmain(int argc, wchar_t** argv)
{
    using namespace UacDirectoryLeaf;
    if (argc != 2)
        return 2;
    const std::wstring fixture = L"\\\\?\\" + std::wstring(argv[1]);
    const HANDLE rawRoot = CreateFileW(fixture.c_str(), FILE_TRAVERSE | FILE_READ_ATTRIBUTES,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                        nullptr);
    if (rawRoot == INVALID_HANDLE_VALUE)
        return 2;
    const Handle root(rawRoot);

    HANDLE rawParent = nullptr;
    ParentIdentity expected;
    DWORD error = OpenParentBelowRoot(root.get(), L"ancestor\\parent", rawParent, expected);
    Check(error == ERROR_SUCCESS, "open verified NTFS parent below root", error);
    if (error)
        return 1;
    const Handle original(rawParent);
    const auto ancestor = fixture + L"\\ancestor";
    const auto moved = fixture + L"\\moved";
    if (!MoveFileExW((ancestor + L"\\parent").c_str(), moved.c_str(), 0) ||
        !CreateDirectoryW((ancestor + L"\\parent").c_str(), nullptr))
    {
        Check(false, "swap the parent while its original handle is held", GetLastError());
        return 1;
    }

    ParentIdentity replacement;
    error = OpenParentBelowRoot(root.get(), L"ancestor\\parent", rawParent, replacement);
    Check(error == ERROR_SUCCESS && !SameParent(expected, replacement), "replacement has a different identity", error);
    if (error)
        return 1;
    const Handle substitute(rawParent);
    const std::wstring leaf = L"\u017dlu\u0165ou\u010dk\u00fd-\u6771\u4eac";
    const auto unexpected = ancestor + L"\\parent\\" + leaf;
    const auto intended = moved + L"\\" + leaf;
    error = CreateVerifiedLeaf(substitute.get(), expected, leaf);
    Check(error == ERROR_FILE_INVALID && !DirectoryExists(unexpected), "reopened replacement rejected before mutation", error);

    error = CreateVerifiedLeaf(original.get(), expected, leaf);
    Check(error == ERROR_SUCCESS && DirectoryExists(intended) && !DirectoryExists(unexpected),
          "held handle creates only under the original parent after swap", error);
    error = CreateVerifiedLeaf(original.get(), expected, leaf);
    Check(error == ERROR_ALREADY_EXISTS && DirectoryExists(intended), "collision preserves existing folder", error);

    for (const auto invalid : {L"..", L"parent\\escaped", L"parent/escaped", L"x:ads", L"NUL.txt", L"x.", L"x "})
    {
        error = CreateVerifiedLeaf(original.get(), expected, invalid);
        Check(error == ERROR_INVALID_NAME, "reject traversal, ADS, reserved or ambiguous leaf", error);
    }
    const wchar_t embedded[] = {L'x', L'\0', L'y'};
    error = CreateVerifiedLeaf(original.get(), expected, std::wstring_view(embedded, ARRAYSIZE(embedded)));
    Check(error == ERROR_INVALID_NAME, "reject embedded NUL", error);
    error = OpenParentBelowRoot(root.get(), L"..\\escape", rawParent, replacement);
    Check(error == ERROR_INVALID_NAME && !rawParent, "reject traversal in parent path", error);

    for (const auto reparse : {L"junction", L"junction\\nested"})
    {
        error = OpenParentBelowRoot(root.get(), reparse, rawParent, replacement);
        Check(error != ERROR_SUCCESS && !rawParent, "reject final or intermediate junction", error);
        if (rawParent)
            CloseHandle(rawParent);
    }

    // Convert an existing, empty and verified parent to a junction in place.
    const auto mutablePath = fixture + L"\\mutable";
    if (!CreateDirectoryW(mutablePath.c_str(), nullptr))
        return 2;
    error = OpenParentBelowRoot(root.get(), L"mutable", rawParent, expected);
    if (error)
        return 2;
    const Handle mutableParent(rawParent);
    const auto openReparse = [](const std::wstring& path, DWORD access) {
        const HANDLE h = CreateFileW(path.c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                     nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                     nullptr);
        return Handle(h != INVALID_HANDLE_VALUE ? h : nullptr);
    };
    const auto donor = openReparse(fixture + L"\\junction", FILE_READ_ATTRIBUTES);
    const auto writable = openReparse(mutablePath, GENERIC_WRITE);
    std::unique_ptr<BYTE[]> tag(new BYTE[MAXIMUM_REPARSE_DATA_BUFFER_SIZE]);
    DWORD bytes = 0;
    if (!donor || !writable ||
        !DeviceIoControl(donor.get(), FSCTL_GET_REPARSE_POINT, nullptr, 0, tag.get(),
                         MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &bytes, nullptr) ||
        !DeviceIoControl(writable.get(), FSCTL_SET_REPARSE_POINT, tag.get(), bytes, nullptr, 0, &bytes, nullptr))
    {
        Check(false, "convert the already verified parent into a junction", GetLastError());
        return 1;
    }
    const auto redirect = fixture + L"\\destination\\must-not-create";
    error = CreateVerifiedLeaf(mutableParent.get(), expected, L"must-not-create");
    Check(error == ERROR_REPARSE_TAG_INVALID && !DirectoryExists(redirect),
          "reject a reparse tag added to the verified parent in place", error);
    // Model a tag added after validation but before the syscall. A root-relative
    // create must either fail or stay on the original object.
    HANDLE raced = nullptr;
    const DWORD racedResult = Detail::NativeDirectory(mutableParent.get(), L"race-leaf", FILE_CREATE, raced);
    if (racedResult == ERROR_SUCCESS)
        CloseHandle(raced);
    Check(!DirectoryExists(fixture + L"\\destination\\race-leaf"),
          "native call cannot follow a parent tag added after validation", racedResult);
    struct { DWORD tag; WORD length; WORD reserved; } remove{IO_REPARSE_TAG_MOUNT_POINT, 0, 0};
    if (!DeviceIoControl(writable.get(), FSCTL_DELETE_REPARSE_POINT, &remove, sizeof(remove), nullptr, 0, &bytes, nullptr))
    {
        Check(false, "remove the test junction tag", GetLastError());
        return 1;
    }

    Check(racedResult != ERROR_SUCCESS || DirectoryExists(mutablePath + L"\\race-leaf"),
          "successful raced creation stays in the original object", racedResult);

    const std::wstring first = L"long-" + std::wstring(175, L'\u65e5');
    const std::wstring second(100, L'\u672c');
    const auto longPath = fixture + L"\\" + first + L"\\" + second;
    if (!CreateDirectoryW((fixture + L"\\" + first).c_str(), nullptr) || !CreateDirectoryW(longPath.c_str(), nullptr))
        return 2;
    error = OpenParentBelowRoot(root.get(), first + L"\\" + second, rawParent, expected);
    Check(error == ERROR_SUCCESS && longPath.size() > MAX_PATH, "open long Unicode parent", error);
    if (error)
        return 1;
    Handle longParent(rawParent);
    error = CreateVerifiedLeaf(longParent.get(), expected, leaf);
    Check(error == ERROR_SUCCESS && DirectoryExists(longPath + L"\\" + leaf), "create exact Unicode leaf on a long path", error);

    longParent.reset();
    // The Python runner may have a MAX_PATH-limited filesystem API.
    const BOOL removedLeaf = error == ERROR_SUCCESS ? RemoveDirectoryW((longPath + L"\\" + leaf).c_str()) : TRUE;
    const BOOL removedParent = RemoveDirectoryW(longPath.c_str());
    const BOOL removedFirst = RemoveDirectoryW((fixture + L"\\" + first).c_str());
    Check(removedLeaf && removedParent && removedFirst, "remove only the owned long-path fixture");
    return failures ? 1 : 0;
}
