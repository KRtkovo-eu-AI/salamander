// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "../salamatrix/salamatrix_script_runner.h"

class CGeneratedScriptRunner : public Salamatrix::Automation::IScriptRunner
{
public:
    virtual DWORD WINAPI GetVersion() const;
    virtual BOOL WINAPI ExecuteGenerated(
        const Salamatrix::Automation::GeneratedScriptRequest* request,
        Salamatrix::Automation::GeneratedScriptResult* result);
    virtual BOOL WINAPI RefreshExtensions();
};
