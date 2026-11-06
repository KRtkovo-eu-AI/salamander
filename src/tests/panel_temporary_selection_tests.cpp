// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
#include "../panel_temporary_selection.h"
#include <algorithm>
#include <cstdio>
#include <vector>

static int Checks = 0;
static int Failures = 0;
static void Check(bool ok, const char* text)
{
    ++Checks;
    printf("%s %s\n", ok ? "PASS" : "FAIL", text);
    if (!ok) ++Failures;
}
struct Row
{
    std::wstring Identity;
    bool Selected;
};
static int Find(const CPanelTemporarySelection& snapshot, const std::vector<Row>& rows)
{
    return FindBranchTemporarySelection(snapshot, (int)rows.size(),
        [&](int index) -> const std::wstring& { return rows[index].Identity; });
}
static CPanelTemporarySelection Snapshot(const std::wstring& identity)
{
    CPanelTemporarySelection result;
    result.Name = "same.png";
    result.BranchIdentity = identity;
    return result;
}
static void Cleanup(const CPanelTemporarySelection& snapshot, std::vector<Row>& rows)
{
    const int index = Find(snapshot, rows);
    if (index != -1) rows[index].Selected = false;
}
int main()
{
    const std::vector<Row> duplicateRows = {
        {L"C:\\root\\ascii\\same.png", false},
        {L"C:\\root\\\u017elu\u0165ou\u010dk\u00fd\u6771\u4eac\\same.png", false},
        {L"C:\\root\\third\\same.png", false}};
    for (size_t target = 0; target < duplicateRows.size(); ++target)
    {
        auto rows = duplicateRows;
        rows[target].Selected = true;
        const auto snapshot = Snapshot(rows[target].Identity);
        Check(Find(snapshot, rows) == (int)target, "first/middle/last duplicate resolves to its exact full path");
        Cleanup(snapshot, rows);
        Check(std::none_of(rows.begin(), rows.end(), [](const Row& row) { return row.Selected; }),
              "cancel removes the temporary mark from the focused duplicate");
    }

    auto snapshot = Snapshot(duplicateRows[1].Identity);
    std::vector<Row> reordered = {duplicateRows[2], duplicateRows[0], duplicateRows[1]};
    reordered.back().Selected = true;
    Check(Find(snapshot, reordered) == 2, "sorting during an operation changes index without changing identity");
    Cleanup(snapshot, reordered);
    Check(!reordered[2].Selected, "cleanup survives replacement row/string allocations after refresh");

    std::vector<Row> gone = {duplicateRows[0], duplicateRows[2]};
    gone[0].Selected = true;
    gone[1].Selected = true;
    Check(Find(snapshot, gone) == -1, "removed target never falls back to a same-basename sibling");
    Cleanup(snapshot, gone);
    Check(gone[0].Selected && gone[1].Selected, "removed target leaves other explicit selections untouched");

    CPanelTemporarySelection explicitSelection;
    int accesses = 0;
    Check(FindBranchTemporarySelection(explicitSelection, 3,
              [&](int) { ++accesses; return duplicateRows[0].Identity; }) == -1 && accesses == 0,
          "pre-existing explicit selection produces no temporary target and no cleanup lookup");
    auto explicitRows = duplicateRows;
    explicitRows[0].Selected = true;
    explicitRows[2].Selected = true;
    Cleanup(explicitSelection, explicitRows);
    Check(explicitRows[0].Selected && !explicitRows[1].Selected && explicitRows[2].Selected,
          "cancel preserves an existing multi-selection exactly");

    CPanelTemporarySelection ordinary;
    ordinary.Name = "same.png";
    Check(Find(ordinary, duplicateRows) == -1, "ordinary basename snapshot cannot enter Branch identity matching");
    snapshot.Clear();
    Check(snapshot.Name.empty() && snapshot.BranchIdentity.empty() && Find(snapshot, duplicateRows) == -1,
          "clearing a reused snapshot clears both names and disables cleanup");

    std::vector<Row> caseVariants = {
        {L"C:\\root\\\u0160\\same.png", true},
        {L"C:\\root\\\u0161\\same.png", true},
        {L"C:\\root\\e\u0301\\same.png", true},
        {L"C:\\root\\\u00e9\\same.png", true}};
    auto lower = Snapshot(caseVariants[1].Identity);
    Cleanup(lower, caseVariants);
    Check(caseVariants[0].Selected && !caseVariants[1].Selected,
          "case-sensitive directory identities are not Unicode-case-folded together");
    auto composed = Snapshot(caseVariants[3].Identity);
    Cleanup(composed, caseVariants);
    Check(caseVariants[2].Selected && !caseVariants[3].Selected,
          "canonically equivalent but distinct filesystem paths are not normalized together");

    std::wstring longDirectory = L"C:\\root";
    for (int i = 0; i < 12; ++i) longDirectory += L"\\" + std::wstring(40, L'x');
    std::vector<Row> longRows = {
        {longDirectory + L"\\first\\same.png", true},
        {longDirectory + L"\\\u6771\u4eac\U0001f642\\same.png", true}};
    auto longSnapshot = Snapshot(longRows[1].Identity);
    Check(longSnapshot.BranchIdentity.size() > 260 && Find(longSnapshot, longRows) == 1,
          "UTF-16 paths longer than MAX_PATH retain their complete identity");
    Cleanup(longSnapshot, longRows);
    Check(longRows[0].Selected && !longRows[1].Selected,
          "long Unicode and supplementary-character paths clean up only the intended file");

    const std::wstring multibyteDirectory = L"C:\\" + std::wstring(150, L'\u0161');
    std::vector<Row> multibyteRows = {
        {multibyteDirectory + L"\\first\\same.png", true},
        {multibyteDirectory + L"\\second\\same.png", true}};
    auto multibyteSnapshot = Snapshot(multibyteRows[1].Identity);
    Check(multibyteSnapshot.BranchIdentity.size() < 260 && Find(multibyteSnapshot, multibyteRows) == 1,
          "path with UTF-8 bytes above MAX_PATH and UTF-16 length below it is lossless");

    const std::wstring commonName(240, L'\u6771');
    std::vector<Row> truncatedDisplay = {
        {L"C:\\root\\" + commonName + L"a.txt", true},
        {L"C:\\root\\" + commonName + L"b.txt", true}};
    auto exactName = Snapshot(truncatedDisplay[1].Identity);
    exactName.Name = std::string(259, 'x'); // the display mirror is deliberately insufficient
    Cleanup(exactName, truncatedDisplay);
    Check(truncatedDisplay[0].Selected && !truncatedDisplay[1].Selected,
          "equal truncated display mirrors cannot confuse full wide identities");

    std::vector<Row> empty;
    Check(Find(exactName, empty) == -1, "empty refreshed listing has no cleanup target");
    printf("%d checks, %d failures\n", Checks, Failures);
    return Failures == 0 ? 0 : 1;
}
