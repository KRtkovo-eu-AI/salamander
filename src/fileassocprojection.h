// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-08 | Require explicit target resolution before projection. | Prevent runtime application of associations whose stable target or legacy metadata cannot be resolved. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |
// | 2026-11-08 | Add a transactional runtime projection boundary. | Prevent incomplete target resolution from partially replacing legacy runtime associations. | https://github.com/KRtkovo-eu-AI/salamander/issues/733 | OpenAI GPT-5.6 Luna Pro |

#pragma once

#include "fileassoc.h"

#include <cstddef>
#include <string>

namespace Salamander
{
namespace FileAssociations
{

typedef bool (*RuntimeProjectionBegin)(void* context);
typedef bool (*RuntimeProjectionResolve)(const Association& association, void* context);
typedef bool (*RuntimeProjectionApply)(const Association& association, void* context);
typedef void (*RuntimeProjectionRollback)(void* context);
typedef bool (*RuntimeProjectionCommit)(void* context);

struct RuntimeProjectionOperations
{
    RuntimeProjectionBegin Begin;
    RuntimeProjectionResolve Resolve;
    RuntimeProjectionApply Apply;
    RuntimeProjectionRollback Rollback;
    RuntimeProjectionCommit Commit;
    void* Context;
    void* ResolveContext;

    RuntimeProjectionOperations()
        : Begin(NULL), Resolve(NULL), Apply(NULL), Rollback(NULL), Commit(NULL),
          Context(NULL), ResolveContext(NULL) {}
};

struct RuntimeProjectionReport
{
    size_t CommittedCount;
    size_t BlockedCount;
    bool RuntimeProjectionBlocked;

    RuntimeProjectionReport()
        : CommittedCount(0), BlockedCount(0), RuntimeProjectionBlocked(false) {}
};

// The callback owner must resolve stable targets and provide all legacy runtime
// metadata before Apply changes its staging area. The projection is committed
// only after every association succeeds; otherwise Rollback must restore the
// previous runtime projection.
bool ProjectRuntimeAssociations(const CFileAssociationModel& model,
                                const RuntimeProjectionOperations& operations,
                                RuntimeProjectionReport* report = NULL,
                                std::string* error = NULL);

} // namespace FileAssociations
} // namespace Salamander
