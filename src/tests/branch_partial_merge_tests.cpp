// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-12 | Exercise identity-bound partial listing merges. | Detect replacements, duplicates and missing prior rows. | #731 | GPT-6.1 Terra |
#include "../branch_view.h"
#include <cstdio>
using namespace Salamander::BranchView;
static int failures = 0;
static void Check(bool ok, const char* label) { std::printf("%s: %s\n", ok ? "PASS" : "FAIL", label); if (!ok) ++failures; }
static WIN32_FIND_DATAW Row(const wchar_t* name, DWORD attrs = FILE_ATTRIBUTE_NORMAL)
{
    WIN32_FIND_DATAW row{}; wcscpy_s(row.cFileName, name); row.dwFileAttributes = attrs;
    row.nFileSizeLow = 8; row.ftCreationTime.dwLowDateTime = 1; row.ftLastWriteTime.dwLowDateTime = 2;
    return row;
}
int wmain()
{
    DeniedDirectory parent; parent.Verified = true; parent.Volume = 17; parent.FileId[0] = 42;
    BYTE first[16]{1}, next[16]{2}, replacement[16]{3};
    const auto file = Row(L"a.txt"), child = Row(L"sub", FILE_ATTRIBUTE_DIRECTORY), added = Row(L"b.txt");
    {
        PartialDirectoryRows rows(parent);
        Check(rows.ObserveOriginal(file, first) && rows.ObserveOriginal(child, next) &&
              rows.BeginRetry(parent.Volume, parent.FileId), "bind partial rows to the original parent");
        Check(rows.ObserveRetry(file, first) == RetryRowDecision::KeepOriginal &&
              rows.ObserveRetry(child, next) == RetryRowDecision::KeepOriginal &&
              rows.ObserveRetry(added, replacement) == RetryRowDecision::Add && rows.Complete(),
              "already published file and queued child are not duplicated while new row is added");
    }
    {
        PartialDirectoryRows rows(parent); rows.ObserveOriginal(file, first);
        Check(rows.BeginRetry(parent.Volume, parent.FileId) &&
              rows.ObserveRetry(file, replacement) == RetryRowDecision::Conflict && !rows.Complete(),
              "same name and metadata with a different file ID is a conflict");
    }
    {
        PartialDirectoryRows rows(parent); rows.ObserveOriginal(file, first);
        BYTE wrongParent[16]{9};
        Check(!rows.BeginRetry(parent.Volume, wrongParent) && !rows.BeginRetry(parent.Volume, parent.FileId) && !rows.Complete(),
              "a replaced parent cannot be accepted by retrying the original ID later");
    }
    {
        PartialDirectoryRows rows(parent); rows.ObserveOriginal(file, first);
        Check(!rows.BeginRetry(parent.Volume + 1, parent.FileId), "a different volume is refused");
    }
    {
        PartialDirectoryRows rows(parent); rows.ObserveOriginal(file, first); rows.ObserveOriginal(child, next);
        rows.BeginRetry(parent.Volume, parent.FileId); rows.ObserveRetry(file, first);
        Check(!rows.Complete(), "a previously observed child missing from the retry is incomplete");
    }
    {
        PartialDirectoryRows rows(parent); const auto upper = Row(L"A.txt");
        rows.ObserveOriginal(file, first); rows.ObserveOriginal(upper, first); rows.BeginRetry(parent.Volume, parent.FileId);
        Check(rows.ObserveRetry(file, first) == RetryRowDecision::KeepOriginal &&
              rows.ObserveRetry(upper, first) == RetryRowDecision::KeepOriginal && rows.Complete(),
              "case-distinct hard links are retained as separate names");
    }
    {
        PartialDirectoryRows rows(parent);
        Check(rows.ObserveOriginal(file, first) && rows.ObserveOriginal(file, replacement) &&
              !rows.BeginRetry(parent.Volume, parent.FileId) && !rows.Complete(),
              "duplicate original names are detected before any retry row is published");
    }
    {
        PartialDirectoryRows rows(parent); auto changed = file; changed.nFileSizeLow++;
        rows.ObserveOriginal(file, first); rows.BeginRetry(parent.Volume, parent.FileId);
        Check(rows.ObserveRetry(changed, first) == RetryRowDecision::Conflict && !rows.Complete(),
              "same identity with changed displayed metadata is incomplete");
    }
    {
        PartialDirectoryRows rows(parent); auto changed = file; wcscpy_s(changed.cAlternateFileName, L"CHANGED.TXT");
        rows.ObserveOriginal(file, first); rows.BeginRetry(parent.Volume, parent.FileId);
        Check(rows.ObserveRetry(changed, first) == RetryRowDecision::Conflict && !rows.Complete(),
              "changed alternate name is reported as an incomplete listing");
    }
    {
        PartialDirectoryRows rows(parent); rows.BeginRetry(parent.Volume, parent.FileId);
        Check(rows.ObserveRetry(file, first) == RetryRowDecision::Add &&
              rows.ObserveRetry(file, first) == RetryRowDecision::Conflict && !rows.Complete(),
              "duplicate retry rows are rejected rather than emitted twice");
    }
    {
        PartialDirectoryRows rows(parent); const auto bad = Row(L"..\\outside");
        Check(!rows.ObserveOriginal(bad, first) && !rows.BeginRetry(parent.Volume, parent.FileId),
              "invalid component names fail closed before retry");
    }
    {
        DeniedDirectory unverified = parent; unverified.Verified = false; PartialDirectoryRows rows(unverified);
        Check(!rows.ObserveOriginal(file, first) && !rows.BeginRetry(parent.Volume, parent.FileId),
              "an unverified parent never enables a retry");
    }
    return failures ? 1 : 0;
}
