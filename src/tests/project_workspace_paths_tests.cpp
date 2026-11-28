// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-28 | Test project-workspace path boundaries and reparse-point resolution. | Prevent sibling, long-path, and linked-directory escapes from being treated as in-root navigation. | https://github.com/KRtkovo-eu-AI/salamander/issues/853 | openai/gpt-6-luna-pro |
#include "../project_workspace_paths.h"
#include <stdio.h>

#ifndef SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
#define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE 0x2
#endif

static int Failures = 0;
static int Checks = 0;

static void Check(bool condition, const char* message)
{
    ++Checks;
    if (!condition)
    {
        ++Failures;
        printf("FAIL: %s (error %lu)\n", message, GetLastError());
    }
}

static std::wstring Join(const std::wstring& parent, const wchar_t* child)
{
    return parent + (parent.empty() || parent.back() == L'\\' ? L"" : L"\\") + child;
}

static bool CreateFixtureDirectory(const std::wstring& path)
{
    const std::wstring extended = Salamander::ProjectWorkspacePaths::ExtendedPath(path);
    return CreateDirectoryW(extended.c_str(), NULL) != FALSE;
}

static void RemoveFixtureDirectory(const std::wstring& path)
{
    const std::wstring extended = Salamander::ProjectWorkspacePaths::ExtendedPath(path);
    RemoveDirectoryW(extended.c_str());
}

struct Fixture
{
    std::wstring Root;
    std::wstring Child;
    std::wstring Sibling;
    std::wstring Outside;
    std::wstring Link;
    std::wstring BrokenLink;
    bool RootCreated = false;
    bool ChildCreated = false;
    bool SiblingCreated = false;
    bool OutsideCreated = false;
    bool LinkCreated = false;
    bool BrokenLinkCreated = false;

    Fixture()
    {
        std::vector<wchar_t> temporaryPath(32768, L'\0');
        const DWORD length = GetTempPathW(static_cast<DWORD>(temporaryPath.size()), temporaryPath.data());
        Check(length > 0 && length < temporaryPath.size(), "get temporary directory");
        if (length == 0 || length >= temporaryPath.size())
            return;

        Root = Join(temporaryPath.data(), (L"samandarin-project-root-" + std::to_wstring(GetCurrentProcessId()) +
                                             L"-" + std::to_wstring(GetTickCount64())).c_str());
        Child = Join(Root, L"src");
        Sibling = Root + L"2";
        Outside = Join(Sibling, L"other");
        Link = Join(Root, L"external-link");
        BrokenLink = Join(Root, L"broken-external-link");
        RootCreated = CreateFixtureDirectory(Root);
        Check(RootCreated, "create project root");
        if (!RootCreated)
            return;

        ChildCreated = CreateFixtureDirectory(Child);
        Check(ChildCreated, "create project child");
        SiblingCreated = CreateFixtureDirectory(Sibling);
        Check(SiblingCreated, "create sibling directory");
        if (SiblingCreated)
        {
            OutsideCreated = CreateFixtureDirectory(Outside);
            Check(OutsideCreated, "create external target");
        }
    }

    ~Fixture()
    {
        if (LinkCreated)
            RemoveFixtureDirectory(Link);
        if (BrokenLinkCreated)
            RemoveFixtureDirectory(BrokenLink);
        if (ChildCreated)
            RemoveFixtureDirectory(Child);
        if (OutsideCreated)
            RemoveFixtureDirectory(Outside);
        if (SiblingCreated)
            RemoveFixtureDirectory(Sibling);
        if (RootCreated)
            RemoveFixtureDirectory(Root);
    }
};

