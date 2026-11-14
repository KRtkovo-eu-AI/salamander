// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-11 | Expose the real Branch scanner to the isolated panel harness. | Test pending worker batches at explicit Stop. | #731 | GPT-6.1 Terra |
#include "../branch_view.h"
using namespace Salamander::BranchView;
extern "C" void* BranchTestCreate() { return new Scan; }
extern "C" void BranchTestDestroy(void* scan) { delete static_cast<Scan*>(scan); }
extern "C" bool BranchTestStart(void* scan, const wchar_t* path) { return static_cast<Scan*>(scan)->Start(path, false); }
extern "C" bool BranchTestWait(void* scan) { return static_cast<Scan*>(scan)->WaitForTest(10000); }
extern "C" void BranchTestCancel(void* scan) { static_cast<Scan*>(scan)->Cancel(); }
extern "C" void BranchTestDrain(void* scan, std::vector<Entry>* entries, Progress* progress)
{ static_cast<Scan*>(scan)->Drain(*entries, *progress); }