int main()
{
    using namespace Salamander::ProjectWorkspacePaths;

    Check(IsWithinLexically(L"C:\\Projects\\Example", L"C:\\Projects\\Example"), "the root contains itself");
    Check(IsWithinLexically(L"C:\\Projects\\Example", L"C:\\Projects\\Example\\src"), "a child directory is inside");
    Check(!IsWithinLexically(L"C:\\Projects\\Example", L"C:\\Projects\\Example2"), "a sibling sharing the prefix is outside");
    Check(!IsWithinLexically(L"C:\\Projects\\Example", L"C:\\Projects\\Other"), "a neighboring directory is outside");
    Check(IsWithinLexically(L"C:\\Projects\\Example", L"\\\\?\\C:\\Projects\\Example\\src"), "extended drive paths normalize");
    Check(IsWithinLexically(L"C:\\Projects\\Example", L"C:\\Projects\\Example\\src\\..\\tests"), "dot segments normalize before checking");
    Check(!IsWithinLexically(L"C:\\Projects\\Example", L"C:\\Projects\\Example\\..\\Other"), "parent segments cannot escape the root");
    Check(IsWithinLexically(L"\\\\server\\share\\Projects\\Example", L"\\\\server\\share\\Projects\\Example\\src"), "UNC children are supported");
    Check(!IsWithinLexically(L"\\\\server\\share\\Projects\\Example", L"\\\\server\\share\\Projects\\Example2"), "UNC component boundaries are respected");
    Check(IsWithinLexically(L"C:\\Projekty\\Příliš žluťoučký", L"c:\\projekty\\PŘÍLIŠ ŽLUŤOUČKÝ\\src"), "Unicode paths compare case-insensitively");

    std::wstring longRoot = L"C:\\Projects";
    for (int i = 0; i < 40; ++i)
        longRoot += L"\\long-component-0123456789";
    const std::wstring longChild = Join(longRoot, L"src");
    Check(longRoot.size() > MAX_PATH && IsWithinLexically(longRoot.c_str(), longChild.c_str()), "long paths use dynamically sized storage");

    std::wstring longUtf8Root = L"C:\\Projects";
    for (int i = 0; i < 40; ++i)
        longUtf8Root += L"\\\u00e9\u00e9\u00e9\u00e9";
    const int longUtf8Bytes = WideCharToMultiByte(CP_UTF8, 0, longUtf8Root.c_str(), -1, NULL, 0, NULL, NULL);
    Check(longUtf8Root.size() < MAX_PATH && longUtf8Bytes > MAX_PATH &&
              IsWithinLexically(longUtf8Root.c_str(), Join(longUtf8Root, L"src").c_str()),
          "UTF-8 byte length above MAX_PATH does not truncate a UTF-16 path");

    Fixture fixture;
    if (fixture.RootCreated)
    {
        if (fixture.ChildCreated)
            Check(IsWithinProjectRoot(fixture.Root.c_str(), fixture.Child.c_str()), "existing descendant resolves inside the root");
        Check(IsWithinProjectRoot(fixture.Root.c_str(), Join(fixture.Root, L"missing\\child").c_str()), "missing descendants inherit their nearest existing ancestor");
        if (fixture.SiblingCreated)
            Check(!IsWithinProjectRoot(fixture.Root.c_str(), fixture.Sibling.c_str()), "an existing sibling resolves outside the root");

        const DWORD linkFlags = SYMBOLIC_LINK_FLAG_DIRECTORY | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;
        fixture.LinkCreated = fixture.SiblingCreated && fixture.OutsideCreated &&
                              CreateSymbolicLinkW(ExtendedPath(fixture.Link).c_str(),
                                                  ExtendedPath(fixture.Outside).c_str(), linkFlags) != FALSE;
        if (fixture.LinkCreated)
            Check(!IsWithinProjectRoot(fixture.Root.c_str(), Join(fixture.Link, L"child").c_str()), "a directory link cannot cross the physical project boundary");

        const std::wstring missingTarget = Join(fixture.Sibling, L"missing-target");
        fixture.BrokenLinkCreated = fixture.SiblingCreated &&
                                    CreateSymbolicLinkW(ExtendedPath(fixture.BrokenLink).c_str(),
                                                        ExtendedPath(missingTarget).c_str(), linkFlags) != FALSE;
        if (fixture.BrokenLinkCreated)
            Check(!IsWithinProjectRoot(fixture.Root.c_str(), Join(fixture.BrokenLink, L"child").c_str()), "an unresolved directory link cannot inherit lexical containment");
    }

    printf("Project workspace path tests: %d checks, %d failures.\n", Checks, Failures);
    return Failures == 0 ? 0 : 1;
}
